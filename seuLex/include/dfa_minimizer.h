#pragma once

#include "dfa.h"

/**
 * @file dfa_minimizer.h
 * @brief 确定有限自动机最小化接口。
 */

namespace seu_lex {

/**
 * @brief 使用划分细化法最小化 DFA。
 */
class DFAMinimizer {
 public:
  /**
   * @brief 在保持接受动作不变的前提下最小化 DFA。
   *
   * 复杂度：O(P * V * |Sigma|)，其中 P 为划分细化轮数。
   */
  dfa minimizeDFA(const dfa& automaton) const;
};

}  // 命名空间 seu_lex
