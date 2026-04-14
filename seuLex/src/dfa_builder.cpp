#include "dfa_builder.h"

#include <iostream>
#include <cstddef>
#include <limits>
#include <map>
#include <queue>
#include <sstream>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "internal/lex_state.h"

namespace seu_lex {

namespace {

constexpr char kEpsilon = '\0';

std::string joinKey(const std::vector<int>& ids) {
  std::ostringstream oss;
  for (std::size_t index = 0; index < ids.size(); ++index) {
    if (index != 0) {
      oss << ',';
    }
    oss << ids[index];
  }
  return oss.str();
}

std::string pickActionFromSet(const std::set<node*>& states) {
  std::size_t best_priority = std::numeric_limits<std::size_t>::max();
  std::string chosen;
  for (node* state : states) {
    if (!state->IsAccepted()) {
      continue;
    }
    const int label = state->GetState();
    const auto found_priority = nfaPriorityTableInternal.find(label);
    if (found_priority == nfaPriorityTableInternal.end()) {
      continue;
    }
    if (found_priority->second < best_priority) {
      best_priority = found_priority->second;
      chosen = nfaterstatetoaction.at(label);
    }
  }
  return chosen;
}

}  // namespace

dfa::dfa(node* st) : start(st) {}

void dfa::Eclosure(std::set<node*>& x) {
  std::queue<node*> work;
  for (node* item : x) {
    if (item != nullptr) {
      work.push(item);
    }
  }
  while (!work.empty()) {
    node* current = work.front();
    work.pop();
    if (current == nullptr) {
      continue;
    }
    const auto transitions = current->getMultimap();
    const auto range = transitions.equal_range(kEpsilon);
    for (auto it = range.first; it != range.second; ++it) {
      if (x.insert(it->second).second) {
        work.push(it->second);
      }
    }
  }
}

void dfa::printDFA() {
  for (const node& state : nodeVec) {
    std::cout << "state " << state.GetState();
    if (state.IsAccepted()) {
      std::cout << " [accept]";
    }
    std::cout << '\n';
    const auto transitions = state.getMultimap();
    for (const auto& entry : transitions) {
      std::cout << "  --" << static_cast<int>(static_cast<unsigned char>(entry.first)) << "--> "
                << entry.second->GetState() << '\n';
    }
  }
}

dfa DFABuilder::subsetConstruct(const nfa& automaton) const {
  if (automaton.start == nullptr) {
    dfaterminals.clear();
    TerStateActionTable.clear();
    mindfareturn.clear();
    return dfa();
  }

  std::set<node*> start_set = {automaton.start};
  dfa helper;
  helper.Eclosure(start_set);

  std::vector<std::set<node*>> state_sets;
  std::vector<std::map<char, int>> transitions;
  std::vector<std::string> accept_actions;
  std::unordered_map<std::string, int> state_index;
  std::queue<int> work;

  auto keyFromSet = [](const std::set<node*>& states) {
    std::vector<int> ids;
    ids.reserve(states.size());
    for (node* item : states) {
      ids.push_back(item->GetState());
    }
    return joinKey(ids);
  };

  state_sets.push_back(start_set);
  transitions.emplace_back();
  accept_actions.push_back(pickActionFromSet(start_set));
  state_index[keyFromSet(start_set)] = 0;
  work.push(0);

  while (!work.empty()) {
    const int current_id = work.front();
    work.pop();
    for (char symbol : char_set) {
      std::set<node*> moved;
      for (node* state : state_sets[static_cast<std::size_t>(current_id)]) {
        const auto edges = state->getMultimap();
        const auto range = edges.equal_range(symbol);
        for (auto it = range.first; it != range.second; ++it) {
          moved.insert(it->second);
        }
      }
      if (moved.empty()) {
        continue;
      }
      helper.Eclosure(moved);
      const std::string key = keyFromSet(moved);
      int target_id = -1;
      const auto found = state_index.find(key);
      if (found == state_index.end()) {
        target_id = static_cast<int>(state_sets.size());
        state_index[key] = target_id;
        state_sets.push_back(moved);
        transitions.emplace_back();
        accept_actions.push_back(pickActionFromSet(moved));
        work.push(target_id);
      } else {
        target_id = found->second;
      }
      transitions[static_cast<std::size_t>(current_id)][symbol] = target_id;
    }
  }

  dfa result;
  result.nodeVec.resize(state_sets.size());
  result.endNode.clear();
  dfaterminals.clear();
  TerStateActionTable.clear();
  for (std::size_t state_id = 0; state_id < result.nodeVec.size(); ++state_id) {
    result.nodeVec[state_id].Setstate(static_cast<int>(state_id));
    result.nodeVec[state_id].SetAccept(!accept_actions[state_id].empty());
  }
  for (std::size_t state_id = 0; state_id < transitions.size(); ++state_id) {
    for (const auto& entry : transitions[state_id]) {
      result.nodeVec[state_id].Addoutstate(entry.first, &result.nodeVec[static_cast<std::size_t>(entry.second)]);
    }
  }
  if (!result.nodeVec.empty()) {
    result.start = &result.nodeVec.front();
  }
  for (std::size_t state_id = 0; state_id < accept_actions.size(); ++state_id) {
    if (!accept_actions[state_id].empty()) {
      TerStateActionTable[static_cast<int>(state_id)] = accept_actions[state_id];
      dfaterminals.push_back(&result.nodeVec[state_id]);
      result.endNode.push_back(result.nodeVec[state_id]);
    }
  }
  return result;
}

}  // namespace seu_lex
