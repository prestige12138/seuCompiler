#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "symbol_table.h"
#include "yacc_parser.h"

/**
 * @file lr1_pda.h
 * @brief LR(1) item-set automaton construction interfaces.
 */

namespace seu_yacc {

/**
 * @brief Report-defined LR(1) item.
 */
typedef struct ITEM {
  std::string left;
  std::vector<std::string> right;
  int dotpos = 0;
  std::string predict;
} LRItem;

/**
 * @brief Report-defined LR automaton node.
 */
typedef struct node {
  int stateindex = 0;
  std::vector<ITEM> items;
  std::map<std::string, int> nextnode;
} LRnode;

/**
 * @brief Report-defined LR pushdown automaton.
 */
typedef struct PDA {
  std::vector<LRnode> nodes;
} LRPDA;

/**
 * @brief Build FIRST/FOLLOW sets and the canonical LR(1) automaton.
 */
class LR1Builder {
 public:
  /**
   * @brief Compute FIRST sets for the current grammar.
   *
   * Complexity: O(P * K * I), where P is production count, K is average RHS
   * length, and I is refinement rounds.
   */
  std::map<std::string, std::set<std::string>> computeFirstSets() const;

  /**
   * @brief Compute FOLLOW sets for the current grammar.
   *
   * Complexity: O(P * K * I).
   */
  std::map<std::string, std::set<std::string>> computeFollowSets(
      const std::map<std::string, std::set<std::string>>& first_sets,
      const std::string& start_symbol) const;

  /**
   * @brief Compute LR(1) closure of one kernel item set.
   *
   * Complexity: O(P * T) over reachable closure items.
   */
  std::vector<ITEM> closure(
      const std::vector<ITEM>& kernel,
      const std::map<std::string, std::set<std::string>>& first_sets,
      const std::string& start_symbol) const;

  /**
   * @brief Compute GOTO(items, symbol).
   *
   * Complexity: O(I + C), where I is input item count and C is closure cost.
   */
  std::vector<ITEM> gotoSet(
      const std::vector<ITEM>& items,
      const std::string& symbol,
      const std::map<std::string, std::set<std::string>>& first_sets,
      const std::string& start_symbol) const;

  /**
   * @brief Build an LALR(1) automaton directly on LR(0) cores by propagating
   * LR(1) lookaheads until a fixed point.
   *
   * Complexity: O(S * I * L * F) per propagation round in the worst case,
   * where S is LR(0) state count, I is items per state, L is lookahead count,
   * and F is FIRST-sequence cost.
   */
  LRPDA buildLALRPDA(
      const std::string& start_symbol,
      std::map<std::string, std::set<std::string>>* first_sets = nullptr,
      std::map<std::string, std::set<std::string>>* follow_sets = nullptr) const;

  /**
   * @brief Build the canonical LR(1) item-set automaton.
   *
   * Complexity: exponential in the worst case.
   */
  LRPDA buildCanonicalPDA(
      const std::string& start_symbol,
      std::map<std::string, std::set<std::string>>* first_sets = nullptr,
      std::map<std::string, std::set<std::string>>* follow_sets = nullptr) const;
};

}  // namespace seu_yacc
