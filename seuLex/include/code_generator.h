#pragma once

#include <string>

#include "dfa.h"
#include "lex_parser.h"
#include "nfa.h"

/**
 * @file code_generator.h
 * @brief Lexer emission, visualization, and orchestration interfaces.
 */

namespace seu_lex {

/**
 * @brief Emit a standalone lexer C++ source file.
 */
class CodeGenerator {
 public:
  /**
   * @brief Emit lexer source code from a minimized DFA.
   *
   * Complexity: O(V * |Sigma| + A), where A is emitted action size.
   */
  void emitLexer(const dfa& automaton,
                 const LexSpecification& specification,
                 const std::string& outPath,
                 const std::string& tokenHeaderPath = "") const;
};

/**
 * @brief Export automata visualizations in dot format.
 */
class Visualizer {
 public:
  /**
   * @brief Dump NFA to Graphviz dot.
   *
   * Complexity: O(V + E).
   */
  void dumpNFA(const nfa& automaton, const std::string& path) const;

  /**
   * @brief Dump DFA to Graphviz dot.
   *
   * Complexity: O(V + E).
   */
  void dumpDFA(const dfa& automaton, const std::string& path) const;
};

/**
 * @brief End-to-end seuLex orchestration facade.
 */
class SeuLexDriver {
 public:
  /**
   * @brief Run the full seuLex generation pipeline.
   *
   * Complexity: dominated by determinization and minimization.
   */
  void generate(const std::string& lexPath,
                const std::string& outCppPath,
                const std::string& dotDir,
                const std::string& tokenHeaderPath = "") const;

  /**
   * @brief Run built-in smoke tests.
   *
   * Complexity: proportional to the embedded test workload.
   */
  bool runSelfTests(const std::string& workspaceRoot) const;
};

}  // namespace seu_lex
