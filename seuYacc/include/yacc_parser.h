#pragma once

#include <map>
#include <cstddef>
#include <string>
#include <vector>

/**
 * @file yacc_parser.h
 * @brief 定义 Yacc 源文件解析接口以及中期报告要求的文法数据结构。
 */

namespace seu_yacc {

/**
 * @brief 中期报告中定义的运算符优先级分组结构。
 *
 * `rl` 以 C 风格字符串保存结合性，保持与报告定义完全一致。
 */
typedef struct operators {
  std::vector<char> op;
  char* rl = nullptr;
  int level = 0;
} LROP;

/**
 * @brief 中期报告中定义的产生式结构。
 */
typedef struct produce {
  std::string left;
  std::vector<std::string> right;
} producer;

/**
 * @brief 一条已经解析完成的 Yacc 规则分支及其附加信息。
 */
struct YaccRule {
  produce grammar;
  std::string action;
  std::string precedence_symbol;
  int source_line = 0;
};

/**
 * @brief 一个优先级与结合性声明分组。
 */
struct PrecedenceDeclaration {
  std::string associativity;
  int level = 0;
  std::vector<std::string> symbols;
};

/**
 * @brief 将 `.y` 文件按定义段、规则段、用户代码段拆解后的完整结果。
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
 * @brief 负责把 `.y` 输入文件解析为结构化文法数据。
 */
class YaccParser {
 public:
  /**
   * @brief 解析一个 Yacc 源文件。
   *
   * 时间复杂度：O(N)，其中 N 为文件长度。
   */
  YaccSpecification parseYaccFile(const std::string& path) const;
};

}  // 命名空间 seu_yacc
