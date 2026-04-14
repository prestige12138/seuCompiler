#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include "lalr_converter.h"
#include "lr1_pda.h"
#include "parse_table.h"
#include "symbol_table.h"
#include "yacc_parser.h"

namespace {

using seu_yacc::ITEM;
using seu_yacc::LALRConverter;
using seu_yacc::LALRResult;
using seu_yacc::LR1Builder;
using seu_yacc::LRPDA;
using seu_yacc::ParseTableBuilder;
using seu_yacc::PrecedenceDeclaration;
using seu_yacc::SemanticSymbol;
using seu_yacc::SymbolTableManager;
using seu_yacc::YaccParser;
using seu_yacc::YaccRule;
using seu_yacc::YaccSpecification;
using seu_yacc::nonterminals;
using seu_yacc::operators;
using seu_yacc::producers;
using seu_yacc::terminals;

struct LoadedGrammar {
  YaccSpecification specification;
  SymbolTableManager symbols;
};

int decodeQuotedTerminal(const std::string& symbol) {
  if (symbol.size() < 3 || symbol.front() != '\'' || symbol.back() != '\'') {
    throw std::runtime_error("not a quoted terminal: " + symbol);
  }
  if (symbol[1] != '\\') {
    return static_cast<unsigned char>(symbol[1]);
  }
  switch (symbol[2]) {
    case 'n':
      return '\n';
    case 't':
      return '\t';
    case 'r':
      return '\r';
    case '\\':
      return '\\';
    case '\'':
      return '\'';
    case '0':
      return '\0';
    default:
      return static_cast<unsigned char>(symbol[2]);
  }
}

std::string yesNo(bool value) {
  return value ? "yes" : "no";
}

std::string formatSet(const std::set<std::string>& values) {
  std::ostringstream out;
  out << '{';
  bool first = true;
  for (const std::string& value : values) {
    if (!first) {
      out << ',';
    }
    first = false;
    out << value;
  }
  out << '}';
  return out.str();
}

std::string formatItem(const ITEM& item) {
  std::ostringstream out;
  out << item.left << " ->";
  if (item.right.empty() && item.dotpos == 0) {
    out << " .";
  }
  for (std::size_t index = 0; index < item.right.size(); ++index) {
    if (static_cast<int>(index) == item.dotpos) {
      out << " .";
    }
    out << ' ' << item.right[index];
  }
  if (item.dotpos == static_cast<int>(item.right.size()) && !item.right.empty()) {
    out << " .";
  }
  out << " , " << item.predict;
  return out.str();
}

std::string serializeRow(const seu_yacc::parse_table_item& row) {
  std::ostringstream out;
  out << "state=" << row.State() << '\n';
  const std::map<std::string, std::string> actions = row.GetAction();
  for (const auto& entry : actions) {
    out << "A[" << entry.first << "]=" << entry.second << '\n';
  }
  const std::map<std::string, int> gotos = row.GetGoto();
  for (const auto& entry : gotos) {
    out << "G[" << entry.first << "]=" << entry.second << '\n';
  }
  return out.str();
}

LoadedGrammar loadGrammar(const std::string& path) {
  LoadedGrammar loaded;
  loaded.symbols.reset();

  YaccParser parser;
  loaded.specification = parser.parseYaccFile(path);
  for (const std::string& token : loaded.specification.tokenOrder) {
    loaded.symbols.registerTerminal(token);
  }
  for (const auto& entry : loaded.specification.tokenTypes) {
    loaded.symbols.setTokenType(entry.first, entry.second);
  }
  for (const YaccRule& rule : loaded.specification.rules) {
    loaded.symbols.registerNonterminal(rule.grammar.left);
  }
  for (const auto& entry : loaded.specification.nonterminalTypes) {
    loaded.symbols.setNonterminalType(entry.first, entry.second);
  }
  for (const YaccRule& rule : loaded.specification.rules) {
    for (const std::string& symbol : rule.grammar.right) {
      if (std::find(nonterminals.begin(), nonterminals.end(), symbol) == nonterminals.end()) {
        loaded.symbols.registerTerminal(symbol);
      }
    }
  }
  for (const PrecedenceDeclaration& declaration : loaded.specification.precedenceDeclarations) {
    operators group;
    group.level = declaration.level;
    group.rl = declaration.associativity == "left"
                   ? const_cast<char*>("left")
                   : (declaration.associativity == "right" ? const_cast<char*>("right")
                                                            : const_cast<char*>("nonassoc"));
    for (const std::string& symbol : declaration.symbols) {
      if (symbol.size() >= 3 && symbol.front() == '\'' && symbol.back() == '\'') {
        group.op.push_back(static_cast<char>(decodeQuotedTerminal(symbol)));
      }
    }
    loaded.symbols.addOperatorGroup(group, declaration.symbols);
  }
  for (const YaccRule& rule : loaded.specification.rules) {
    loaded.symbols.addProducer(rule.grammar);
  }
  loaded.symbols.setStartSymbol(loaded.specification.startSymbol);
  return loaded;
}

std::pair<int, std::string> parseConflictLocation(const std::string& conflict) {
  const std::string state_tag = "state ";
  const std::string lookahead_tag = " on lookahead ";
  const std::size_t state_begin = conflict.find(state_tag);
  const std::size_t lookahead_begin = conflict.find(lookahead_tag);
  if (state_begin == std::string::npos || lookahead_begin == std::string::npos) {
    return {-1, ""};
  }
  const std::size_t state_value_begin = state_begin + state_tag.size();
  const std::size_t state_value_end = conflict.find(' ', state_value_begin);
  const std::size_t lookahead_value_begin = lookahead_begin + lookahead_tag.size();
  const std::size_t lookahead_value_end = conflict.find(':', lookahead_value_begin);
  if (state_value_end == std::string::npos || lookahead_value_end == std::string::npos) {
    return {-1, ""};
  }
  return {std::stoi(conflict.substr(state_value_begin, state_value_end - state_value_begin)),
          conflict.substr(lookahead_value_begin, lookahead_value_end - lookahead_value_begin)};
}

void commandParseSummary(const std::string& path) {
  YaccParser parser;
  const YaccSpecification specification = parser.parseYaccFile(path);
  int midrule_rules = 0;
  for (const YaccRule& rule : specification.rules) {
    if (rule.grammar.left.rfind("__midrule_", 0) == 0) {
      ++midrule_rules;
    }
  }
  std::cout << "start=" << specification.startSymbol << '\n'
            << "tokens=" << specification.tokenOrder.size() << '\n'
            << "typed_tokens=" << specification.tokenTypes.size() << '\n'
            << "typed_nonterminals=" << specification.nonterminalTypes.size() << '\n'
            << "precedence_groups=" << specification.precedenceDeclarations.size() << '\n'
            << "rules=" << specification.rules.size() << '\n'
            << "midrule_rules=" << midrule_rules << '\n'
            << "verbatim=" << yesNo(!specification.verbatimDefinitions.empty()) << '\n'
            << "union=" << yesNo(!specification.semanticUnion.empty()) << '\n'
            << "user_code=" << yesNo(!specification.userSubroutines.empty()) << '\n';
}

void commandFirstFollow(const std::string& path, const std::vector<std::string>& symbols) {
  const LoadedGrammar loaded = loadGrammar(path);
  LR1Builder builder;
  const auto first_sets = builder.computeFirstSets();
  const auto follow_sets = builder.computeFollowSets(first_sets, loaded.specification.startSymbol);
  for (const std::string& symbol : symbols) {
    const auto first = first_sets.find(symbol);
    std::cout << "FIRST(" << symbol << ")="
              << (first == first_sets.end() ? "{}" : formatSet(first->second)) << '\n';
    const auto follow = follow_sets.find(symbol);
    if (follow != follow_sets.end()) {
      std::cout << "FOLLOW(" << symbol << ")=" << formatSet(follow->second) << '\n';
    }
  }
}

void commandStartItems(const std::string& path) {
  const LoadedGrammar loaded = loadGrammar(path);
  LR1Builder builder;
  const LRPDA canonical = builder.buildCanonicalPDA(loaded.specification.startSymbol);
  if (canonical.nodes.empty()) {
    throw std::runtime_error("canonical PDA is empty");
  }
  std::vector<std::string> lines;
  for (const ITEM& item : canonical.nodes.front().items) {
    lines.push_back(formatItem(item));
  }
  std::sort(lines.begin(), lines.end());
  std::cout << "state0_items=" << lines.size() << '\n';
  for (const std::string& line : lines) {
    std::cout << line << '\n';
  }
}

void commandAutomatonStats(const std::string& path) {
  const LoadedGrammar loaded = loadGrammar(path);
  LR1Builder builder;
  LALRConverter converter;
  ParseTableBuilder table_builder;

  std::vector<std::string> merged_conflicts;
  std::vector<std::string> direct_conflicts;
  const LRPDA canonical = builder.buildCanonicalPDA(loaded.specification.startSymbol);
  const LALRResult merged = converter.convert(canonical);
  const LRPDA direct = builder.buildLALRPDA(loaded.specification.startSymbol);
  const auto merged_table = table_builder.buildLALRTable(
      merged.automaton, loaded.specification, loaded.specification.startSymbol, &merged_conflicts);
  const auto direct_table = table_builder.buildLALRTable(
      direct, loaded.specification, loaded.specification.startSymbol, &direct_conflicts);

  bool table_equal = merged_table.size() == direct_table.size();
  if (table_equal) {
    std::vector<std::string> merged_rows;
    std::vector<std::string> direct_rows;
    for (const auto& row : merged_table) {
      merged_rows.push_back(serializeRow(row));
    }
    for (const auto& row : direct_table) {
      direct_rows.push_back(serializeRow(row));
    }
    std::sort(merged_rows.begin(), merged_rows.end());
    std::sort(direct_rows.begin(), direct_rows.end());
    table_equal = merged_rows == direct_rows;
  }

  std::cout << "canonical_states=" << canonical.nodes.size() << '\n'
            << "merged_lalr_states=" << merged.automaton.nodes.size() << '\n'
            << "direct_lalr_states=" << direct.nodes.size() << '\n'
            << "merged_conflicts=" << merged_conflicts.size() << '\n'
            << "direct_conflicts=" << direct_conflicts.size() << '\n'
            << "table_equal=" << yesNo(table_equal) << '\n';
}

void commandConflictSummary(const std::string& path, const std::string& mode) {
  const LoadedGrammar loaded = loadGrammar(path);
  LR1Builder builder;
  LALRConverter converter;
  ParseTableBuilder table_builder;

  std::vector<std::string> conflicts;
  std::vector<seu_yacc::parse_table_item> table;
  if (mode == "lr1") {
    const LRPDA canonical = builder.buildCanonicalPDA(loaded.specification.startSymbol);
    table = table_builder.buildLR1Table(
        canonical, loaded.specification, loaded.specification.startSymbol, &conflicts);
  } else if (mode == "lalr") {
    const LRPDA canonical = builder.buildCanonicalPDA(loaded.specification.startSymbol);
    const LALRResult merged = converter.convert(canonical);
    table = table_builder.buildLALRTable(
        merged.automaton, loaded.specification, loaded.specification.startSymbol, &conflicts);
  } else {
    throw std::runtime_error("unsupported mode: " + mode);
  }

  int shift_reduce = 0;
  int reduce_reduce = 0;
  int action_conflicts = 0;
  for (const std::string& conflict : conflicts) {
    if (conflict.rfind("shift/reduce", 0) == 0) {
      ++shift_reduce;
    } else if (conflict.rfind("reduce/reduce", 0) == 0) {
      ++reduce_reduce;
    } else {
      ++action_conflicts;
    }
  }

  int err_actions = 0;
  for (const auto& row : table) {
    for (const auto& entry : row.GetAction()) {
      if (entry.second == "err") {
        ++err_actions;
      }
    }
  }

  std::cout << "conflicts=" << conflicts.size() << '\n'
            << "shift_reduce=" << shift_reduce << '\n'
            << "reduce_reduce=" << reduce_reduce << '\n'
            << "action_conflicts=" << action_conflicts << '\n'
            << "err_actions=" << err_actions << '\n';

  if (!conflicts.empty()) {
    std::cout << "first_conflict=" << conflicts.front() << '\n';
    const auto [state, lookahead] = parseConflictLocation(conflicts.front());
    if (state >= 0 && state < static_cast<int>(table.size())) {
      const auto actions = table[static_cast<std::size_t>(state)].GetAction();
      const auto found = actions.find(lookahead);
      if (found != actions.end()) {
        std::cout << "resolved_action=" << found->second << '\n';
      }
    }
  }
}

void commandSymbolScope() {
  SymbolTableManager symbols;
  symbols.reset();
  symbols.enterScope();
  symbols.declareSymbol({"x", "int", 0, 0, false, false});
  symbols.declareSymbol({"y", "float", 0, 4, false, false});

  const std::string outer_x =
      symbols.lookupSymbol("x") == nullptr ? "missing" : symbols.lookupSymbol("x")->type;
  symbols.enterScope();
  symbols.declareSymbol({"x", "char", 1, 8, false, false});
  const std::string inner_x =
      symbols.lookupSymbol("x") == nullptr ? "missing" : symbols.lookupSymbol("x")->type;
  const std::string inner_y =
      symbols.lookupSymbol("y") == nullptr ? "missing" : symbols.lookupSymbol("y")->type;
  symbols.exitScope();
  const std::string restored_x =
      symbols.lookupSymbol("x") == nullptr ? "missing" : symbols.lookupSymbol("x")->type;
  symbols.exitScope();
  const bool missing_x = symbols.lookupSymbol("x") == nullptr;

  std::cout << "outer_x=" << outer_x << '\n'
            << "inner_x=" << inner_x << '\n'
            << "inner_y=" << inner_y << '\n'
            << "after_exit_x=" << restored_x << '\n'
            << "after_all_missing=" << yesNo(missing_x) << '\n';
}

void printUsage(const char* program) {
  std::cerr << "Usage:\n"
            << "  " << program << " parse-summary <file>\n"
            << "  " << program << " first-follow <file> <symbol> [symbol...]\n"
            << "  " << program << " start-items <file>\n"
            << "  " << program << " automaton-stats <file>\n"
            << "  " << program << " conflict-summary <file> <lr1|lalr>\n"
            << "  " << program << " symbol-scope\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc < 2) {
      printUsage(argv[0]);
      return 1;
    }
    const std::string command = argv[1];
    if (command == "parse-summary" && argc == 3) {
      commandParseSummary(argv[2]);
      return 0;
    }
    if (command == "first-follow" && argc >= 4) {
      std::vector<std::string> symbols;
      for (int index = 3; index < argc; ++index) {
        symbols.emplace_back(argv[index]);
      }
      commandFirstFollow(argv[2], symbols);
      return 0;
    }
    if (command == "start-items" && argc == 3) {
      commandStartItems(argv[2]);
      return 0;
    }
    if (command == "automaton-stats" && argc == 3) {
      commandAutomatonStats(argv[2]);
      return 0;
    }
    if (command == "conflict-summary" && argc == 4) {
      commandConflictSummary(argv[2], argv[3]);
      return 0;
    }
    if (command == "symbol-scope" && argc == 2) {
      commandSymbolScope();
      return 0;
    }
    printUsage(argv[0]);
    return 1;
  } catch (const std::exception& ex) {
    std::cerr << "yacc_probe error: " << ex.what() << '\n';
    return 1;
  }
}
