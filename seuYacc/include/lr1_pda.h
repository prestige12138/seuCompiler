#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "symbol_table.h"
#include "yacc_parser.h"

/**
 * @file lr1_pda.h
 * @brief 定义 FIRST/FOLLOW 集、LR(1) 项目集闭包以及 LR 自动机构造接口。
 */

namespace seu_yacc {

/**
 * @brief 中期报告中定义的 LR(1) 项结构。
 */
typedef struct ITEM {
  std::string left;
  std::vector<std::string> right;
  int dotpos = 0;
  std::string predict;
} LRItem;

/**
 * @brief 中期报告中定义的 LR 自动机状态结点。
 */
typedef struct node {
  int stateindex = 0;
  std::vector<ITEM> items;
  std::map<std::string, int> nextnode;
} LRnode;

/**
 * @brief 中期报告中定义的 LR 下推自动机结构。
 */
typedef struct PDA {
  std::vector<LRnode> nodes;
} LRPDA;

/**
 * @brief 负责构造 FIRST/FOLLOW 集、规范 LR(1) 自动机以及直接 LALR 自动机。
 */
class LR1Builder {
 public:
  /**
   * @brief 计算当前文法中所有符号的 FIRST 集。
   *
   * 时间复杂度：O(P * K * I)，其中 P 为产生式数量，K 为右部平均长度，
   * I 为迭代收敛轮数。
   */
  std::map<std::string, std::set<std::string>> computeFirstSets() const;

  /**
   * @brief 基于 FIRST 集计算当前文法的 FOLLOW 集。
   *
   * 时间复杂度：O(P * K * I)。
   */
  std::map<std::string, std::set<std::string>> computeFollowSets(
      const std::map<std::string, std::set<std::string>>& first_sets,
      const std::string& start_symbol) const;

  /**
   * @brief 计算一个核心项目集对应的 LR(1) 闭包。
   *
   * 时间复杂度：对可达闭包项目而言为 O(P * T)。
   */
  std::vector<ITEM> closure(
      const std::vector<ITEM>& kernel,
      const std::map<std::string, std::set<std::string>>& first_sets,
      const std::string& start_symbol) const;

  /**
   * @brief 计算 GOTO(items, symbol) 转移结果。
   *
   * 时间复杂度：O(I + C)，其中 I 为输入项目数，C 为闭包计算代价。
   */
  std::vector<ITEM> gotoSet(
      const std::vector<ITEM>& items,
      const std::string& symbol,
      const std::map<std::string, std::set<std::string>>& first_sets,
      const std::string& start_symbol) const;

  /**
   * @brief 直接在 LR(0) 核上迭代传播展望符，构造 LALR(1) 自动机。
   *
   * 时间复杂度：最坏情况下每轮传播为 O(S * I * L * F)，其中
   * S 为 LR(0) 状态数，I 为每个状态中的项目数，L 为展望符数量，
   * F 为 FIRST 串计算代价。
   */
  LRPDA buildLALRPDA(
      const std::string& start_symbol,
      std::map<std::string, std::set<std::string>>* first_sets = nullptr,
      std::map<std::string, std::set<std::string>>* follow_sets = nullptr) const;

  /**
   * @brief 构造规范 LR(1) 项目集自动机。
   *
   * 时间复杂度：最坏情况下呈指数级。
   */
  LRPDA buildCanonicalPDA(
      const std::string& start_symbol,
      std::map<std::string, std::set<std::string>>* first_sets = nullptr,
      std::map<std::string, std::set<std::string>>* follow_sets = nullptr) const;
};

}  // 命名空间 seu_yacc
