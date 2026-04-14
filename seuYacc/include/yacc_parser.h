#pragma once

#include <map>
#include <cstddef>
#include <string>
#include <vector>

/**
 * @file yacc_parser.h
 * @brief Yacc source parser interfaces and report-defined grammar structures.
 */

namespace seu_yacc {

/**
 * @brief Report-defined operator precedence group.
 *
 * `rl` stores associativity as a C-style string to match the report.
 */
typedef struct operators {
  std::vector<char> op;
  char* rl = nullptr;
  int level = 0;
} LROP;

/**
 * @brief Report-defined production structure.
 */
typedef struct produce {
  std::string left;
  std::vector<std::string> right;
} producer;

/**
 * @brief One parsed Yacc alternative with attached metadata.
 */
struct YaccRule {
  produce grammar;
  std::string action;
  std::string precedence_symbol;
  int source_line = 0;
};

/**
 * @brief One precedence/associativity declaration group.
 */
struct PrecedenceDeclaration {
  std::string associativity;
  int level = 0;
  std::vector<std::string> symbols;
};

/**
 * @brief Parsed Yacc specification split into three sections.
 */
struct YaccSpecification {
  std::string definitionsSection;
  std::string verbatimDefinitions;
  std::string semanticUnion;
  std::string rulesSection;
  std::string userSubroutines;
  std::string startSymbol;
  std::vector<std::string> tokenOrder;
  std::vector<PrecedenceDeclaration> precedenceDeclarations;
  std::map<std::string, std::string> tokenTypes;
  std::map<std::string, std::string> nonterminalTypes;
  std::vector<YaccRule> rules;
};

/**
 * @brief Parse `.y` input files into structured grammar data.
 */
class YaccParser {
 public:
  /**
   * @brief Parse a Yacc source file.
   *
   * Complexity: O(N), where N is the file size.
   */
  YaccSpecification parseYaccFile(const std::string& path) const;
};

}  // namespace seu_yacc
