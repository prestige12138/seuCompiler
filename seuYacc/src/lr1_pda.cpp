#include "lr1_pda.h"

#include <algorithm>
#include <deque>
#include <queue>
#include <set>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace seu_yacc {
namespace {

constexpr const char* kEpsilon = "<epsilon>";
constexpr const char* kEndMarker = "$";
constexpr const char* kAugmentedStart = "__SEU_YACC_AUGMENTED_START__";

struct CoreItem {
  std::string left;
  std::vector<std::string> right;
  int dotpos = 0;
};

struct CoreState {
  int stateindex = 0;
  std::vector<CoreItem> items;
  std::map<std::string, int> nextnode;
};

std::string itemKey(const ITEM& item, bool include_predict) {
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
  if (include_predict) {
    oss << " {" << item.predict << '}';
  }
  return oss.str();
}

std::string coreItemKey(const CoreItem& item) {
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

std::vector<ITEM> normalizeItems(std::vector<ITEM> items) {
  std::sort(items.begin(), items.end(), [](const ITEM& lhs, const ITEM& rhs) {
    return itemKey(lhs, true) < itemKey(rhs, true);
  });
  items.erase(std::unique(items.begin(), items.end(), [](const ITEM& lhs, const ITEM& rhs) {
                return lhs.left == rhs.left && lhs.right == rhs.right && lhs.dotpos == rhs.dotpos &&
                       lhs.predict == rhs.predict;
              }),
              items.end());
  return items;
}

std::vector<CoreItem> normalizeCoreItems(std::vector<CoreItem> items) {
  std::sort(items.begin(), items.end(), [](const CoreItem& lhs, const CoreItem& rhs) {
    return coreItemKey(lhs) < coreItemKey(rhs);
  });
  items.erase(std::unique(items.begin(), items.end(), [](const CoreItem& lhs, const CoreItem& rhs) {
                return lhs.left == rhs.left && lhs.right == rhs.right && lhs.dotpos == rhs.dotpos;
              }),
              items.end());
  return items;
}

std::string stateKey(const std::vector<ITEM>& items, bool include_predict) {
  std::ostringstream oss;
  for (const ITEM& item : items) {
    oss << itemKey(item, include_predict) << '\n';
  }
  return oss.str();
}

std::string coreStateKey(const std::vector<CoreItem>& items) {
  std::ostringstream oss;
  for (const CoreItem& item : items) {
    oss << coreItemKey(item) << '\n';
  }
  return oss.str();
}

std::vector<producer> buildAugmentedProductions(const std::string& start_symbol) {
  std::vector<producer> all;
  producer augmented;
  augmented.left = kAugmentedStart;
  augmented.right = {start_symbol};
  all.push_back(augmented);
  all.insert(all.end(), producers.begin(), producers.end());
  return all;
}

std::map<std::string, std::vector<producer>> buildProductionMap(
    const std::vector<producer>& all_productions) {
  std::map<std::string, std::vector<producer>> by_left;
  for (const producer& production : all_productions) {
    by_left[production.left].push_back(production);
  }
  return by_left;
}

bool isNonterminalSymbol(const std::string& symbol) {
  return std::find(nonterminals.begin(), nonterminals.end(), symbol) != nonterminals.end() ||
         symbol == kAugmentedStart;
}

std::set<std::string> firstOfSequence(
    const std::vector<std::string>& sequence,
    std::size_t start_index,
    const std::map<std::string, std::set<std::string>>& first_sets,
    const std::string& lookahead) {
  std::set<std::string> result;
  bool all_nullable = true;
  for (std::size_t index = start_index; index < sequence.size(); ++index) {
    const auto found = first_sets.find(sequence[index]);
    if (found == first_sets.end()) {
      result.insert(sequence[index]);
      all_nullable = false;
      break;
    }
    for (const std::string& value : found->second) {
      if (value != kEpsilon) {
        result.insert(value);
      }
    }
    if (found->second.count(kEpsilon) == 0) {
      all_nullable = false;
      break;
    }
  }
  if (all_nullable) {
    result.insert(lookahead);
  }
  return result;
}

ITEM makeItem(const CoreItem& core, const std::string& predict) {
  ITEM item;
  item.left = core.left;
  item.right = core.right;
  item.dotpos = core.dotpos;
  item.predict = predict;
  return item;
}

std::set<std::string> symbolsAfterDots(const std::vector<ITEM>& items) {
  std::set<std::string> symbols;
  for (const ITEM& item : items) {
    if (item.dotpos < static_cast<int>(item.right.size())) {
      symbols.insert(item.right[item.dotpos]);
    }
  }
  return symbols;
}

std::set<std::string> coreSymbolsAfterDots(const std::vector<CoreItem>& items) {
  std::set<std::string> symbols;
  for (const CoreItem& item : items) {
    if (item.dotpos < static_cast<int>(item.right.size())) {
      symbols.insert(item.right[item.dotpos]);
    }
  }
  return symbols;
}

std::vector<ITEM> closureWithProductions(
    const std::vector<ITEM>& kernel,
    const std::map<std::string, std::set<std::string>>& first_sets,
    const std::map<std::string, std::vector<producer>>& productions_by_left) {
  std::vector<ITEM> result = normalizeItems(kernel);
  std::set<std::string> seen;
  std::deque<ITEM> work;
  for (const ITEM& item : result) {
    seen.insert(itemKey(item, true));
    work.push_back(item);
  }

  while (!work.empty()) {
    const ITEM item = work.front();
    work.pop_front();
    if (item.dotpos >= static_cast<int>(item.right.size())) {
      continue;
    }
    const std::string& symbol = item.right[item.dotpos];
    if (!isNonterminalSymbol(symbol)) {
      continue;
    }

    const auto found_productions = productions_by_left.find(symbol);
    if (found_productions == productions_by_left.end()) {
      continue;
    }

    const std::set<std::string> predicts =
        firstOfSequence(item.right, static_cast<std::size_t>(item.dotpos + 1), first_sets, item.predict);
    for (const producer& production : found_productions->second) {
      for (const std::string& predict : predicts) {
        ITEM next;
        next.left = production.left;
        next.right = production.right;
        next.dotpos = 0;
        next.predict = predict;
        const std::string key = itemKey(next, true);
        if (seen.insert(key).second) {
          result.push_back(next);
          work.push_back(next);
        }
      }
    }
  }

  return normalizeItems(result);
}

std::vector<CoreItem> lr0Closure(
    const std::vector<CoreItem>& kernel,
    const std::map<std::string, std::vector<producer>>& productions_by_left) {
  std::vector<CoreItem> result = normalizeCoreItems(kernel);
  std::set<std::string> seen;
  std::deque<CoreItem> work;
  for (const CoreItem& item : result) {
    seen.insert(coreItemKey(item));
    work.push_back(item);
  }

  while (!work.empty()) {
    const CoreItem item = work.front();
    work.pop_front();
    if (item.dotpos >= static_cast<int>(item.right.size())) {
      continue;
    }
    const std::string& symbol = item.right[item.dotpos];
    if (!isNonterminalSymbol(symbol)) {
      continue;
    }

    const auto found_productions = productions_by_left.find(symbol);
    if (found_productions == productions_by_left.end()) {
      continue;
    }

    for (const producer& production : found_productions->second) {
      CoreItem next;
      next.left = production.left;
      next.right = production.right;
      next.dotpos = 0;
      const std::string key = coreItemKey(next);
      if (seen.insert(key).second) {
        result.push_back(next);
        work.push_back(next);
      }
    }
  }

  return normalizeCoreItems(result);
}

std::vector<CoreItem> lr0Goto(
    const std::vector<CoreItem>& items,
    const std::string& symbol,
    const std::map<std::string, std::vector<producer>>& productions_by_left) {
  std::vector<CoreItem> moved;
  for (const CoreItem& item : items) {
    if (item.dotpos < static_cast<int>(item.right.size()) && item.right[item.dotpos] == symbol) {
      CoreItem next = item;
      ++next.dotpos;
      moved.push_back(next);
    }
  }
  if (moved.empty()) {
    return {};
  }
  return lr0Closure(moved, productions_by_left);
}

std::vector<CoreState> buildLR0Automaton(
    const std::string& start_symbol,
    const std::map<std::string, std::vector<producer>>& productions_by_left) {
  CoreItem start_item;
  start_item.left = kAugmentedStart;
  start_item.right = {start_symbol};
  start_item.dotpos = 0;

  const std::vector<CoreItem> start_state = lr0Closure({start_item}, productions_by_left);
  std::vector<CoreState> automaton;
  CoreState initial;
  initial.stateindex = 0;
  initial.items = start_state;
  automaton.push_back(initial);

  std::map<std::string, int> known_states;
  known_states[coreStateKey(start_state)] = 0;
  std::queue<int> work;
  work.push(0);

  while (!work.empty()) {
    const int state_index = work.front();
    work.pop();
    const std::vector<CoreItem> items = automaton[static_cast<std::size_t>(state_index)].items;
    for (const std::string& symbol : coreSymbolsAfterDots(items)) {
      const std::vector<CoreItem> target = lr0Goto(items, symbol, productions_by_left);
      if (target.empty()) {
        continue;
      }

      const std::string key = coreStateKey(target);
      int target_state = -1;
      const auto found = known_states.find(key);
      if (found == known_states.end()) {
        CoreState next;
        next.stateindex = static_cast<int>(automaton.size());
        next.items = target;
        automaton.push_back(next);
        known_states[key] = next.stateindex;
        target_state = next.stateindex;
        work.push(next.stateindex);
      } else {
        target_state = found->second;
      }
      automaton[static_cast<std::size_t>(state_index)].nextnode[symbol] = target_state;
    }
  }

  return automaton;
}

}  // namespace

std::map<std::string, std::set<std::string>> LR1Builder::computeFirstSets() const {
  std::map<std::string, std::set<std::string>> first_sets;
  for (const std::string& terminal : terminals) {
    first_sets[terminal].insert(terminal);
  }
  first_sets[kEndMarker].insert(kEndMarker);
  for (const std::string& nonterminal : nonterminals) {
    first_sets[nonterminal];
  }
  first_sets[kAugmentedStart];

  bool changed = true;
  while (changed) {
    changed = false;
    for (const producer& production : producers) {
      auto& target = first_sets[production.left];
      if (production.right.empty()) {
        if (target.insert(kEpsilon).second) {
          changed = true;
        }
        continue;
      }
      bool all_nullable = true;
      for (const std::string& symbol : production.right) {
        auto& source = first_sets[symbol];
        if (source.empty() && !isNonterminalSymbol(symbol)) {
          source.insert(symbol);
        }
        for (const std::string& value : source) {
          if (value != kEpsilon && target.insert(value).second) {
            changed = true;
          }
        }
        if (source.count(kEpsilon) == 0) {
          all_nullable = false;
          break;
        }
      }
      if (all_nullable && target.insert(kEpsilon).second) {
        changed = true;
      }
    }
  }
  return first_sets;
}

std::map<std::string, std::set<std::string>> LR1Builder::computeFollowSets(
    const std::map<std::string, std::set<std::string>>& first_sets,
    const std::string& start_symbol) const {
  std::map<std::string, std::set<std::string>> follow_sets;
  for (const std::string& nonterminal : nonterminals) {
    follow_sets[nonterminal];
  }
  follow_sets[start_symbol].insert(kEndMarker);

  bool changed = true;
  while (changed) {
    changed = false;
    for (const producer& production : producers) {
      for (std::size_t index = 0; index < production.right.size(); ++index) {
        const std::string& symbol = production.right[index];
        if (!isNonterminalSymbol(symbol) || symbol == kAugmentedStart) {
          continue;
        }
        bool suffix_nullable = true;
        for (std::size_t suffix = index + 1; suffix < production.right.size(); ++suffix) {
          const auto found = first_sets.find(production.right[suffix]);
          if (found == first_sets.end()) {
            if (follow_sets[symbol].insert(production.right[suffix]).second) {
              changed = true;
            }
            suffix_nullable = false;
            break;
          }
          for (const std::string& value : found->second) {
            if (value != kEpsilon && follow_sets[symbol].insert(value).second) {
              changed = true;
            }
          }
          if (found->second.count(kEpsilon) == 0) {
            suffix_nullable = false;
            break;
          }
        }
        if (index + 1 == production.right.size() || suffix_nullable) {
          for (const std::string& value : follow_sets[production.left]) {
            if (follow_sets[symbol].insert(value).second) {
              changed = true;
            }
          }
        }
      }
    }
  }
  return follow_sets;
}

std::vector<ITEM> LR1Builder::closure(
    const std::vector<ITEM>& kernel,
    const std::map<std::string, std::set<std::string>>& first_sets,
    const std::string& start_symbol) const {
  const std::vector<producer> all_productions = buildAugmentedProductions(start_symbol);
  const auto productions_by_left = buildProductionMap(all_productions);
  return closureWithProductions(kernel, first_sets, productions_by_left);
}

std::vector<ITEM> LR1Builder::gotoSet(
    const std::vector<ITEM>& items,
    const std::string& symbol,
    const std::map<std::string, std::set<std::string>>& first_sets,
    const std::string& start_symbol) const {
  std::vector<ITEM> moved;
  for (const ITEM& item : items) {
    if (item.dotpos < static_cast<int>(item.right.size()) && item.right[item.dotpos] == symbol) {
      ITEM next = item;
      ++next.dotpos;
      moved.push_back(next);
    }
  }
  if (moved.empty()) {
    return {};
  }
  const std::vector<producer> all_productions = buildAugmentedProductions(start_symbol);
  const auto productions_by_left = buildProductionMap(all_productions);
  return closureWithProductions(moved, first_sets, productions_by_left);
}

LRPDA LR1Builder::buildLALRPDA(
    const std::string& start_symbol,
    std::map<std::string, std::set<std::string>>* first_sets,
    std::map<std::string, std::set<std::string>>* follow_sets) const {
  const auto computed_first = computeFirstSets();
  if (first_sets != nullptr) {
    *first_sets = computed_first;
  }
  if (follow_sets != nullptr) {
    *follow_sets = computeFollowSets(computed_first, start_symbol);
  }

  const std::vector<producer> all_productions = buildAugmentedProductions(start_symbol);
  const auto productions_by_left = buildProductionMap(all_productions);
  const std::vector<CoreState> core_automaton = buildLR0Automaton(start_symbol, productions_by_left);

  std::vector<std::map<std::string, std::set<std::string>>> lookaheads(core_automaton.size());
  CoreItem start_item;
  start_item.left = kAugmentedStart;
  start_item.right = {start_symbol};
  start_item.dotpos = 0;
  lookaheads[0][coreItemKey(start_item)].insert(kEndMarker);

  std::queue<int> work;
  std::vector<bool> queued(core_automaton.size(), false);
  work.push(0);
  queued[0] = true;

  while (!work.empty()) {
    const int state_index = work.front();
    work.pop();
    queued[static_cast<std::size_t>(state_index)] = false;
    const CoreState& state = core_automaton[static_cast<std::size_t>(state_index)];

    bool local_changed = true;
    while (local_changed) {
      local_changed = false;
      for (const CoreItem& item : state.items) {
        const auto found_lookahead = lookaheads[static_cast<std::size_t>(state_index)].find(coreItemKey(item));
        if (found_lookahead == lookaheads[static_cast<std::size_t>(state_index)].end() ||
            found_lookahead->second.empty()) {
          continue;
        }
        if (item.dotpos >= static_cast<int>(item.right.size())) {
          continue;
        }

        const std::string& symbol = item.right[static_cast<std::size_t>(item.dotpos)];
        if (!isNonterminalSymbol(symbol)) {
          continue;
        }

        const auto found_productions = productions_by_left.find(symbol);
        if (found_productions == productions_by_left.end()) {
          continue;
        }

        std::set<std::string> propagated_predicts;
        for (const std::string& lookahead : found_lookahead->second) {
          const std::set<std::string> predicts =
              firstOfSequence(item.right, static_cast<std::size_t>(item.dotpos + 1), computed_first, lookahead);
          propagated_predicts.insert(predicts.begin(), predicts.end());
        }

        for (const producer& production : found_productions->second) {
          CoreItem target_core;
          target_core.left = production.left;
          target_core.right = production.right;
          target_core.dotpos = 0;
          auto& target_predicts =
              lookaheads[static_cast<std::size_t>(state_index)][coreItemKey(target_core)];
          const std::size_t before = target_predicts.size();
          target_predicts.insert(propagated_predicts.begin(), propagated_predicts.end());
          if (target_predicts.size() != before) {
            local_changed = true;
          }
        }
      }
    }

    for (const auto& edge : state.nextnode) {
      bool target_changed = false;
      for (const CoreItem& item : state.items) {
        if (item.dotpos >= static_cast<int>(item.right.size()) || item.right[item.dotpos] != edge.first) {
          continue;
        }
        const auto found_lookahead = lookaheads[static_cast<std::size_t>(state_index)].find(coreItemKey(item));
        if (found_lookahead == lookaheads[static_cast<std::size_t>(state_index)].end() ||
            found_lookahead->second.empty()) {
          continue;
        }

        CoreItem advanced = item;
        ++advanced.dotpos;
        auto& target_predicts =
            lookaheads[static_cast<std::size_t>(edge.second)][coreItemKey(advanced)];
        const std::size_t before = target_predicts.size();
        target_predicts.insert(found_lookahead->second.begin(), found_lookahead->second.end());
        if (target_predicts.size() != before) {
          target_changed = true;
        }
      }

      if (target_changed && !queued[static_cast<std::size_t>(edge.second)]) {
        work.push(edge.second);
        queued[static_cast<std::size_t>(edge.second)] = true;
      }
    }
  }

  LRPDA automaton;
  automaton.nodes.resize(core_automaton.size());
  for (std::size_t state_index = 0; state_index < core_automaton.size(); ++state_index) {
    LRnode node;
    node.stateindex = static_cast<int>(state_index);
    node.nextnode = core_automaton[state_index].nextnode;
    std::vector<ITEM> items;
    for (const CoreItem& core_item : core_automaton[state_index].items) {
      const auto found_lookahead = lookaheads[state_index].find(coreItemKey(core_item));
      if (found_lookahead == lookaheads[state_index].end()) {
        continue;
      }
      for (const std::string& predict : found_lookahead->second) {
        items.push_back(makeItem(core_item, predict));
      }
    }
    node.items = normalizeItems(std::move(items));
    automaton.nodes[state_index] = node;
  }

  return automaton;
}

LRPDA LR1Builder::buildCanonicalPDA(
    const std::string& start_symbol,
    std::map<std::string, std::set<std::string>>* first_sets,
    std::map<std::string, std::set<std::string>>* follow_sets) const {
  const auto computed_first = computeFirstSets();
  if (first_sets != nullptr) {
    *first_sets = computed_first;
  }
  if (follow_sets != nullptr) {
    *follow_sets = computeFollowSets(computed_first, start_symbol);
  }
  const std::vector<producer> all_productions = buildAugmentedProductions(start_symbol);
  const auto productions_by_left = buildProductionMap(all_productions);

  ITEM start_item;
  start_item.left = kAugmentedStart;
  start_item.right = {start_symbol};
  start_item.dotpos = 0;
  start_item.predict = kEndMarker;

  const std::vector<ITEM> start_state = closureWithProductions({start_item}, computed_first, productions_by_left);
  LRPDA automaton;
  LRnode initial;
  initial.stateindex = 0;
  initial.items = start_state;
  automaton.nodes.push_back(initial);

  std::map<std::string, int> known_states;
  known_states[stateKey(start_state, true)] = 0;
  std::queue<int> work;
  work.push(0);

  while (!work.empty()) {
    const int state_index = work.front();
    work.pop();
    const std::vector<ITEM> items = automaton.nodes[static_cast<std::size_t>(state_index)].items;
    for (const std::string& symbol : symbolsAfterDots(items)) {
      std::vector<ITEM> moved;
      for (const ITEM& item : items) {
        if (item.dotpos < static_cast<int>(item.right.size()) && item.right[item.dotpos] == symbol) {
          ITEM next = item;
          ++next.dotpos;
          moved.push_back(next);
        }
      }
      const std::vector<ITEM> target =
          moved.empty() ? std::vector<ITEM>{} : closureWithProductions(moved, computed_first, productions_by_left);
      if (target.empty()) {
        continue;
      }
      const std::string key = stateKey(target, true);
      int target_state = -1;
      const auto found = known_states.find(key);
      if (found == known_states.end()) {
        LRnode next;
        next.stateindex = static_cast<int>(automaton.nodes.size());
        next.items = target;
        automaton.nodes.push_back(next);
        known_states[key] = next.stateindex;
        target_state = next.stateindex;
        work.push(next.stateindex);
      } else {
        target_state = found->second;
      }
      automaton.nodes[static_cast<std::size_t>(state_index)].nextnode[symbol] = target_state;
    }
  }
  return automaton;
}

}  // namespace seu_yacc
