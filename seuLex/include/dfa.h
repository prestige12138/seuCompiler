#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "node.h"

/**
 * @file dfa.h
 * @brief 中期报告规定的 DFA 结构与共享 DFA 阶段表。
 */

namespace seu_lex {

/**
 * @brief 中期报告规定的 DFA 聚合结构。
 */
typedef struct dfa {
  node* start;
  std::vector<node> nodeVec;
  std::vector<node> endNode;

  /**
   * @brief 用给定起始结点构造 DFA。
   *
   * 复杂度：O(1)。
   */
  dfa(node* st = nullptr);

  /**
   * @brief 对 NFA 状态集合求 epsilon 闭包。
   *
   * 复杂度：在可达 epsilon 子图上为 O(V + E)。
   */
  void Eclosure(std::set<node*>& x);

  /**
   * @brief 打印 DFA 转移，供调试使用。
   *
   * 复杂度：O(V + E)。
   */
  void printDFA();
} dfa;

/**
 * @brief 确定化或最小化之后的 DFA 终态集合。
 */
extern std::vector<node*> dfaterminals;

/**
 * @brief 最小化前当前 DFA 的动作表。
 */
extern std::map<int, std::string> TerStateActionTable;

/**
 * @brief 最小化后供代码生成使用的动作表。
 */
extern std::map<int, std::string> mindfareturn;

}  // 命名空间 seu_lex
