#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "node.h"

/**
 * @file dfa.h
 * @brief Report-defined DFA type and shared DFA-stage tables.
 */

namespace seu_lex {

/**
 * @brief Report-defined DFA aggregate.
 */
typedef struct dfa {
  node* start;
  std::vector<node> nodeVec;
  std::vector<node> endNode;

  /**
   * @brief Construct a DFA with the given start node.
   *
   * Complexity: O(1).
   */
  dfa(node* st = nullptr);

  /**
   * @brief Expand epsilon closure over an NFA-state set.
   *
   * Complexity: O(V + E) over the reachable epsilon subgraph.
   */
  void Eclosure(std::set<node*>& x);

  /**
   * @brief Print DFA transitions for debugging.
   *
   * Complexity: O(V + E).
   */
  void printDFA();
} dfa;

/**
 * @brief Accepting DFA states after determinization or minimization.
 */
extern std::vector<node*> dfaterminals;

/**
 * @brief Action table for the current DFA before minimization.
 */
extern std::map<int, std::string> TerStateActionTable;

/**
 * @brief Action table for the minimized DFA used by code emission.
 */
extern std::map<int, std::string> mindfareturn;

}  // namespace seu_lex
