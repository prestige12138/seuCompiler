/**
 * @file lex_state.h
 * @brief Internal allocation helpers and shared construction tables for
 *        seuLex implementation files.
 */

#pragma once

#include <cstddef>
#include <map>

#include "node.h"

namespace seu_lex {

/**
 * @brief Internal rule-priority table used during NFA-to-DFA determinization.
 *
 * This symbol is intentionally private to the implementation layer and must
 * not be included from public headers.
 */
extern std::map<int, std::size_t> nfaPriorityTableInternal;

/**
 * @brief Allocate one NFA-state node from the shared construction arena.
 *
 * The returned pointer remains valid until `resetGlobalTables()` is called.
 */
node* createState(bool accepted = false);

}  // namespace seu_lex
