#pragma once

#include "dfa.h"

/**
 * @file dfa_minimizer.h
 * @brief DFA minimization interface.
 */

namespace seu_lex {

/**
 * @brief Minimize a DFA by partition refinement.
 */
class DFAMinimizer {
 public:
  /**
   * @brief Minimize a DFA while preserving accepting actions.
   *
   * Complexity: O(P * V * |Sigma|), where P is the refinement round count.
   */
  dfa minimizeDFA(const dfa& automaton) const;
};

}  // namespace seu_lex
