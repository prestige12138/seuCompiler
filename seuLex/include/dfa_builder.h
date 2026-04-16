#pragma once

#include "dfa.h"
#include "nfa.h"

/**
 * @file dfa_builder.h
 * @brief 确定有限自动机子集构造接口。
 */

namespace seu_lex {

/**
 * @brief 使用子集构造法将 NFA 确定化。
 */
class DFABuilder {
 public:
  /**
   * @brief 从 NFA 构造 DFA。
   *
   * 复杂度：最坏情况下为 O(2^V * |Sigma|)。
   */
  dfa subsetConstruct(const nfa& automaton) const;
};

}  // 命名空间 seu_lex
