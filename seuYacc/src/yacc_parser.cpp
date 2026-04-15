/**
 * @file yacc_parser.cpp
 * @brief Yacc three-section parsing, directive extraction, and production
 *        normalization implementation.
 */

#include "yacc_parser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

namespace seu_yacc {
namespace {

std::string readWholeFile(const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open file: " + path);
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

std::string trim(const std::string& input) {
  const auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char ch) {
    return std::isspace(ch) != 0;
  });
  if (begin == input.end()) {
    return "";
  }
  const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char ch) {
    return std::isspace(ch) != 0;
  }).base();
  return std::string(begin, end);
}

std::size_t findSectionDelimiter(const std::string& content, std::size_t start_pos) {
  bool in_verbatim = false;
  bool line_start = true;
  for (std::size_t index = start_pos; index + 1 < content.size(); ++index) {
    // `%{ ... %}` may legally contain `%%`, so only top-level markers split
    // the Definitions / Rules / User subroutines sections.
    if (line_start) {
      std::size_t marker = index;
      while (marker < content.size() &&
             (content[marker] == ' ' || content[marker] == '\t' || content[marker] == '\r')) {
        ++marker;
      }
      if (!in_verbatim && marker + 1 < content.size() && content[marker] == '%' &&
          content[marker + 1] == '%') {
        std::size_t tail = marker + 2;
        while (tail < content.size() &&
               (content[tail] == ' ' || content[tail] == '\t' || content[tail] == '\r')) {
          ++tail;
        }
        if (tail == content.size() || content[tail] == '\n') {
          return marker;
        }
      }
    }
    if (!in_verbatim && content[index] == '%' && content[index + 1] == '{') {
      in_verbatim = true;
      ++index;
      line_start = false;
      continue;
    }
    if (in_verbatim && content[index] == '%' && content[index + 1] == '}') {
      in_verbatim = false;
      ++index;
      line_start = false;
      continue;
    }
    line_start = content[index] == '\n';
  }
  return std::string::npos;
}

std::string takeVerbatimDefinitions(const std::string& text,
                                    std::vector<std::pair<std::size_t, std::size_t>>* ranges) {
  std::ostringstream extracted;
  std::size_t search_pos = 0;
  while (true) {
    const std::size_t begin = text.find("%{", search_pos);
    if (begin == std::string::npos) {
      break;
    }
    const std::size_t end = text.find("%}", begin + 2);
    if (end == std::string::npos) {
      throw std::runtime_error("unterminated %{ %} block in Yacc definitions");
    }
    extracted << text.substr(begin + 2, end - begin - 2) << '\n';
    if (ranges != nullptr) {
      ranges->push_back({begin, end + 2});
    }
    search_pos = end + 2;
  }
  return extracted.str();
}

std::string removeRanges(const std::string& text,
                         const std::vector<std::pair<std::size_t, std::size_t>>& ranges) {
  std::ostringstream oss;
  std::size_t cursor = 0;
  for (const auto& range : ranges) {
    if (cursor < range.first) {
      oss << text.substr(cursor, range.first - cursor);
    }
    cursor = range.second;
  }
  if (cursor < text.size()) {
    oss << text.substr(cursor);
  }
  return oss.str();
}

std::vector<std::string> splitByLines(const std::string& text) {
  std::vector<std::string> lines;
  std::stringstream ss(text);
  std::string line;
  while (std::getline(ss, line)) {
    lines.push_back(line);
  }
  if (!text.empty() && text.back() == '\n') {
    lines.emplace_back();
  }
  return lines;
}

std::string parseQuotedToken(const std::string& text, std::size_t* pos) {
  const char quote = text[*pos];
  std::ostringstream oss;
  oss << quote;
  ++(*pos);
  bool escaping = false;
  while (*pos < text.size()) {
    const char ch = text[*pos];
    oss << ch;
    ++(*pos);
    if (escaping) {
      escaping = false;
      continue;
    }
    if (ch == '\\') {
      escaping = true;
      continue;
    }
    if (ch == quote) {
      return oss.str();
    }
  }
  throw std::runtime_error("unterminated quoted token in grammar section");
}

