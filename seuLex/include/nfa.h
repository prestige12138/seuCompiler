#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "node.h"

/**
 * @file nfa.h
 * @brief Report-defined NFA type and shared NFA-stage tables.
 */

namespace seu_lex {

/**
 * @brief Report-defined NFA aggregate.
 */
typedef struct nfa {
  node* start;
  std::vector<node*> terminal;
} nfa;

/**
 * @brief Input alphabet collected during NFA construction.
 *
 * The set is filled while Thompson fragments are created and is later reused
 * by subset construction and DFA minimization.
 */
extern std::set<char> char_set;

/**
 * @brief Named regular-definition table from the Lex definitions section.
 */
extern std::map<std::string, std::string> idreTable;

/**
 * @brief One NFA fragment per Lex rule, preserved until global reset.
 */
extern std::vector<nfa> nfaTable;

/**
 * @brief Accept-state to action mapping produced by Thompson construction.
 */
extern std::map<int, std::string> nfaterstatetoaction;

}  // namespace seu_lex
