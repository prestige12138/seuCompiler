/**
 * @file lalr_converter.cpp
 * @brief Canonical LR(1) core merging used to materialize an explicit
 *        LR(1) -> LALR(1) conversion path.
 */

#include "lalr_converter.h"

#include <algorithm>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace seu_yacc {
namespace {

std::string coreItemKey(const ITEM& item) {
  std::ostringstream oss;
  oss << item.left << "->";
  for (std::size_t i = 0; i < item.right.size(); ++i) {
    if (static_cast<int>(i) == item.dotpos) {
      oss << '.';
    }
    oss << item.right[i] << ' ';
  }
  if (item.dotpos == static_cast<int>(item.right.size())) {
    oss << '.';
  }
  return oss.str();
}

std::string coreStateKey(const LRnode& node) {
  std::vector<std::string> keys;
  keys.reserve(node.items.size());
  for (const ITEM& item : node.items) {
    keys.push_back(coreItemKey(item));
  }
  std::sort(keys.begin(), keys.end());
  std::ostringstream oss;
  for (const std::string& key : keys) {
    oss << key << '\n';
  }
  return oss.str();
}

std::vector<ITEM> mergeItems(const std::vector<ITEM>& items) {
  std::vector<ITEM> merged = items;
  std::sort(merged.begin(), merged.end(), [](const ITEM& lhs, const ITEM& rhs) {
    if (lhs.left != rhs.left) {
      return lhs.left < rhs.left;
    }
    if (lhs.right != rhs.right) {
      return lhs.right < rhs.right;
    }
    if (lhs.dotpos != rhs.dotpos) {
      return lhs.dotpos < rhs.dotpos;
    }
    return lhs.predict < rhs.predict;
  });
  merged.erase(std::unique(merged.begin(), merged.end(), [](const ITEM& lhs, const ITEM& rhs) {
                 return lhs.left == rhs.left && lhs.right == rhs.right && lhs.dotpos == rhs.dotpos &&
                        lhs.predict == rhs.predict;
               }),
               merged.end());
  return merged;
}

}  // namespace

LALRResult LALRConverter::convert(const LRPDA& canonical) const {
  LALRResult result;
  result.old_to_new.assign(canonical.nodes.size(), -1);

  std::map<std::string, int> core_to_state;
  std::vector<std::vector<int>> groups;
  for (std::size_t index = 0; index < canonical.nodes.size(); ++index) {
    // States with the same LR(0) core are grouped together; lookaheads are
    // merged afterwards so the resulting graph matches LALR(1) structure.
    const std::string key = coreStateKey(canonical.nodes[index]);
    const auto found = core_to_state.find(key);
    if (found == core_to_state.end()) {
      const int new_state = static_cast<int>(groups.size());
      core_to_state[key] = new_state;
      groups.push_back({static_cast<int>(index)});
      result.old_to_new[index] = new_state;
    } else {
      groups[static_cast<std::size_t>(found->second)].push_back(static_cast<int>(index));
      result.old_to_new[index] = found->second;
    }
  }

  result.automaton.nodes.resize(groups.size());
  for (std::size_t new_state = 0; new_state < groups.size(); ++new_state) {
    LRnode merged;
    merged.stateindex = static_cast<int>(new_state);
    std::vector<ITEM> combined_items;
    for (const int old_state : groups[new_state]) {
      const LRnode& old = canonical.nodes[static_cast<std::size_t>(old_state)];
      combined_items.insert(combined_items.end(), old.items.begin(), old.items.end());
    }
    merged.items = mergeItems(combined_items);
    result.automaton.nodes[new_state] = merged;
  }

  for (std::size_t old_state = 0; old_state < canonical.nodes.size(); ++old_state) {
    const int from = result.old_to_new[old_state];
    for (const auto& edge : canonical.nodes[old_state].nextnode) {
      const int to = result.old_to_new[static_cast<std::size_t>(edge.second)];
      result.automaton.nodes[static_cast<std::size_t>(from)].nextnode[edge.first] = to;
    }
  }
  return result;
}

}  // namespace seu_yacc
