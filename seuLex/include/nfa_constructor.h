#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "dfa.h"
#include "lex_parser.h"
#include "nfa.h"

/**
 * @file nfa_constructor.h
 * @brief Regex expansion, NFA construction, and subset construction.
 */

namespace seu_lex {

/**
 * @brief Expand extended Lex regular expressions into ordinary RE tokens.
 */
class REExpander {
 public:
  /**
   * @brief Expand one extended RE using `idreTable`.
   *
   * Complexity: O(M + K), where M is input length and K is normalized output.
   */
  std::string expandRE(const std::string& raw) const;
};

/**
 * @brief Convert normalized RE to postfix and Thompson NFA.
 */
class NFABuilder {
 public:
  /**
   * @brief Convert infix RE with explicit concatenation to postfix.
   *
   * Complexity: O(T), where T is the number of tokens.
   */
  std::string toPostfix(const std::string& infix) const;

  /**
   * @brief Build one NFA from one postfix RE.
   *
   * Complexity: O(T + E).
   */
  nfa buildNFA(const std::string& postfix,
               const std::string& action,
               std::size_t priority) const;

  /**
   * @brief Merge rule NFAs under a fresh epsilon start node.
   *
   * Complexity: O(R), where R is the number of input NFAs.
   */
  nfa mergeNFA(const std::vector<nfa>& automata) const;
};

/**
 * @brief Determinize an NFA with subset construction.
 */
class DFABuilder {
 public:
  /**
   * @brief Build a DFA from an NFA.
   *
   * Complexity: O(2^V * |Sigma|) in the worst case.
   */
  dfa subsetConstruct(const nfa& automaton) const;
};

/**
 * @brief Reset all report-defined global tables and construction state.
 *
 * Complexity: O(S), where S is the total size of stored global state.
 */
void resetGlobalTables();

}  // namespace seu_lex
