#pragma once

#include <cstddef>
#include <string>
#include <vector>

#include "dfa_builder.h"
#include "nfa.h"
#include "regex_expander.h"

/**
 * @file nfa_constructor.h
 * @brief 汤普森非确定有限自动机构造与共享生成状态重置接口。
 *
 * 该头文件保持历史 include 入口稳定，同时重新导出拆分后的 RE 展开与
 * 同时重新导出确定有限自动机构造接口。
 */

namespace seu_lex {

/**
 * @brief 将规范化 RE 转成后缀表达式并构造 Thompson NFA。
 */
class NFABuilder {
 public:
  /**
   * @brief 将显式拼接的中缀 RE 转成后缀 RE。
   *
   * 复杂度：O(T)，其中 T 为记号数。
   */
  std::string toPostfix(const std::string& infix) const;

  /**
   * @brief 根据一条后缀 RE 构造一张 NFA。
   *
   * 复杂度：O(T + E)。
   */
  nfa buildNFA(const std::string& postfix,
               const std::string& action,
               std::size_t priority) const;

  /**
   * @brief 在一个新的 epsilon 起点下合并多条规则的 NFA。
   *
   * 复杂度：O(R)，其中 R 为输入 NFA 数量。
   */
  nfa mergeNFA(const std::vector<nfa>& automata) const;
};

/**
 * @brief 重置所有报告规定的全局表与构造状态。
 *
 * 复杂度：O(S)，其中 S 为已存全局状态总大小。
 *
 * @note 重置后，之前返回的所有 NFA 状态指针都会失效。
 */
void resetGlobalTables();

}  // 命名空间 seu_lex
