/**
 * @file nfa_constructor.cpp
 * @brief Infix-to-postfix conversion and Thompson NFA construction.
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

struct Fragment {
  node* start = nullptr;
  node* accept = nullptr;
};

std::vector<std::string> splitSpaceTokens(const std::string& text) {
  std::stringstream stream(text);
  std::vector<std::string> tokens;
  std::string token;
  while (stream >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

}  // namespace

std::string NFABuilder::toPostfix(const std::string& infix) const {
  // The RE expander emits a space-separated explicit infix language. Here we
  // only need shunting-yard over `|`, explicit concatenation `&`, and `*`.
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
      // Thompson concatenation: wire lhs accept state into rhs start with
      // epsilon so only one fragment boundary remains on the stack.
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
  // Accept states keep both the user action and the original rule priority so
  // DFA determinization can still implement Lex's first-rule tie-breaking.
  nfaterstatetoaction[built_fragment.accept->GetState()] = action;
  nfaPriorityTableInternal[built_fragment.accept->GetState()] = priority;

  nfa automaton {};
  automaton.start = built_fragment.start;
  automaton.terminal.push_back(built_fragment.accept);
  return automaton;
}

nfa NFABuilder::mergeNFA(const std::vector<nfa>& automata) const {
  nfa merged {};
  merged.start = createState(false);
  // All rule NFAs are connected under one fresh start state with epsilon edges.
  for (const nfa& automaton : automata) {
    merged.start->Addoutstate(kEpsilon, automaton.start);
    for (node* terminal : automaton.terminal) {
      merged.terminal.push_back(terminal);
    }
  }
  return merged;
}

}  // namespace seu_lex
