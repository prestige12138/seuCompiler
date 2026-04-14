#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "dfa_builder.h"
#include "nfa.h"
#include "regex_expander.h"

/**
 * @file nfa_constructor.h
 * @brief Thompson NFA construction and shared generation-state reset.
 *
 * This header keeps the historical include surface stable by re-exporting the
 * split regex-expansion and DFA-construction interfaces.
 */

namespace seu_lex {

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
 * @brief Reset all report-defined global tables and construction state.
 *
 * Complexity: O(S), where S is the total size of stored global state.
 *
 * @note All previously returned NFA-state pointers become invalid after reset.
 */
void resetGlobalTables();

}  // namespace seu_lex
