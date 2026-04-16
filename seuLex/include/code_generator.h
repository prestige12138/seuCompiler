#pragma once

#include <string>

#include "dfa.h"
#include "lex_parser.h"
#include "nfa.h"

/**
 * @file code_generator.h
 * @brief 词法分析器生成、自动机可视化与总控接口。
 */

namespace seu_lex {

/**
 * @brief 输出独立可编译的词法分析器 C++ 源文件。
 */
class CodeGenerator {
 public:
  /**
   * @brief 根据最小化 DFA 生成词法分析器源码。
   *
   * 复杂度：O(V * |Sigma| + A)，其中 A 为动作代码输出总长度。
   */
  void emitLexer(const dfa& automaton,
                 const LexSpecification& specification,
                 const std::string& outPath,
                 const std::string& tokenHeaderPath = "") const;
};

/**
 * @brief 以 dot 格式导出自动机可视化结果。
 */
class Visualizer {
 public:
  /**
   * @brief 将 NFA 导出为 Graphviz dot 文件。
   *
   * 复杂度：O(V + E)。
   */
  void dumpNFA(const nfa& automaton, const std::string& path) const;

  /**
   * @brief 将 DFA 导出为 Graphviz dot 文件。
   *
   * 复杂度：O(V + E)。
   */
  void dumpDFA(const dfa& automaton, const std::string& path) const;
};

/**
 * @brief 词法分析模块端到端总控入口接口。
 */
class SeuLexDriver {
 public:
  /**
   * @brief 执行完整的 seuLex 生成流水线。
   *
   * 复杂度：主要由确定化与最小化阶段决定。
   */
  void generate(const std::string& lexPath,
                const std::string& outCppPath,
                const std::string& dotDir,
                const std::string& tokenHeaderPath = "") const;

  /**
   * @brief 运行内置自测。
   *
   * 复杂度：与内置测试工作量成正比。
   */
  bool runSelfTests(const std::string& workspaceRoot) const;
};

}  // 命名空间 seu_lex
