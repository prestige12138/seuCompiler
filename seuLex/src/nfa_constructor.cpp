/**
 * @file nfa_constructor.cpp
 * @brief 实现中缀正则转后缀以及 Thompson NFA 构造流程。
 */

#include "nfa_constructor.h"

#include <sstream>
#include <stack>
#include <stdexcept>
#include <utility>
#include <vector>

#include "internal/lex_state.h"

namespace seu_lex {

namespace {

constexpr char kEpsilon = '\0';
constexpr int kAsciiLimit = 128;

/**
 * @brief 暂存一个 Thompson 片段的起点和终点。
 */
struct Fragment {
  node* start = nullptr;
  node* accept = nullptr;
};

/**
 * @brief 将以空格分隔的显式正则记号序列切分为 token 列表。
 */
std::vector<std::string> splitSpaceTokens(const std::string& text) {
  std::stringstream stream(text);
  std::vector<std::string> tokens;
  std::string token;
  while (stream >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

}  // 匿名命名空间

/**
 * @brief 将扩展器输出的显式中缀正则转换为后缀形式。
 */
std::string NFABuilder::toPostfix(const std::string& infix) const {
  // 正规式展开阶段已经把表达式改写成显式的中缀记号串，
  // 这里仅需对 `|`、显式连接符 `&` 和 `*` 做一次调度场转换。
  const std::vector<std::string> tokens = splitSpaceTokens(infix);
  std::vector<std::string> output;
  std::stack<std::string> operators;
  const auto precedence = [](const std::string& token) {
    if (token == "|") {
      return 1;
    }
    if (token == "&") {
      return 2;
    }
    return 0;
  };

  for (const std::string& token : tokens) {
    if (token.rfind("ch:", 0) == 0 || token == "eps") {
      output.push_back(token);
      continue;
    }
    if (token == "*") {
      output.push_back(token);
      continue;
    }
    if (token == "(") {
      operators.push(token);
      continue;
    }
    if (token == ")") {
      while (!operators.empty() && operators.top() != "(") {
        output.push_back(operators.top());
        operators.pop();
      }
      if (operators.empty()) {
        throw std::runtime_error("mismatched parentheses in RE");
      }
      operators.pop();
      continue;
    }
    if (token == "|" || token == "&") {
      while (!operators.empty() && operators.top() != "(" &&
             precedence(operators.top()) >= precedence(token)) {
        output.push_back(operators.top());
        operators.pop();
      }
      operators.push(token);
      continue;
    }
    throw std::runtime_error("unknown infix RE token: " + token);
  }

  while (!operators.empty()) {
    if (operators.top() == "(") {
      throw std::runtime_error("mismatched parentheses in RE");
    }
    output.push_back(operators.top());
    operators.pop();
  }

  std::ostringstream stream;
  for (std::size_t index = 0; index < output.size(); ++index) {
    if (index != 0) {
      stream << ' ';
    }
    stream << output[index];
  }
  return stream.str();
}

/**
 * @brief 根据后缀正则表达式构造单条规则对应的 NFA。
 */
nfa NFABuilder::buildNFA(const std::string& postfix,
                         const std::string& action,
                         std::size_t priority) const {
  std::stack<Fragment> fragments;
  for (const std::string& token : splitSpaceTokens(postfix)) {
    if (token == "eps") {
      node* start = createState(false);
      node* accept = createState(true);
      start->Addoutstate(kEpsilon, accept);
      fragments.push({start, accept});
      continue;
    }

    if (token.rfind("ch:", 0) == 0) {
      const int ascii = std::stoi(token.substr(3));
      if (ascii <= 0 || ascii >= kAsciiLimit) {
        throw std::runtime_error("literal out of supported ASCII range");
      }
      const char ch = static_cast<char>(ascii);
      char_set.insert(ch);
      node* start = createState(false);
      node* accept = createState(true);
      start->Addoutstate(ch, accept);
      fragments.push({start, accept});
      continue;
    }

    if (token == "&") {
      if (fragments.size() < 2) {
        throw std::runtime_error("malformed postfix RE around '&'");
      }
      Fragment rhs = fragments.top();
      fragments.pop();
      Fragment lhs = fragments.top();
      fragments.pop();
      lhs.accept->SetAccept(false);
      // 汤普森连接规则：把左片段的接受态用空转移接到右片段起点，
      // 这样栈上最终仍只保留一个片段边界。
      lhs.accept->Addoutstate(kEpsilon, rhs.start);
      fragments.push({lhs.start, rhs.accept});
      continue;
    }

    if (token == "|") {
      if (fragments.size() < 2) {
        throw std::runtime_error("malformed postfix RE around '|'");
      }
      Fragment rhs = fragments.top();
      fragments.pop();
      Fragment lhs = fragments.top();
      fragments.pop();
      node* start = createState(false);
      node* accept = createState(true);
      lhs.accept->SetAccept(false);
      rhs.accept->SetAccept(false);
      start->Addoutstate(kEpsilon, lhs.start);
      start->Addoutstate(kEpsilon, rhs.start);
      lhs.accept->Addoutstate(kEpsilon, accept);
      rhs.accept->Addoutstate(kEpsilon, accept);
      fragments.push({start, accept});
      continue;
    }

    if (token == "*") {
      if (fragments.empty()) {
        throw std::runtime_error("malformed postfix RE around '*'");
      }
      Fragment inner = fragments.top();
      fragments.pop();
      node* start = createState(false);
      node* accept = createState(true);
      inner.accept->SetAccept(false);
      start->Addoutstate(kEpsilon, inner.start);
      start->Addoutstate(kEpsilon, accept);
      inner.accept->Addoutstate(kEpsilon, inner.start);
      inner.accept->Addoutstate(kEpsilon, accept);
      fragments.push({start, accept});
      continue;
    }

    throw std::runtime_error("unknown postfix RE token: " + token);
  }

  if (fragments.size() != 1) {
    throw std::runtime_error("postfix RE did not reduce to a single NFA");
  }

  const Fragment built_fragment = fragments.top();
  // 接受态同时记录动作和原始优先级，这样确定化阶段仍能实现
  // 词法分析中的“最长匹配后再按先出现规则优先”冲突消解。
  nfaterstatetoaction[built_fragment.accept->GetState()] = action;
  nfaPriorityTableInternal[built_fragment.accept->GetState()] = priority;

  nfa automaton {};
  automaton.start = built_fragment.start;
  automaton.terminal.push_back(built_fragment.accept);
  return automaton;
}

/**
 * @brief 将多条规则生成的 NFA 通过新起点的 epsilon 边合并为总 NFA。
 */
nfa NFABuilder::mergeNFA(const std::vector<nfa>& automata) const {
  nfa merged {};
  merged.start = createState(false);
  // 所有规则 NFA 都挂到新的统一起点下，后续确定化即可把它们视为一个整体。
  for (const nfa& automaton : automata) {
    merged.start->Addoutstate(kEpsilon, automaton.start);
    for (node* terminal : automaton.terminal) {
      merged.terminal.push_back(terminal);
    }
  }
  return merged;
}

}  // 命名空间 seu_lex
