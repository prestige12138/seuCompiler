#pragma once

#include <vector>

#include "lr1_pda.h"

/**
 * @file lalr_converter.h
 * @brief LR(1) to LALR(1) state merging interfaces.
 */

namespace seu_yacc {

/**
 * @brief Result of LR(1) to LALR(1) conversion.
 */
struct LALRResult {
  LRPDA automaton;
  std::vector<int> old_to_new;
};

/**
 * @brief Merge LR(1) states with identical LR(0) cores.
 */
class LALRConverter {
 public:
  /**
   * @brief Convert a canonical LR(1) PDA into an LALR(1) PDA.
   *
   * Complexity: O(S * I log I), where S is state count and I is item count.
   */
  LALRResult convert(const LRPDA& canonical) const;
};

}  // namespace seu_yacc
