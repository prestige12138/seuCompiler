#include "parse_table.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <utility>
#include <vector>

namespace seu_yacc {
namespace {

constexpr const char* kEndMarker = "$";
constexpr const char* kAugmentedStart = "__SEU_YACC_AUGMENTED_START__";

#ifdef SEU_YACC_TEST_CXX
constexpr const char* kSelfTestCompiler = SEU_YACC_TEST_CXX;
#else
constexpr const char* kSelfTestCompiler = "c++";
#endif

bool isTerminalSymbol(const std::string& symbol) {
  return std::find(terminals.begin(), terminals.end(), symbol) != terminals.end() || symbol == kEndMarker;
}

bool isNonterminalSymbol(const std::string& symbol) {
  return std::find(nonterminals.begin(), nonterminals.end(), symbol) != nonterminals.end() ||
         symbol == kAugmentedStart;
}

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

int tokenCodeForSymbol(const std::string& symbol) {
  if (symbol == kEndMarker) {
    return 0;
  }
  if (symbol.size() >= 3 && symbol.front() == '\'' && symbol.back() == '\'') {
    return decodeQuotedTerminal(symbol);
  }
  int next_code = 256;
  for (const std::string& terminal : terminals) {
    if (terminal.size() >= 3 && terminal.front() == '\'' && terminal.back() == '\'') {
      continue;
    }
    if (terminal == symbol) {
      return next_code;
    }
    ++next_code;
  }
  throw std::runtime_error("failed to assign token code for symbol: " + symbol);
}

std::string basenameOf(const std::string& path) {
  const std::size_t slash = path.find_last_of("/\\");
  if (slash == std::string::npos) {
    return path;
  }
  return path.substr(slash + 1);
}

std::string dirnameOf(const std::string& path) {
  const std::size_t slash = path.find_last_of("/\\");
  if (slash == std::string::npos) {
    return "";
  }
  return path.substr(0, slash);
}

void ensureDirectory(const std::string& path) {
  if (path.empty() || path == ".") {
    return;
  }
  std::size_t start = path.front() == '/' ? 1 : 0;
  while (true) {
    const std::size_t slash = path.find('/', start);
    const std::string current = slash == std::string::npos ? path : path.substr(0, slash);
    if (!current.empty() && current != ".") {
      if (::mkdir(current.c_str(), 0755) != 0 && errno != EEXIST) {
        throw std::runtime_error("failed to create directory: " + current);
      }
    }
    if (slash == std::string::npos) {
      break;
    }
    start = slash + 1;
  }
}

std::string makeTempDir() {
  char pattern[] = "/tmp/seuYacc_runtimeXXXXXX";
  char* created = ::mkdtemp(pattern);
  if (created == nullptr) {
    throw std::runtime_error("failed to create temporary directory");
  }
  return std::string(created);
}

bool fileExists(const std::string& path) {
  struct stat info {};
  return ::stat(path.c_str(), &info) == 0;
}

std::string joinPath(const std::string& left, const std::string& right) {
  if (left.empty()) {
    return right;
  }
  if (left.back() == '/') {
    return left + right;
  }
  return left + "/" + right;
}

std::string absolutePath(const std::string& path) {
  char buffer[4096];
  if (::getcwd(buffer, sizeof(buffer)) == nullptr) {
    throw std::runtime_error("failed to resolve current working directory");
  }
  if (path.empty()) {
    return std::string(buffer);
  }
  if (path.front() == '/') {
    return path;
  }
  return joinPath(std::string(buffer), path);
}

std::string canonicalizeDirectory(const std::string& path) {
  const std::string absolute = absolutePath(path);
  char buffer[4096];
  if (::realpath(absolute.c_str(), buffer) != nullptr) {
    return std::string(buffer);
  }
  return absolute;
}

std::string canonicalizeFilePath(const std::string& path) {
  const std::string absolute = absolutePath(path);
  const std::string directory = dirnameOf(absolute);
  const std::string base = basenameOf(absolute);
  return joinPath(canonicalizeDirectory(directory.empty() ? "." : directory), base);
}

std::vector<std::string> splitPath(const std::string& path) {
  std::vector<std::string> parts;
  std::string current;
  for (char ch : path) {
    if (ch == '/') {
      if (!current.empty() && current != ".") {
        if (current == "..") {
          if (!parts.empty()) {
            parts.pop_back();
          }
        } else {
          parts.push_back(current);
        }
      }
      current.clear();
      continue;
    }
    current.push_back(ch);
  }
  if (!current.empty() && current != ".") {
    if (current == "..") {
      if (!parts.empty()) {
        parts.pop_back();
      }
    } else {
      parts.push_back(current);
    }
  }
  return parts;
}

std::string relativeIncludePath(const std::string& from_path, const std::string& to_path) {
  const std::string from_dir = dirnameOf(from_path);
  const std::vector<std::string> from_parts =
      splitPath(canonicalizeDirectory(from_dir.empty() ? "." : from_dir));
  const std::vector<std::string> to_parts = splitPath(canonicalizeFilePath(to_path));

  std::size_t common = 0;
  while (common < from_parts.size() && common < to_parts.size() &&
         from_parts[common] == to_parts[common]) {
    ++common;
  }

  std::string relative;
  for (std::size_t index = common; index < from_parts.size(); ++index) {
    relative += "../";
  }
  for (std::size_t index = common; index < to_parts.size(); ++index) {
    relative += to_parts[index];
    if (index + 1 != to_parts.size()) {
      relative += '/';
    }
  }

  if (relative.empty()) {
    return basenameOf(to_path);
  }
  return relative;
}

std::string resolveRepoRoot(const std::string& workspace_root) {
  const std::vector<std::string> candidates = {
      workspace_root,
      joinPath(workspace_root, ".."),
      joinPath(joinPath(workspace_root, ".."), ".."),
  };
  for (const std::string& candidate : candidates) {
    if (fileExists(joinPath(candidate, "resources/minic.y")) &&
        fileExists(joinPath(candidate, "resources/minic.l"))) {
      return candidate;
    }
  }
  throw std::runtime_error("failed to locate repository resources directory");
}

int runProcess(const std::string& program, const std::vector<std::string>& args) {
  std::vector<char*> argv;
  argv.reserve(args.size() + 2);
  argv.push_back(const_cast<char*>(program.c_str()));
  for (const std::string& arg : args) {
    argv.push_back(const_cast<char*>(arg.c_str()));
  }
  argv.push_back(nullptr);

  const pid_t pid = ::fork();
  if (pid < 0) {
    throw std::runtime_error("failed to fork child process");
  }
  if (pid == 0) {
    ::execvp(program.c_str(), argv.data());
    std::perror("execvp");
    _exit(127);
  }

  int status = 0;
  constexpr int kTimeoutMs = 60000;
  int waited_ms = 0;
  while (true) {
    const pid_t wait_result = ::waitpid(pid, &status, WNOHANG);
    if (wait_result == pid) {
      break;
    }
    if (wait_result < 0) {
      throw std::runtime_error("failed to wait for child process");
    }
    if (waited_ms >= kTimeoutMs) {
      ::kill(pid, SIGKILL);
      ::waitpid(pid, &status, 0);
      return -1;
    }
    ::usleep(100000);
    waited_ms += 100;
  }
  if (!WIFEXITED(status)) {
    return -1;
  }
  return WEXITSTATUS(status);
}

int productionIndexFromItem(const ITEM& item) {
  for (std::size_t index = 0; index < producers.size(); ++index) {
    if (producers[index].left == item.left && producers[index].right == item.right) {
      return static_cast<int>(index);
    }
  }
  return -1;
}

std::string productionPrecedenceSymbol(const YaccSpecification& specification, int production_index) {
  if (production_index < 0 || production_index >= static_cast<int>(specification.rules.size())) {
    return "";
  }
  if (!specification.rules[static_cast<std::size_t>(production_index)].precedence_symbol.empty()) {
    return specification.rules[static_cast<std::size_t>(production_index)].precedence_symbol;
  }
  const auto& rhs = producers[static_cast<std::size_t>(production_index)].right;
  for (auto it = rhs.rbegin(); it != rhs.rend(); ++it) {
    if (isTerminalSymbol(*it)) {
      return *it;
    }
  }
  return "";
}

std::pair<int, std::string> precedenceOfSymbol(const YaccSpecification& specification,
                                               const std::string& symbol) {
  for (const PrecedenceDeclaration& decl : specification.precedenceDeclarations) {
    if (std::find(decl.symbols.begin(), decl.symbols.end(), symbol) != decl.symbols.end()) {
      return {decl.level, decl.associativity};
    }
  }
  return {-1, ""};
}

std::string resolveConflict(const std::string& existing,
                            const std::string& incoming,
                            const std::string& lookahead,
                            const YaccSpecification& specification,
                            std::vector<std::string>* conflicts,
                            int state) {
  if (existing == incoming) {
    return existing;
  }
  const bool existing_shift = !existing.empty() && existing.front() == 's';
  const bool incoming_shift = !incoming.empty() && incoming.front() == 's';
  const bool existing_reduce = !existing.empty() && existing.front() == 'r';
  const bool incoming_reduce = !incoming.empty() && incoming.front() == 'r';

  auto pushConflict = [&](const std::string& label) {
    if (conflicts != nullptr) {
      std::ostringstream oss;
      oss << label << " conflict in state " << state << " on lookahead " << lookahead << ": "
          << existing << " vs " << incoming;
      conflicts->push_back(oss.str());
    }
  };

  if ((existing_shift && incoming_reduce) || (existing_reduce && incoming_shift)) {
    const std::string shift_action = existing_shift ? existing : incoming;
    const std::string reduce_action = existing_reduce ? existing : incoming;
    const int reduce_index = std::stoi(reduce_action.substr(1));
    const std::string precedence_symbol = productionPrecedenceSymbol(specification, reduce_index);
    const auto token_precedence = precedenceOfSymbol(specification, lookahead);
    const auto production_precedence = precedenceOfSymbol(specification, precedence_symbol);
    pushConflict("shift/reduce");
    if (token_precedence.first < 0 || production_precedence.first < 0) {
      return shift_action;
    }
    if (token_precedence.first > production_precedence.first) {
      return shift_action;
    }
    if (token_precedence.first < production_precedence.first) {
      return reduce_action;
    }
    if (token_precedence.second == "left") {
      return reduce_action;
    }
    if (token_precedence.second == "right") {
      return shift_action;
    }
    return "err";
  }

  if (existing_reduce && incoming_reduce) {
    pushConflict("reduce/reduce");
    return std::stoi(existing.substr(1)) <= std::stoi(incoming.substr(1)) ? existing : incoming;
  }

  pushConflict("action");
  return existing;
}

std::string sanitizeIdentifier(const std::string& raw) {
  std::string out;
  for (char ch : raw) {
    if (std::isalnum(static_cast<unsigned char>(ch)) != 0) {
      out.push_back(ch);
    } else {
      out.push_back('_');
    }
  }
  if (out.empty() || std::isdigit(static_cast<unsigned char>(out.front())) != 0) {
    out.insert(out.begin(), '_');
  }
  return out;
}

bool isValidIdentifier(const std::string& raw) {
  if (raw.empty()) {
    return false;
  }
  const unsigned char first = static_cast<unsigned char>(raw.front());
  if (std::isalpha(first) == 0 && raw.front() != '_') {
    return false;
  }
  for (char ch : raw) {
    const unsigned char value = static_cast<unsigned char>(ch);
    if (std::isalnum(value) == 0 && ch != '_') {
      return false;
    }
  }
  return true;
}

std::string escapeCppString(const std::string& raw) {
  std::ostringstream oss;
  for (char ch : raw) {
    switch (ch) {
      case '\\':
        oss << "\\\\";
        break;
      case '"':
        oss << "\\\"";
        break;
      case '\n':
        oss << "\\n";
        break;
      case '\r':
        oss << "\\r";
        break;
      case '\t':
        oss << "\\t";
        break;
      default:
        oss << ch;
        break;
    }
  }
  return oss.str();
}

std::string tokenEnumName(const std::string& symbol, int code) {
  if (isValidIdentifier(symbol)) {
    return symbol;
  }
  return "TOK_" + sanitizeIdentifier(symbol) + "_" + std::to_string(code);
}

std::string stripOuterBraces(const std::string& action) {
  if (action.size() >= 2 && action.front() == '{' && action.back() == '}') {
    return action.substr(1, action.size() - 2);
  }
  return action;
}

std::string semanticFieldForSymbol(const YaccSpecification& specification, const std::string& symbol) {
  const auto nonterminal = specification.nonterminalTypes.find(symbol);
  if (nonterminal != specification.nonterminalTypes.end()) {
    return sanitizeIdentifier(nonterminal->second);
  }
  const auto terminal = specification.tokenTypes.find(symbol);
  if (terminal != specification.tokenTypes.end()) {
    return sanitizeIdentifier(terminal->second);
  }
  return "";
}

std::string typedValueAccess(const YaccRule& rule,
                             const std::string& tag,
                             int rhs_index) {
  const std::string member = sanitizeIdentifier(tag);
  if (rhs_index == 0) {
    return member.empty() ? "yyval_ref" : "yyval_ref." + member;
  }
  if (rhs_index > 0 && rhs_index <= static_cast<int>(rule.grammar.right.size())) {
    const std::string base =
        "rhs[static_cast<std::size_t>(" + std::to_string(rhs_index - 1) + ")].value";
    if (!member.empty()) {
      return base + "." + member;
    }
    return base;
  }
  return "";
}

std::string inferredValueAccess(const YaccSpecification& specification,
                                const YaccRule& rule,
                                int rhs_index) {
  if (rhs_index == 0) {
    const std::string member = semanticFieldForSymbol(specification, rule.grammar.left);
    return member.empty() ? "yyval_ref" : "yyval_ref." + member;
  }
  if (rhs_index > 0 && rhs_index <= static_cast<int>(rule.grammar.right.size())) {
    const std::string base =
        "rhs[static_cast<std::size_t>(" + std::to_string(rhs_index - 1) + ")].value";
    const std::string member =
        semanticFieldForSymbol(specification, rule.grammar.right[static_cast<std::size_t>(rhs_index - 1)]);
    return member.empty() ? base : base + "." + member;
  }
  return "";
}

std::string translateSemanticAction(const std::string& action,
                                    const YaccRule& rule,
                                    const YaccSpecification& specification) {
  const std::string body = stripOuterBraces(action);
  std::ostringstream out;
  bool in_string = false;
  bool in_char = false;
  bool in_line_comment = false;
  bool in_block_comment = false;
  bool escaping = false;

  for (std::size_t index = 0; index < body.size();) {
    const char ch = body[index];
    if (escaping) {
      out << ch;
      escaping = false;
      ++index;
      continue;
    }
    if (in_line_comment) {
      out << ch;
      if (ch == '\n') {
        in_line_comment = false;
      }
      ++index;
      continue;
    }
    if (in_block_comment) {
      out << ch;
      if (ch == '*' && index + 1 < body.size() && body[index + 1] == '/') {
        out << '/';
        index += 2;
        in_block_comment = false;
        continue;
      }
      ++index;
      continue;
    }
    if ((in_string || in_char) && ch == '\\') {
      out << ch;
      escaping = true;
      ++index;
      continue;
    }
    if (!in_char && ch == '"') {
      out << ch;
      in_string = !in_string;
      ++index;
      continue;
    }
    if (!in_string && ch == '\'') {
      out << ch;
      in_char = !in_char;
      ++index;
      continue;
    }
    if (in_string || in_char) {
      out << ch;
      ++index;
      continue;
    }
    if (ch == '/' && index + 1 < body.size() && body[index + 1] == '/') {
      out << "//";
      index += 2;
      in_line_comment = true;
      continue;
    }
    if (ch == '/' && index + 1 < body.size() && body[index + 1] == '*') {
      out << "/*";
      index += 2;
      in_block_comment = true;
      continue;
    }
    if (ch == '$') {
      if (index + 1 < body.size() && body[index + 1] == '$') {
        out << inferredValueAccess(specification, rule, 0);
        index += 2;
        continue;
      }
      if (index + 1 < body.size() && body[index + 1] == '<') {
        const std::size_t tag_end = body.find('>', index + 2);
        if (tag_end != std::string::npos) {
          const std::string tag = body.substr(index + 2, tag_end - index - 2);
          std::size_t cursor = tag_end + 1;
          if (cursor < body.size() && body[cursor] == '$') {
            out << typedValueAccess(rule, tag, 0);
            index = cursor + 1;
            continue;
          }
          const std::size_t number_begin = cursor;
          while (cursor < body.size() && std::isdigit(static_cast<unsigned char>(body[cursor])) != 0) {
            ++cursor;
          }
          if (cursor > number_begin) {
            const int rhs_index = std::stoi(body.substr(number_begin, cursor - number_begin));
            if (rhs_index > 0) {
              out << typedValueAccess(rule, tag, rhs_index);
              index = cursor;
              continue;
            }
          }
        }
      }
      std::size_t cursor = index + 1;
      while (cursor < body.size() && std::isdigit(static_cast<unsigned char>(body[cursor])) != 0) {
        ++cursor;
      }
      if (cursor > index + 1) {
        const int rhs_index = std::stoi(body.substr(index + 1, cursor - index - 1));
        if (rhs_index > 0) {
          out << inferredValueAccess(specification, rule, rhs_index);
          index = cursor;
          continue;
        }
      }
    }
    out << ch;
    ++index;
  }
  return out.str();
}

std::string parserNamespace(const std::string& out_cpp_path) {
  const std::string base = basenameOf(out_cpp_path);
  const std::size_t dot = base.find('.');
  const std::string stem = dot == std::string::npos ? base : base.substr(0, dot);
  return sanitizeIdentifier(stem) + "_generated";
}

}  // namespace

parse_table_item::parse_table_item(LRnode x) : state(x.stateindex) {}

std::map<std::string, std::string> parse_table_item::GetAction() const {
  return action;
}

std::map<std::string, int> parse_table_item::GetGoto() const {
  return gotos;
}

void parse_table_item::AddtoAction(const std::string& ter, const std::string& rs) {
  action[ter] = rs;
}

void parse_table_item::AddtoGoto(const std::string& nonter, int s) {
  gotos[nonter] = s;
}

int parse_table_item::State() const {
  return state;
}

std::vector<parse_table_item> ParseTableBuilder::buildLR1Table(
    const LRPDA& automaton,
    const YaccSpecification& specification,
    const std::string& start_symbol,
    std::vector<std::string>* conflicts) const {
  (void)start_symbol;
  std::vector<parse_table_item> table;
  table.reserve(automaton.nodes.size());
  for (const LRnode& state : automaton.nodes) {
    table.emplace_back(state);
  }

  for (const LRnode& state : automaton.nodes) {
    parse_table_item& row = table[static_cast<std::size_t>(state.stateindex)];
    for (const auto& edge : state.nextnode) {
      if (isTerminalSymbol(edge.first)) {
        const std::string incoming = "s" + std::to_string(edge.second);
        const auto existing = row.GetAction();
        const auto found = existing.find(edge.first);
        if (found == existing.end()) {
          row.AddtoAction(edge.first, incoming);
        } else {
          row.AddtoAction(edge.first, resolveConflict(found->second,
                                                      incoming,
                                                      edge.first,
                                                      specification,
                                                      conflicts,
                                                      state.stateindex));
        }
      } else if (isNonterminalSymbol(edge.first)) {
        row.AddtoGoto(edge.first, edge.second);
      }
    }

    for (const ITEM& item : state.items) {
      if (item.dotpos != static_cast<int>(item.right.size())) {
        continue;
      }
      if (item.left == kAugmentedStart && item.predict == kEndMarker) {
        row.AddtoAction(kEndMarker, "acc");
        continue;
      }
      const int production_index = productionIndexFromItem(item);
      if (production_index < 0) {
        continue;
      }
      const std::string incoming = "r" + std::to_string(production_index);
      const auto existing = row.GetAction();
      const auto found = existing.find(item.predict);
      if (found == existing.end()) {
        row.AddtoAction(item.predict, incoming);
      } else {
        row.AddtoAction(item.predict, resolveConflict(found->second,
                                                      incoming,
                                                      item.predict,
                                                      specification,
                                                      conflicts,
                                                      state.stateindex));
      }
    }
  }
  return table;
}

std::vector<parse_table_item> ParseTableBuilder::buildLALRTable(
    const LRPDA& automaton,
    const YaccSpecification& specification,
    const std::string& start_symbol,
    std::vector<std::string>* conflicts) const {
  return buildLR1Table(automaton, specification, start_symbol, conflicts);
}

void ParserCodeGenerator::emitParser(const std::vector<parse_table_item>& table,
                                     const LRPDA& automaton,
                                     const YaccSpecification& specification,
                                     const std::string& start_symbol,
                                     const std::string& out_cpp_path,
                                     const std::string& out_header_path) const {
  (void)automaton;
  (void)start_symbol;
  ensureDirectory(dirnameOf(out_cpp_path));
  ensureDirectory(dirnameOf(out_header_path));

  const std::string name_space = parserNamespace(out_cpp_path);
  const std::string header_include = relativeIncludePath(out_cpp_path, out_header_path);
  const bool has_semantic_union = !specification.semanticUnion.empty();

  std::ofstream header(out_header_path);
  if (!header) {
    throw std::runtime_error("failed to open generated parser header path: " + out_header_path);
  }
  header << "#pragma once\n\n"
         << "#include <string>\n"
         << "#include <vector>\n\n"
         << "namespace " << name_space << " {\n\n";
  if (has_semantic_union) {
    header << "union YYSTYPE " << specification.semanticUnion << ";\n\n";
  } else {
    header << "struct YYSTYPE {\n"
           << "  std::string lexeme;\n"
           << "};\n\n";
  }
  header << "enum TokenKind {\n"
         << "  YYEOF_TOKEN = 0,\n";
  int next_code = 256;
  for (const std::string& terminal : terminals) {
    if (terminal == kEndMarker || (terminal.size() >= 3 && terminal.front() == '\'' && terminal.back() == '\'')) {
      continue;
    }
    header << "  " << tokenEnumName(terminal, next_code) << " = " << next_code++ << ",\n";
  }
  header << "};\n\n"
         << "struct Token {\n"
         << "  int type = 0;\n"
         << "  std::string lexeme;\n"
         << "  int line = 0;\n"
         << "  int column = 0;\n"
         << "  YYSTYPE semantic{};\n"
         << "};\n\n"
         << "using yytoken_type = Token;\n"
         << "using yysemantic_type = YYSTYPE;\n\n"
         << "bool yyparse(const std::vector<Token>& tokens);\n\n"
         << "}  // namespace " << name_space << "\n\n"
         << "#if defined(SEU_YACC_TOKEN_NAMESPACE) || defined(SEU_YACC_TOKEN_TYPE) || \\\n"
         << "    defined(SEU_YACC_SEMANTIC_TYPE)\n"
         << "#error \"multiple generated seuYacc token ABI headers included in one translation unit\"\n"
         << "#endif\n"
         << "#define SEU_YACC_TOKEN_NAMESPACE " << name_space << "\n"
         << "#define SEU_YACC_TOKEN_TYPE " << name_space << "::Token\n"
         << "#define SEU_YACC_SEMANTIC_TYPE " << name_space << "::YYSTYPE\n";

  std::ofstream output(out_cpp_path);
  if (!output) {
    throw std::runtime_error("failed to open generated parser path: " + out_cpp_path);
  }

  output << "#include \"" << escapeCppString(header_include) << "\"\n\n"
         << "#include <algorithm>\n"
         << "#include <iostream>\n"
         << "#include <string>\n"
         << "#include <unordered_map>\n"
         << "#include <utility>\n"
         << "#include <vector>\n\n"
         << specification.verbatimDefinitions << '\n';
  if (!specification.userSubroutines.empty()) {
    output << "\nusing " << name_space << "::Token;\n"
           << "using " << name_space << "::YYSTYPE;\n\n"
           << specification.userSubroutines << '\n';
  }
  output << '\n'
         << "namespace " << name_space << " {\n\n"
         << "namespace {\n"
         << "struct SemanticValue {\n"
         << "  std::string symbol;\n"
         << "  std::string lexeme;\n"
         << "  YYSTYPE value{};\n"
         << "};\n\n"
         << "struct RuntimeSymbol {\n"
         << "  std::string name;\n"
         << "  std::string type;\n"
         << "  int scope = 0;\n"
         << "  bool is_function = false;\n"
         << "  bool is_parameter = false;\n"
         << "};\n\n"
         << "class RuntimeSymbolTable {\n"
         << " public:\n"
         << "  RuntimeSymbolTable() { scopes_.push_back({}); }\n"
         << "  void enterScope() { scopes_.push_back({}); }\n"
         << "  void exitScope() { if (scopes_.size() > 1) scopes_.pop_back(); }\n"
         << "  void declareSymbol(const RuntimeSymbol& symbol) { scopes_.back()[symbol.name] = symbol; }\n"
         << " private:\n"
         << "  std::vector<std::unordered_map<std::string, RuntimeSymbol>> scopes_;\n"
         << "};\n\n"
         << "const std::vector<std::string> kProductionLeft = {\n";
  for (std::size_t index = 0; index < producers.size(); ++index) {
    output << "  \"" << escapeCppString(producers[index].left) << "\"";
    if (index + 1 != producers.size()) {
      output << ',';
    }
    output << '\n';
  }
  output << "};\n\n"
         << "const std::vector<int> kProductionSize = {";
  for (std::size_t index = 0; index < producers.size(); ++index) {
    if (index != 0) {
      output << ", ";
    }
    output << producers[index].right.size();
  }
  output << "};\n\n"
         << "const std::vector<std::unordered_map<int, std::string>> kAction = {\n";

  for (std::size_t row = 0; row < table.size(); ++row) {
    output << "  {";
    const auto actions = table[row].GetAction();
    bool first = true;
    for (const auto& entry : actions) {
      if (!first) {
        output << ", ";
      }
      first = false;
      output << '{' << tokenCodeForSymbol(entry.first) << ", \"" << entry.second << "\"}";
    }
    output << "}";
    if (row + 1 != table.size()) {
      output << ",";
    }
    output << '\n';
  }
  output << "};\n\n"
         << "const std::vector<std::unordered_map<std::string, int>> kGoto = {\n";
  for (std::size_t row = 0; row < table.size(); ++row) {
    output << "  {";
    const auto gotos = table[row].GetGoto();
    bool first = true;
    for (const auto& entry : gotos) {
      if (!first) {
        output << ", ";
      }
      first = false;
      output << "{\"" << escapeCppString(entry.first) << "\", " << entry.second << "}";
    }
    output << "}";
    if (row + 1 != table.size()) {
      output << ",";
    }
    output << '\n';
  }
  output << "};\n\n"
         << "std::string tokenSymbol(int type) {\n"
         << "  switch (type) {\n"
         << "    case 0: return \"$\";\n";
  next_code = 256;
  for (const std::string& terminal : terminals) {
    if (terminal == kEndMarker || (terminal.size() >= 3 && terminal.front() == '\'' && terminal.back() == '\'')) {
      continue;
    }
    output << "    case " << next_code++ << ": return \"" << escapeCppString(terminal) << "\";\n";
  }
  output << "    default:\n"
         << "      if (type >= 0 && type < 128) {\n"
         << "        return std::string(\"'\") + static_cast<char>(type) + \"'\";\n"
         << "      }\n"
         << "      return \"<unknown>\";\n"
         << "  }\n"
         << "}\n\n"
         << "std::string joinedType(const std::vector<SemanticValue>& rhs) {\n"
         << "  static const std::vector<std::string> kTypeTokens = {\n"
         << "      \"VOID\", \"CHAR\", \"SHORT\", \"INT\", \"LONG\", \"FLOAT\", \"DOUBLE\", \"SIGNED\", \"UNSIGNED\", \"BOOL\", \"STRUCT\", \"UNION\", \"ENUM\", \"TYPE_NAME\"};\n"
         << "  std::string type;\n"
         << "  for (const SemanticValue& value : rhs) {\n"
         << "    if (std::find(kTypeTokens.begin(), kTypeTokens.end(), value.symbol) != kTypeTokens.end()) {\n"
         << "      if (!type.empty()) type += ' ';\n"
         << "      type += value.symbol;\n"
         << "    }\n"
         << "  }\n"
         << "  return type;\n"
         << "}\n\n"
         << "void observeReduction(int production, const std::vector<SemanticValue>& rhs, RuntimeSymbolTable* symbols) {\n"
         << "  if (symbols == nullptr) return;\n"
         << "  const std::string& left = kProductionLeft[static_cast<std::size_t>(production)];\n"
         << "  const std::string type = joinedType(rhs);\n"
         << "  if (left == \"declaration\" || left == \"parameter_declaration\" || left == \"function_definition\") {\n"
         << "    for (const SemanticValue& value : rhs) {\n"
         << "      if (value.symbol == \"IDENTIFIER\" && !value.lexeme.empty()) {\n"
         << "        RuntimeSymbol symbol;\n"
         << "        symbol.name = value.lexeme;\n"
         << "        symbol.type = type;\n"
         << "        symbol.is_function = left == \"function_definition\";\n"
         << "        symbol.is_parameter = left == \"parameter_declaration\";\n"
         << "        symbols->declareSymbol(symbol);\n"
         << "      }\n"
         << "    }\n"
         << "  }\n"
         << "}\n\n"
         << "void executeAction(int production, const std::vector<SemanticValue>& rhs, YYSTYPE* yyval) {\n"
         << "  YYSTYPE& yyval_ref = *yyval;\n"
         << "  switch (production) {\n";
  for (std::size_t index = 0; index < specification.rules.size(); ++index) {
    output << "    case " << index << ": {\n";
    if (specification.rules[index].action.empty()) {
      output << "      if (!rhs.empty()) {\n"
             << "        yyval_ref = rhs.front().value;\n"
             << "      }\n";
    } else {
      std::istringstream action_stream(
          translateSemanticAction(specification.rules[index].action, specification.rules[index], specification));
      std::string line;
      while (std::getline(action_stream, line)) {
        output << "      " << line << '\n';
      }
    }
    output << "      break;\n"
           << "    }\n";
  }
  output << "    default:\n"
         << "      break;\n"
         << "  }\n"
         << "}\n\n"
         << "}  // namespace\n\n"
         << "bool yyparse(const std::vector<Token>& input_tokens) {\n"
         << "  std::vector<Token> tokens = input_tokens;\n"
         << "  if (tokens.empty() || tokens.back().type != 0) {\n"
         << "    tokens.push_back({0, \"\", 0, 0, {}});\n"
         << "  }\n"
         << "  std::vector<int> states = {0};\n"
         << "  std::vector<SemanticValue> semantic_stack;\n"
         << "  RuntimeSymbolTable symbol_table;\n"
         << "  std::size_t index = 0;\n"
         << "  while (index < tokens.size()) {\n"
         << "    const int state = states.back();\n"
         << "    const int token = tokens[index].type;\n"
         << "    const auto action_it = kAction[static_cast<std::size_t>(state)].find(token);\n"
         << "    if (action_it == kAction[static_cast<std::size_t>(state)].end() || action_it->second == \"err\") {\n"
         << "      return false;\n"
         << "    }\n"
         << "    const std::string& action = action_it->second;\n"
         << "    if (action == \"acc\") {\n"
         << "      return true;\n"
         << "    }\n"
         << "    if (!action.empty() && action[0] == 's') {\n"
         << "      const int next_state = std::stoi(action.substr(1));\n"
         << "      states.push_back(next_state);\n"
         << "      if (token == '{') symbol_table.enterScope();\n"
         << "      if (token == '}') symbol_table.exitScope();\n"
         << "      SemanticValue shifted;\n"
         << "      shifted.symbol = tokenSymbol(token);\n"
         << "      shifted.lexeme = tokens[index].lexeme;\n"
         << "      shifted.value = tokens[index].semantic;\n";
  if (!has_semantic_union) {
    output << "      if (shifted.value.lexeme.empty()) {\n"
           << "        shifted.value.lexeme = tokens[index].lexeme;\n"
           << "      }\n";
  }
  output << "      semantic_stack.push_back(shifted);\n"
         << "      ++index;\n"
         << "      continue;\n"
         << "    }\n"
         << "    if (!action.empty() && action[0] == 'r') {\n"
         << "      const int production = std::stoi(action.substr(1));\n"
         << "      const int pop_count = kProductionSize[static_cast<std::size_t>(production)];\n"
         << "      std::vector<SemanticValue> rhs(static_cast<std::size_t>(pop_count));\n"
         << "      for (int offset = pop_count - 1; offset >= 0; --offset) {\n"
         << "        rhs[static_cast<std::size_t>(offset)] = semantic_stack.back();\n"
         << "        semantic_stack.pop_back();\n"
         << "        states.pop_back();\n"
         << "      }\n"
         << "      observeReduction(production, rhs, &symbol_table);\n"
         << "      YYSTYPE yyval{};\n"
         << "      executeAction(production, rhs, &yyval);\n"
         << "      SemanticValue lhs_value;\n"
         << "      lhs_value.symbol = kProductionLeft[static_cast<std::size_t>(production)];\n"
         << "      lhs_value.value = yyval;\n"
         << "      for (const SemanticValue& value : rhs) {\n"
         << "        if (!value.lexeme.empty()) {\n"
         << "          lhs_value.lexeme = value.lexeme;\n"
         << "          break;\n"
         << "        }\n"
         << "      }\n";
  if (!has_semantic_union) {
    output << "      if (lhs_value.value.lexeme.empty()) {\n"
           << "        lhs_value.value.lexeme = lhs_value.lexeme;\n"
           << "      }\n";
  }
  output << "      const int goto_state = kGoto[static_cast<std::size_t>(states.back())].at(lhs_value.symbol);\n"
         << "      states.push_back(goto_state);\n"
         << "      semantic_stack.push_back(lhs_value);\n"
         << "      continue;\n"
         << "    }\n"
         << "    return false;\n"
         << "  }\n"
         << "  return false;\n"
         << "}\n\n"
         << "}  // namespace " << name_space << "\n";
}

void SeuYaccDriver::generate(const std::string& yacc_path,
                             const std::string& out_cpp_path,
                             const std::string& out_header_path,
                             const std::string& mode) const {
  SymbolTableManager symbols;
  symbols.reset();

  YaccParser parser;
  LR1Builder lr1_builder;
  ParseTableBuilder table_builder;
  ParserCodeGenerator generator;

  YaccSpecification specification = parser.parseYaccFile(yacc_path);
  for (const std::string& token : specification.tokenOrder) {
    symbols.registerTerminal(token);
  }
  for (const auto& entry : specification.tokenTypes) {
    symbols.setTokenType(entry.first, entry.second);
  }
  for (const YaccRule& rule : specification.rules) {
    symbols.registerNonterminal(rule.grammar.left);
  }
  for (const auto& entry : specification.nonterminalTypes) {
    symbols.setNonterminalType(entry.first, entry.second);
  }
  for (const YaccRule& rule : specification.rules) {
    for (const std::string& symbol : rule.grammar.right) {
      if (std::find(nonterminals.begin(), nonterminals.end(), symbol) == nonterminals.end()) {
        symbols.registerTerminal(symbol);
      }
    }
  }
  for (const PrecedenceDeclaration& declaration : specification.precedenceDeclarations) {
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
    symbols.addOperatorGroup(group, declaration.symbols);
  }
  for (const YaccRule& rule : specification.rules) {
    symbols.addProducer(rule.grammar);
  }
  symbols.setStartSymbol(specification.startSymbol);

  std::map<std::string, std::set<std::string>> first_sets;
  std::map<std::string, std::set<std::string>> follow_sets;

  std::vector<std::string> conflicts;
  if (mode == "lr1") {
    const LRPDA canonical =
        lr1_builder.buildCanonicalPDA(specification.startSymbol, &first_sets, &follow_sets);
    const auto table = table_builder.buildLR1Table(canonical, specification, specification.startSymbol, &conflicts);
    generator.emitParser(table, canonical, specification, specification.startSymbol, out_cpp_path, out_header_path);
    return;
  }
  const LRPDA lalr_automaton = lr1_builder.buildLALRPDA(specification.startSymbol, &first_sets, &follow_sets);
  const auto table =
      table_builder.buildLALRTable(lalr_automaton, specification, specification.startSymbol, &conflicts);
  generator.emitParser(table, lalr_automaton, specification, specification.startSymbol, out_cpp_path, out_header_path);
}

bool SeuYaccDriver::runSelfTests(const std::string& workspace_root) const {
  const std::string repo_root = resolveRepoRoot(workspace_root);
  const std::string temp_dir = makeTempDir();
  const std::string grammar_path = joinPath(temp_dir, "expr.y");
  const std::string parser_cpp = joinPath(temp_dir, "expr_parser.cpp");
  const std::string parser_h = joinPath(temp_dir, "expr_tokens.h");
  const std::string driver_cpp = joinPath(temp_dir, "expr_driver.cpp");
  const std::string driver_bin = joinPath(temp_dir, "expr_driver");
  const std::string action_grammar_path = joinPath(temp_dir, "semantic.y");
  const std::string action_parser_cpp = joinPath(joinPath(temp_dir, "generated"), "semantic_parser.cpp");
  const std::string action_parser_h = joinPath(joinPath(temp_dir, "include"), "semantic_tokens.h");
  const std::string action_driver_cpp = joinPath(temp_dir, "semantic_driver.cpp");
  const std::string action_driver_bin = joinPath(temp_dir, "semantic_driver");

  const std::string grammar =
      "%token ID\n"
      "%left '+'\n"
      "%left '*'\n"
      "%start S\n"
      "%%\n"
      "S : E ;\n"
      "E : E '+' E\n"
      "  | E '*' E\n"
      "  | ID\n"
      "  ;\n"
      "%%\n";

  const std::string semantic_grammar =
      "%{\n"
      "int result_value = 0;\n"
      "%}\n"
      "%union {\n"
      "  int ival;\n"
      "}\n"
      "%token <ival> NUM\n"
      "%type <ival> S E\n"
      "%start S\n"
      "%%\n"
      "S : E { result_value = $<ival>1; $<ival>$ = $<ival>1; } ;\n"
      "E : NUM { $<ival>$ = $<ival>1; }\n"
      "  | E '+' NUM { $<ival>$ = $<ival>1 + $<ival>3; }\n"
      "  ;\n"
      "%%\n"
      "int semantic_helper() { return result_value; }\n";

  {
    std::ofstream file(grammar_path);
    file << grammar;
  }
  {
    std::ofstream file(action_grammar_path);
    file << semantic_grammar;
  }

  SymbolTableManager symbols;
  symbols.reset();
  YaccParser parser;
  LR1Builder lr1_builder;
  LALRConverter lalr_converter;
  ParseTableBuilder table_builder;
  const YaccSpecification specification = parser.parseYaccFile(grammar_path);
  for (const std::string& token : specification.tokenOrder) {
    symbols.registerTerminal(token);
  }
  for (const YaccRule& rule : specification.rules) {
    symbols.registerNonterminal(rule.grammar.left);
  }
  for (const YaccRule& rule : specification.rules) {
    for (const std::string& symbol : rule.grammar.right) {
      if (std::find(nonterminals.begin(), nonterminals.end(), symbol) == nonterminals.end()) {
        symbols.registerTerminal(symbol);
      }
    }
  }
  for (const YaccRule& rule : specification.rules) {
    symbols.addProducer(rule.grammar);
  }
  symbols.setStartSymbol(specification.startSymbol);

  std::map<std::string, std::set<std::string>> first_sets;
  std::map<std::string, std::set<std::string>> follow_sets;
  const LRPDA canonical =
      lr1_builder.buildCanonicalPDA(specification.startSymbol, &first_sets, &follow_sets);
  const LALRResult merged = lalr_converter.convert(canonical);
  const LRPDA direct_lalr =
      lr1_builder.buildLALRPDA(specification.startSymbol, &first_sets, &follow_sets);
  const auto merged_table =
      table_builder.buildLALRTable(merged.automaton, specification, specification.startSymbol);
  const auto direct_table =
      table_builder.buildLALRTable(direct_lalr, specification, specification.startSymbol);
  if (merged_table.size() != direct_table.size()) {
    throw std::runtime_error("direct LALR construction diverges from LR(1)->LALR conversion on sample grammar");
  }

  generate(grammar_path, parser_cpp, parser_h, "lalr");

  const std::string generated_namespace = parserNamespace(parser_cpp);
  {
    std::ofstream driver(driver_cpp);
    driver << "#include <vector>\n"
           << "#include \"" << escapeCppString(parser_h) << "\"\n\n"
           << "int main() {\n"
           << "  std::vector<" << generated_namespace << "::Token> tokens = {\n"
           << "      {" << generated_namespace << "::ID, \"a\", 1, 1},\n"
           << "      {'+', \"+\", 1, 2},\n"
           << "      {" << generated_namespace << "::ID, \"b\", 1, 3},\n"
           << "      {'*', \"*\", 1, 4},\n"
           << "      {" << generated_namespace << "::ID, \"c\", 1, 5},\n"
           << "  };\n"
           << "  return " << generated_namespace << "::yyparse(tokens) ? 0 : 1;\n"
           << "}\n";
  }

  if (runProcess(kSelfTestCompiler,
                 {"-std=c++17", parser_cpp, driver_cpp, "-o", driver_bin}) != 0) {
    throw std::runtime_error("generated sample parser failed to compile");
  }
  if (runProcess(driver_bin, {}) != 0) {
    throw std::runtime_error("generated sample parser failed runtime validation");
  }

  generate(action_grammar_path, action_parser_cpp, action_parser_h, "lalr");
  const std::string action_namespace = parserNamespace(action_parser_cpp);
  {
    std::ofstream driver(action_driver_cpp);
    driver << "#include <vector>\n"
           << "#include \"" << escapeCppString(action_parser_h) << "\"\n\n"
           << "extern int semantic_helper();\n\n"
           << "int main() {\n"
           << "  " << action_namespace << "::Token one;\n"
           << "  one.type = " << action_namespace << "::NUM;\n"
           << "  one.lexeme = \"1\";\n"
           << "  one.line = 1;\n"
           << "  one.column = 1;\n"
           << "  one.semantic.ival = 1;\n"
           << "  " << action_namespace << "::Token two;\n"
           << "  two.type = " << action_namespace << "::NUM;\n"
           << "  two.lexeme = \"2\";\n"
           << "  two.line = 1;\n"
           << "  two.column = 3;\n"
           << "  two.semantic.ival = 2;\n"
           << "  std::vector<" << action_namespace << "::Token> tokens = {\n"
           << "      one,\n"
           << "      {'+', \"+\", 1, 2},\n"
           << "      two,\n"
           << "  };\n"
           << "  if (!" << action_namespace << "::yyparse(tokens)) {\n"
           << "    return 1;\n"
           << "  }\n"
           << "  return semantic_helper() == 3 ? 0 : 1;\n"
           << "}\n";
  }
  if (runProcess(kSelfTestCompiler,
                 {"-std=c++17", action_parser_cpp, action_driver_cpp, "-o", action_driver_bin}) != 0) {
    throw std::runtime_error("generated semantic parser failed to compile");
  }
  if (runProcess(action_driver_bin, {}) != 0) {
    throw std::runtime_error("generated semantic parser failed runtime validation");
  }

  const std::string minic_parser_cpp = joinPath(temp_dir, "minic_parser.cpp");
  const std::string minic_parser_h = joinPath(temp_dir, "minic_tokens.h");
  const std::string minic_parser_obj = joinPath(temp_dir, "minic_parser.o");
  generate(joinPath(repo_root, "resources/minic.y"), minic_parser_cpp, minic_parser_h, "lalr");
  if (runProcess(kSelfTestCompiler, {"-std=c++17", "-c", minic_parser_cpp, "-o", minic_parser_obj}) != 0) {
    throw std::runtime_error("generated minic parser failed to compile");
  }

  char original_cwd[4096];
  if (::getcwd(original_cwd, sizeof(original_cwd)) == nullptr) {
    throw std::runtime_error("failed to snapshot current working directory");
  }
  if (::chdir(temp_dir.c_str()) != 0) {
    throw std::runtime_error("failed to enter temporary directory for relative include self-test");
  }
  try {
    const std::string mixed_parser_cpp = "relative_parser.cpp";
    const std::string mixed_parser_h = joinPath(temp_dir, "relative_tokens.h");
    const std::string mixed_parser_obj = joinPath(temp_dir, "relative_parser.o");
    generate(grammar_path, mixed_parser_cpp, mixed_parser_h, "lalr");
    if (runProcess(kSelfTestCompiler, {"-std=c++17", "-c", mixed_parser_cpp, "-o", mixed_parser_obj}) != 0) {
      throw std::runtime_error("generated parser with relative cpp / absolute header failed to compile");
    }

    const std::string mirrored_parser_cpp = joinPath(temp_dir, "absolute_parser.cpp");
    const std::string mirrored_parser_h = "absolute_tokens.h";
    const std::string mirrored_parser_obj = joinPath(temp_dir, "absolute_parser.o");
    generate(grammar_path, mirrored_parser_cpp, mirrored_parser_h, "lalr");
    if (runProcess(kSelfTestCompiler, {"-std=c++17", "-c", mirrored_parser_cpp, "-o", mirrored_parser_obj}) != 0) {
      throw std::runtime_error("generated parser with absolute cpp / relative header failed to compile");
    }
  } catch (...) {
    ::chdir(original_cwd);
    throw;
  }
  if (::chdir(original_cwd) != 0) {
    throw std::runtime_error("failed to restore working directory after relative include self-test");
  }

  std::cout << "[self-test] generated sample parser: " << parser_cpp << '\n';
  std::cout << "[self-test] generated semantic parser: " << action_parser_cpp << '\n';
  std::cout << "[self-test] generated minic parser: " << minic_parser_cpp << '\n';
  return true;
}

}  // namespace seu_yacc