std::string parseActionBlock(const std::string& text, std::size_t* pos) {
  if (text[*pos] != '{') {
    throw std::runtime_error("expected action block");
  }
  std::ostringstream oss;
  int depth = 0;
  bool in_string = false;
  bool in_char = false;
  bool in_line_comment = false;
  bool in_block_comment = false;
  bool escaping = false;
  while (*pos < text.size()) {
    const char ch = text[*pos];
    oss << ch;
    ++(*pos);
    if (escaping) {
      escaping = false;
      continue;
    }
    if (in_line_comment) {
      if (ch == '\n') {
        in_line_comment = false;
      }
      continue;
    }
    if (in_block_comment) {
      if (ch == '*' && *pos < text.size() && text[*pos] == '/') {
        oss << '/';
        ++(*pos);
        in_block_comment = false;
      }
      continue;
    }
    if ((in_string || in_char) && ch == '\\') {
      escaping = true;
      continue;
    }
    if (!in_char && ch == '"') {
      in_string = !in_string;
      continue;
    }
    if (!in_string && ch == '\'') {
      in_char = !in_char;
      continue;
    }
    if (in_string || in_char) {
      continue;
    }
    if (ch == '/' && *pos < text.size()) {
      if (text[*pos] == '/') {
        oss << '/';
        ++(*pos);
        in_line_comment = true;
        continue;
      }
      if (text[*pos] == '*') {
        oss << '*';
        ++(*pos);
        in_block_comment = true;
        continue;
      }
    }
    if (ch == '{') {
      ++depth;
    } else if (ch == '}') {
      --depth;
      if (depth == 0) {
        return oss.str();
      }
    }
  }
  throw std::runtime_error("unterminated action block in grammar section");
}

void skipSpaceAndComments(const std::string& text, std::size_t* pos, int* line) {
  while (*pos < text.size()) {
    const char ch = text[*pos];
    if (std::isspace(static_cast<unsigned char>(ch)) != 0) {
      if (ch == '\n' && line != nullptr) {
        ++(*line);
      }
      ++(*pos);
      continue;
    }
    if (ch == '/' && *pos + 1 < text.size()) {
      if (text[*pos + 1] == '/') {
        *pos += 2;
        while (*pos < text.size() && text[*pos] != '\n') {
          ++(*pos);
        }
        continue;
      }
      if (text[*pos + 1] == '*') {
        *pos += 2;
        while (*pos + 1 < text.size() && !(text[*pos] == '*' && text[*pos + 1] == '/')) {
          if (text[*pos] == '\n' && line != nullptr) {
            ++(*line);
          }
          ++(*pos);
        }
        if (*pos + 1 >= text.size()) {
          throw std::runtime_error("unterminated comment in grammar section");
        }
        *pos += 2;
        continue;
      }
    }
    break;
  }
}

std::string parseIdentifier(const std::string& text, std::size_t* pos) {
  if (*pos >= text.size()) {
    return "";
  }
  const unsigned char first = static_cast<unsigned char>(text[*pos]);
  if (std::isalpha(first) == 0 && text[*pos] != '_' && text[*pos] != '%') {
    return "";
  }
  std::size_t begin = *pos;
  ++(*pos);
  while (*pos < text.size()) {
    const unsigned char ch = static_cast<unsigned char>(text[*pos]);
    if (std::isalnum(ch) == 0 && text[*pos] != '_' && text[*pos] != '%') {
      break;
    }
    ++(*pos);
  }
  return text.substr(begin, *pos - begin);
}

std::string parseGrammarSymbol(const std::string& text, std::size_t* pos) {
  if (*pos >= text.size()) {
    return "";
  }
  if (text[*pos] == '\'' || text[*pos] == '"') {
    return parseQuotedToken(text, pos);
  }
  return parseIdentifier(text, pos);
}

std::string parseUnionBlock(const std::string& text, std::size_t* pos) {
  if (*pos >= text.size() || text[*pos] != '{') {
    return "";
  }
  return parseActionBlock(text, pos);
}

void parseDefinitionLines(const std::string& text, YaccSpecification* spec) {
  std::string normalized_text = text;
  const std::size_t union_pos = normalized_text.find("%union");
  if (union_pos != std::string::npos) {
    const std::size_t brace = normalized_text.find('{', union_pos);
    if (brace == std::string::npos) {
      throw std::runtime_error("missing '{' after %union declaration");
    }
    std::size_t pos = brace;
    spec->semanticUnion = parseUnionBlock(normalized_text, &pos);
    normalized_text.erase(union_pos, pos - union_pos);
  }

  int precedence_level = 1;
  for (const std::string& raw_line : splitByLines(normalized_text)) {
    const std::string line = trim(raw_line);
    if (line.empty()) {
      continue;
    }
    std::istringstream iss(line);
    std::string directive;
    iss >> directive;
    if (directive == "%start") {
      iss >> spec->startSymbol;
      continue;
    }
    if (directive != "%token" && directive != "%left" && directive != "%right" &&
        directive != "%nonassoc" && directive != "%type") {
      continue;
    }

    std::string semantic_type;
    std::vector<std::string> symbols;
    while (iss) {
      std::string token;
      iss >> token;
      if (token.empty()) {
        continue;
      }
      if (!token.empty() && token.front() == '<' && token.back() == '>') {
        semantic_type = token.substr(1, token.size() - 2);
        continue;
      }
      symbols.push_back(token);
    }

    if (directive == "%left" || directive == "%right" || directive == "%nonassoc") {
      PrecedenceDeclaration decl;
      decl.associativity = directive == "%left" ? "left" : (directive == "%right" ? "right" : "nonassoc");
      decl.level = precedence_level++;
      decl.symbols = symbols;
      spec->precedenceDeclarations.push_back(decl);
      continue;
    }

    if (directive == "%token" || directive == "%type") {
      for (const std::string& symbol : symbols) {
        if (directive == "%token") {
          spec->tokenOrder.push_back(symbol);
          if (!semantic_type.empty()) {
            spec->tokenTypes[symbol] = semantic_type;
          }
        } else if (!semantic_type.empty()) {
          spec->nonterminalTypes[symbol] = semantic_type;
        }
      }
      continue;
    }
  }
}

