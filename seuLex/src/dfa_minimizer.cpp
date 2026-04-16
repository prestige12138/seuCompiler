/**
 * @file dfa_minimizer.cpp
 * @brief 实现基于分区细化的 DFA 最小化过程。
 */

#include "dfa_minimizer.h"

#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include "nfa.h"

namespace seu_lex {
namespace {

/**
 * @brief 查询某个状态当前应当对应的接受动作。
 */
std::string actionForState(int stateId) {
  const auto minimal = mindfareturn.find(stateId);
  if (minimal != mindfareturn.end()) {
    return minimal->second;
  }
  const auto terminal = TerStateActionTable.find(stateId);
  if (terminal != TerStateActionTable.end()) {
    return terminal->second;
  }
  return "";
}

}  // 匿名命名空间

/**
 * @brief 对 DFA 进行最小化，并同步更新接受态动作映射。
 */
dfa DFAMinimizer::minimizeDFA(const dfa& automaton) const {
  if (automaton.nodeVec.empty()) {
    return automaton;
  }

  std::vector<std::vector<int>> partitions;
  std::map<std::string, std::vector<int>> acceptingByAction;
  std::vector<int> nonAccepting;
  for (const node& state : automaton.nodeVec) {
    if (state.IsAccepted()) {
      acceptingByAction[TerStateActionTable[state.GetState()]].push_back(state.GetState());
    } else {
      nonAccepting.push_back(state.GetState());
    }
  }
  if (!nonAccepting.empty()) {
    partitions.push_back(nonAccepting);
  }
  for (const auto& entry : acceptingByAction) {
    partitions.push_back(entry.second);
  }

  std::vector<int> stateToPartition(automaton.nodeVec.size(), -1);
  auto rebuildMap = [&]() {
    for (std::size_t partitionId = 0; partitionId < partitions.size(); ++partitionId) {
      for (int state : partitions[partitionId]) {
        stateToPartition[state] = static_cast<int>(partitionId);
      }
    }
  };
  rebuildMap();

  bool changed = true;
  while (changed) {
    changed = false;
    std::vector<std::vector<int>> refined;
    for (const auto& group : partitions) {
      std::map<std::string, std::vector<int>> buckets;
      for (int stateId : group) {
        std::ostringstream signature;
        // 两个状态只有在出边落点分区和接受动作都一致时，
        // 才能继续保留在同一个等价类里。
        signature << stateToPartition[stateId] << '#';
        signature << actionForState(stateId) << '#';
        const auto transitions = automaton.nodeVec[stateId].getMultimap();
        for (char symbol : char_set) {
          int targetPartition = -1;
          const auto range = transitions.equal_range(symbol);
          if (range.first != range.second) {
            targetPartition = stateToPartition[range.first->second->GetState()];
          }
          signature << static_cast<int>(static_cast<unsigned char>(symbol)) << ':'
                    << targetPartition << ';';
        }
        buckets[signature.str()].push_back(stateId);
      }
      for (const auto& bucket : buckets) {
        refined.push_back(bucket.second);
      }
      if (buckets.size() > 1) {
        changed = true;
      }
    }
    partitions = refined;
    rebuildMap();
  }

  const int startPartition = stateToPartition[automaton.start->GetState()];
  std::vector<std::map<char, int>> minimizedTransitions(partitions.size());
  std::vector<std::string> minimizedActions(partitions.size());
  for (std::size_t partitionId = 0; partitionId < partitions.size(); ++partitionId) {
    const int representative = partitions[partitionId].front();
    const auto edges = automaton.nodeVec[representative].getMultimap();
    for (char symbol : char_set) {
      const auto range = edges.equal_range(symbol);
      if (range.first != range.second) {
        minimizedTransitions[partitionId][symbol] =
            stateToPartition[range.first->second->GetState()];
      }
    }
    minimizedActions[partitionId] = actionForState(representative);
  }

  dfa result;
  result.nodeVec.resize(partitions.size());
  result.endNode.clear();
  dfaterminals.clear();
  TerStateActionTable.clear();
  mindfareturn.clear();

  for (std::size_t stateId = 0; stateId < result.nodeVec.size(); ++stateId) {
    result.nodeVec[stateId].Setstate(static_cast<int>(stateId));
    result.nodeVec[stateId].SetAccept(!minimizedActions[stateId].empty());
  }
  for (std::size_t stateId = 0; stateId < minimizedTransitions.size(); ++stateId) {
    for (const auto& entry : minimizedTransitions[stateId]) {
      result.nodeVec[stateId].Addoutstate(entry.first, &result.nodeVec[entry.second]);
    }
  }
  result.start = &result.nodeVec[startPartition];
  for (std::size_t stateId = 0; stateId < minimizedActions.size(); ++stateId) {
    if (!minimizedActions[stateId].empty()) {
      TerStateActionTable[static_cast<int>(stateId)] = minimizedActions[stateId];
      mindfareturn[static_cast<int>(stateId)] = minimizedActions[stateId];
      dfaterminals.push_back(&result.nodeVec[stateId]);
      result.endNode.push_back(result.nodeVec[stateId]);
    }
  }
  return result;
}

}  // 命名空间 seu_lex
