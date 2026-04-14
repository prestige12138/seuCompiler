#pragma once

#include "dfa.h"
#include "nfa.h"

/**
 * @file dfa_builder.h
 * @brief DFA subset-construction interface.
 */

namespace seu_lex {

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

}  // namespace seu_lex