std::vector<YaccRule> parseRules(const std::string& text) {
  std::vector<YaccRule> rules;
  std::size_t pos = 0;
  int line = 1;
  int midrule_index = 0;
  while (pos < text.size()) {
    skipSpaceAndComments(text, &pos, &line);
    if (pos >= text.size()) {
      break;
    }
    const int rule_line = line;
    const std::string left = parseIdentifier(text, &pos);
    if (left.empty()) {
      throw std::runtime_error("expected nonterminal on left-hand side of production");
    }
    skipSpaceAndComments(text, &pos, &line);
    if (pos >= text.size() || text[pos] != ':') {
      throw std::runtime_error("expected ':' after nonterminal " + left);
    }
    ++pos;
    while (true) {
      YaccRule rule;
      rule.grammar.left = left;
      rule.source_line = rule_line;
      while (true) {
        skipSpaceAndComments(text, &pos, &line);
        if (pos >= text.size()) {
          throw std::runtime_error("unterminated production for " + left);
        }
        if (text[pos] == '{') {
          const std::string action = parseActionBlock(text, &pos);
          std::size_t lookahead_pos = pos;
          int lookahead_line = line;
          skipSpaceAndComments(text, &lookahead_pos, &lookahead_line);
          const bool is_final_action = lookahead_pos >= text.size() || text[lookahead_pos] == '|' ||
                                       text[lookahead_pos] == ';';
          if (is_final_action) {
            rule.action = action;
            continue;
          }

          // Mid-rule actions are lowered into synthetic nonterminals so later
          // LR construction only needs ordinary productions plus final actions.
          YaccRule synthetic;
          synthetic.grammar.left =
              "__midrule_" + left + "_" + std::to_string(midrule_index++);
          synthetic.source_line = line;
          synthetic.action = action;
          rules.push_back(synthetic);
          rule.grammar.right.push_back(synthetic.grammar.left);
          continue;
        }
        if (text[pos] == '|') {
          ++pos;
          rules.push_back(rule);
          break;
        }
        if (text[pos] == ';') {
          ++pos;
          rules.push_back(rule);
          goto next_rule;
        }
        if (text.compare(pos, 5, "%prec") == 0 &&
            (pos + 5 == text.size() ||
             std::isspace(static_cast<unsigned char>(text[pos + 5])) != 0)) {
          pos += 5;
          skipSpaceAndComments(text, &pos, &line);
          rule.precedence_symbol = parseGrammarSymbol(text, &pos);
          continue;
        }
        const std::string symbol = parseGrammarSymbol(text, &pos);
        if (symbol.empty()) {
          throw std::runtime_error("unexpected token while parsing production of " + left);
        }
        rule.grammar.right.push_back(symbol);
      }
    }
  next_rule:
    continue;
  }
  return rules;
}

}  // namespace

YaccSpecification YaccParser::parseYaccFile(const std::string& path) const {
  YaccSpecification spec;
  const std::string content = readWholeFile(path);
  const std::size_t first = findSectionDelimiter(content, 0);
  if (first == std::string::npos) {
    throw std::runtime_error("missing first %% section delimiter");
  }
  const std::size_t second = findSectionDelimiter(content, first + 2);
  if (second == std::string::npos) {
    throw std::runtime_error("missing second %% section delimiter");
  }

  spec.definitionsSection = content.substr(0, first);
  spec.rulesSection = content.substr(first + 2, second - (first + 2));
  spec.userSubroutines = content.substr(second + 2);

  // Definition directives are parsed after verbatim blocks are removed from the
  // normalized view, but the verbatim text itself is still preserved for code
  // generation.
  std::vector<std::pair<std::size_t, std::size_t>> verbatim_ranges;
  spec.verbatimDefinitions = takeVerbatimDefinitions(spec.definitionsSection, &verbatim_ranges);
  const std::string normalized_definitions = removeRanges(spec.definitionsSection, verbatim_ranges);
  parseDefinitionLines(normalized_definitions, &spec);
  spec.rules = parseRules(spec.rulesSection);
  if (spec.startSymbol.empty()) {
    if (spec.rules.empty()) {
      throw std::runtime_error("grammar contains no productions");
    }
    spec.startSymbol = spec.rules.front().grammar.left;
  }
  return spec;
}

}  // namespace seu_yacc
