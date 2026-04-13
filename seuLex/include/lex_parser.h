#pragma once

#include <cstddef>
#include <string>
#include <vector>

/**
 * @file lex_parser.h
 * @brief Lex source parser interfaces.
 */

namespace seu_lex {

/**
 * @brief One Lex rule.
 */
struct LexRule {
  std::string regex;
  std::string action;
  std::size_t priority = 0;
  std::string expandedRegex;
  std::string postfixRegex;
};

/**
 * @brief Parsed Lex specification split into three sections.
 */
struct LexSpecification {
  std::string definitionsSection;
  std::string verbatimDefinitions;
  std::string rulesSection;
  std::string userSubroutines;
  std::vector<LexRule> rules;
};

/**
 * @brief Parse Lex source files into structured sections and rules.
 */
class LexParser {
 public:
  /**
   * @brief Parse a Lex source file.
   *
   * Complexity: O(N), where N is the file size.
   */
  LexSpecification parseLexFile(const std::string& path) const;
};

}  // namespace seu_lex
