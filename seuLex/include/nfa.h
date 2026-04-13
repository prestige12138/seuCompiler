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

extern std::set<char> char_set;
extern std::map<std::string, std::string> idreTable;
extern std::vector<nfa> nfaTable;
extern std::map<int, std::string> nfaterstatetoaction;

}  // namespace seu_lex
