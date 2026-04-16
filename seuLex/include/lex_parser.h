#pragma once

#include <cstddef>
#include <string>
#include <vector>

/**
 * @file lex_parser.h
 * @brief 词法规范源文件解析接口。
 */

namespace seu_lex {

/**
 * @brief 一条 Lex 规则。
 */
struct LexRule {
  std::string regex;
  std::string action;
  std::size_t priority = 0;
  std::string expandedRegex;
  std::string postfixRegex;
};

/**
 * @brief 按三段结构拆分后的 Lex 规格。
 */
struct LexSpecification {
  std::string definitionsSection;
  std::string verbatimDefinitions;
  std::string rulesSection;
  std::string userSubroutines;
  std::vector<LexRule> rules;
};

/**
 * @brief 将 Lex 源文件解析为结构化三段与规则集合。
 */
class LexParser {
 public:
  /**
   * @brief 解析一个 Lex 源文件。
   *
   * 复杂度：O(N)，其中 N 为文件大小。
   */
  LexSpecification parseLexFile(const std::string& path) const;
};

}  // 命名空间 seu_lex
