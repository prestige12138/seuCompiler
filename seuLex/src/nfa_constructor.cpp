#include "nfa_constructor.h"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <limits>
#include <memory>
#include <queue>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace seu_lex {

std::set<char> char_set;
std::map<std::string, std::string> idreTable;
std::vector<nfa> nfaTable;
std::vector<node*> dfaterminals;
std::map<int, std::string> nfaterstatetoaction;
std::map<int, std::string> TerStateActionTable;
std::map<int, std::string> mindfareturn;

namespace {

constexpr char kEpsilon = '\0';
constexpr int kAsciiLimit = 128;
constexpr int kMaxRepeatBound = 10000;

int g_nextStateLabel = 1;
std::vector<std::unique_ptr<node>> g_nodeArena;
std::map<int, std::size_t> g_nfaPriorityTable;

bool isIdentifierLike(const std::string& text) {
  if (text.empty()) {
    return false;
  }
  if (!(std::isalpha(static_cast<unsigned char>(text.front())) || text.front() == '_')) {
    return false;
  }
  return std::all_of(text.begin() + 1, text.end(), [](unsigned char ch) {
    return std::isalnum(ch) != 0 || ch == '_';
  });
}

std::vector<std::string> splitSpaceTokens(const std::string& text) {
  std::stringstream ss(text);
  std::vector<std::string> tokens;
  std::string token;
  while (ss >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

std::string joinKey(const std::vector<int>& ids) {
  std::ostringstream oss;
  for (std::size_t i = 0; i < ids.size(); ++i) {
    if (i != 0) {
      oss << ',';
    }
    oss << ids[i];
  }
  return oss.str();
}

node* createState(bool accepted = false) {
  g_nodeArena.push_back(std::make_unique<node>(g_nextStateLabel++, accepted));
  return g_nodeArena.back().get();
}

std::vector<char> buildAsciiUniverse(bool includeNewline) {
  std::vector<char> chars;
  for (int value = 1; value < kAsciiLimit; ++value) {
    const char ch = static_cast<char>(value);
    if (!includeNewline && ch == '\n') {
      continue;
    }
    chars.push_back(ch);
  }
  return chars;
}

char decodeEscape(const std::string& text, std::size_t& index) {
  if (index >= text.size()) {
    throw std::runtime_error("dangling escape in regular expression");
  }
  const char escaped = text[index++];
  switch (escaped) {
    case 'n':
      return '\n';
    case 't':
      return '\t';
    case 'r':
      return '\r';
    case 'f':
      return '\f';
    case 'v':
      return '\v';
    case '\\':
      return '\\';
    case '"':
      return '"';
    case '\'':
      return '\'';
    case '0':
      return '\0';
    default:
      return escaped;
  }
}

struct RegexAst {
  enum class Kind { kLiteral, kEpsilon, kConcat, kUnion, kStar, kSet };

  Kind kind;
  char literal = '\0';
  std::vector<char> charset;
  std::unique_ptr<RegexAst> left;
  std::unique_ptr<RegexAst> right;
};

std::unique_ptr<RegexAst> makeLiteral(char ch) {
  auto ast = std::make_unique<RegexAst>();
  ast->kind = RegexAst::Kind::kLiteral;
  ast->literal = ch;
  return ast;
}

std::unique_ptr<RegexAst> makeEpsilon() {
  auto ast = std::make_unique<RegexAst>();
  ast->kind = RegexAst::Kind::kEpsilon;
  return ast;
}

std::unique_ptr<RegexAst> makeSet(std::vector<char> charset) {
  auto ast = std::make_unique<RegexAst>();
  ast->kind = RegexAst::Kind::kSet;
  std::sort(charset.begin(), charset.end(), [](char lhs, char rhs) {
    return static_cast<unsigned char>(lhs) < static_cast<unsigned char>(rhs);
  });
  charset.erase(std::unique(charset.begin(), charset.end()), charset.end());
  ast->charset = std::move(charset);
  return ast;
}

std::unique_ptr<RegexAst> makeConcat(std::unique_ptr<RegexAst> lhs,
                                     std::unique_ptr<RegexAst> rhs) {
  auto ast = std::make_unique<RegexAst>();
  ast->kind = RegexAst::Kind::kConcat;
  ast->left = std::move(lhs);
  ast->right = std::move(rhs);
  return ast;
}

std::unique_ptr<RegexAst> makeUnion(std::unique_ptr<RegexAst> lhs,
                                    std::unique_ptr<RegexAst> rhs) {
  auto ast = std::make_unique<RegexAst>();
  ast->kind = RegexAst::Kind::kUnion;
  ast->left = std::move(lhs);
  ast->right = std::move(rhs);
  return ast;
}

std::unique_ptr<RegexAst> makeStar(std::unique_ptr<RegexAst> expr) {
  auto ast = std::make_unique<RegexAst>();
  ast->kind = RegexAst::Kind::kStar;
  ast->left = std::move(expr);
  return ast;
}

std::unique_ptr<RegexAst> cloneAst(const RegexAst& ast) {
  auto copy = std::make_unique<RegexAst>();
  copy->kind = ast.kind;
  copy->literal = ast.literal;
  copy->charset = ast.charset;
  if (ast.left) {
    copy->left = cloneAst(*ast.left);
  }
  if (ast.right) {
    copy->right = cloneAst(*ast.right);
  }
  return copy;
}

std::string expandNamedDefinitions(const std::string& raw,
                                   std::unordered_set<std::string>& recursionGuard) {
  std::ostringstream oss;
  bool inQuote = false;
  bool inClass = false;
  bool escaping = false;
  for (std::size_t i = 0; i < raw.size(); ++i) {
    const char ch = raw[i];
    if (escaping) {
      oss << ch;
      escaping = false;
      continue;
    }
    if (ch == '\\') {
      oss << ch;
      escaping = true;
      continue;
    }
    if (!inClass && ch == '"') {
      inQuote = !inQuote;
      oss << ch;
      continue;
    }
    if (!inQuote && ch == '[') {
      inClass = true;
      oss << ch;
      continue;
    }
    if (!inQuote && ch == ']') {
      inClass = false;
      oss << ch;
      continue;
    }
    if (!inQuote && !inClass && ch == '{') {
      const std::size_t close = raw.find('}', i + 1);
      if (close == std::string::npos) {
        throw std::runtime_error("unmatched '{' in regular expression: " + raw);
      }
      const std::string body = raw.substr(i + 1, close - i - 1);
      if (isIdentifierLike(body) && idreTable.count(body) != 0) {
        if (recursionGuard.count(body) != 0) {
          throw std::runtime_error("cyclic regular definition reference: " + body);
        }
        recursionGuard.insert(body);
        oss << "( " << expandNamedDefinitions(idreTable.at(body), recursionGuard) << " )";
        recursionGuard.erase(body);
        i = close;
        continue;
      }
      if (isIdentifierLike(body) && idreTable.count(body) == 0) {
        throw std::runtime_error("undefined regular definition reference: " + body);
      }
    }
    oss << ch;
  }
  return oss.str();
}

class ExtendedRegexParser {
 public:
  explicit ExtendedRegexParser(std::string text) : text_(std::move(text)) {}

  std::unique_ptr<RegexAst> parse() {
    auto result = parseUnion();
    skipSpace();
    if (pos_ != text_.size()) {
      throw std::runtime_error("unexpected trailing regex input near: " + text_.substr(pos_));
    }
    return result;
  }

 private:
  std::unique_ptr<RegexAst> parseUnion() {
    auto lhs = parseConcat();
    skipSpace();
    while (peek() == '|') {
      ++pos_;
      auto rhs = parseConcat();
      lhs = makeUnion(std::move(lhs), std::move(rhs));
      skipSpace();
    }
    return lhs;
  }

  std::unique_ptr<RegexAst> parseConcat() {
    skipSpace();
    std::vector<std::unique_ptr<RegexAst>> parts;
    while (canStartPrimary(peek())) {
      parts.push_back(parseRepeat());
      skipSpace();
    }
    if (parts.empty()) {
      throw std::runtime_error("missing operand in regular expression");
    }
    auto result = std::move(parts.front());
    for (std::size_t i = 1; i < parts.size(); ++i) {
      result = makeConcat(std::move(result), std::move(parts[i]));
    }
    return result;
  }

  std::unique_ptr<RegexAst> parseRepeat() {
    auto base = parsePrimary();
    skipSpace();
    while (true) {
      if (peek() == '*') {
        ++pos_;
        base = makeStar(std::move(base));
      } else if (peek() == '+') {
        ++pos_;
        auto duplicated = cloneAst(*base);
        base = makeConcat(std::move(base), makeStar(std::move(duplicated)));
      } else if (peek() == '?') {
        ++pos_;
        base = makeUnion(std::move(base), makeEpsilon());
      } else if (peek() == '{') {
        base = parseBoundedRepeat(std::move(base));
      } else {
        break;
      }
      skipSpace();
    }
    return base;
  }

  std::unique_ptr<RegexAst> parseBoundedRepeat(std::unique_ptr<RegexAst> base) {
    expect('{');
    skipSpace();
    const int lower = parseInteger();
    skipSpace();
    int upper = lower;
    bool openEnded = false;
    if (peek() == ',') {
      ++pos_;
      skipSpace();
      if (peek() == '}') {
        openEnded = true;
      } else {
        upper = parseInteger();
      }
    }
    skipSpace();
    expect('}');
    if (!openEnded && upper < lower) {
      throw std::runtime_error("invalid repetition bound: upper < lower");
    }
    if (lower > kMaxRepeatBound || (!openEnded && upper > kMaxRepeatBound)) {
      throw std::runtime_error("repetition bound exceeds implementation limit");
    }

    std::unique_ptr<RegexAst> result;
    if (lower == 0) {
      result = makeEpsilon();
    } else {
      result = cloneAst(*base);
      for (int i = 1; i < lower; ++i) {
        result = makeConcat(std::move(result), cloneAst(*base));
      }
    }

    if (openEnded) {
      auto starTail = makeStar(cloneAst(*base));
      return lower == 0 ? std::move(starTail) : makeConcat(std::move(result), std::move(starTail));
    }

    for (int i = lower; i < upper; ++i) {
      auto optional = makeUnion(cloneAst(*base), makeEpsilon());
      result = result ? makeConcat(std::move(result), std::move(optional)) : std::move(optional);
    }
    return result ? std::move(result) : makeEpsilon();
  }

  std::unique_ptr<RegexAst> parsePrimary() {
    skipSpace();
    const char ch = peek();
    if (ch == '(') {
      ++pos_;
      auto grouped = parseUnion();
      skipSpace();
      expect(')');
      return grouped;
    }
    if (ch == '"') {
      return parseQuotedString();
    }
    if (ch == '[') {
      return parseCharacterClass();
    }
    if (ch == '.') {
      ++pos_;
      return makeSet(buildAsciiUniverse(false));
    }
    if (ch == '\\') {
      ++pos_;
      return makeLiteral(decodeEscape(text_, pos_));
    }
    if (ch == '\0') {
      throw std::runtime_error("unexpected end of regular expression");
    }
    if (ch == ')' || ch == '|' || ch == '*' || ch == '+' || ch == '?' || ch == '}') {
      throw std::runtime_error(std::string("unexpected operator in regular expression: ") + ch);
    }
    ++pos_;
    return makeLiteral(ch);
  }

  std::unique_ptr<RegexAst> parseQuotedString() {
    expect('"');
    std::vector<char> chars;
    while (true) {
      if (pos_ >= text_.size()) {
        throw std::runtime_error("unterminated quoted string in regular expression");
      }
      if (text_[pos_] == '"') {
        ++pos_;
        break;
      }
      if (text_[pos_] == '\\') {
        ++pos_;
        chars.push_back(decodeEscape(text_, pos_));
      } else {
        chars.push_back(text_[pos_++]);
      }
    }
    if (chars.empty()) {
      return makeEpsilon();
    }
    auto result = makeLiteral(chars.front());
    for (std::size_t i = 1; i < chars.size(); ++i) {
      result = makeConcat(std::move(result), makeLiteral(chars[i]));
    }
    return result;
  }

  std::unique_ptr<RegexAst> parseCharacterClass() {
    expect('[');
    bool negated = false;
    if (peek() == '^') {
      negated = true;
      ++pos_;
    }
    std::vector<char> chars;
    bool first = true;
    while (true) {
      if (pos_ >= text_.size()) {
        throw std::runtime_error("unterminated character class in regular expression");
      }
      if (text_[pos_] == ']' && !first) {
        ++pos_;
        break;
      }
      first = false;
      char left = '\0';
      if (text_[pos_] == '\\') {
        ++pos_;
        left = decodeEscape(text_, pos_);
      } else {
        left = text_[pos_++];
      }
      if (peek() == '-' && pos_ + 1 < text_.size() && text_[pos_ + 1] != ']') {
        ++pos_;
        char right = '\0';
        if (text_[pos_] == '\\') {
          ++pos_;
          right = decodeEscape(text_, pos_);
        } else {
          right = text_[pos_++];
        }
        if (static_cast<unsigned char>(left) > static_cast<unsigned char>(right)) {
          throw std::runtime_error("invalid character range in regular expression");
        }
        for (unsigned char value = static_cast<unsigned char>(left);
             value <= static_cast<unsigned char>(right);
             ++value) {
          chars.push_back(static_cast<char>(value));
        }
      } else {
        chars.push_back(left);
      }
    }
    if (negated) {
      std::unordered_set<unsigned char> excluded;
      for (char ch : chars) {
        excluded.insert(static_cast<unsigned char>(ch));
      }
      std::vector<char> complement;
      for (char ch : buildAsciiUniverse(true)) {
        if (excluded.count(static_cast<unsigned char>(ch)) == 0) {
          complement.push_back(ch);
        }
      }
      return makeSet(std::move(complement));
    }
    return makeSet(std::move(chars));
  }

  int parseInteger() {
    if (!std::isdigit(static_cast<unsigned char>(peek()))) {
      throw std::runtime_error("expected integer in repetition bound");
    }
    int value = 0;
    while (std::isdigit(static_cast<unsigned char>(peek())) != 0) {
      const int digit = text_[pos_++] - '0';
      if (value > (std::numeric_limits<int>::max() - digit) / 10) {
        throw std::runtime_error("repetition bound integer overflow");
      }
      value = value * 10 + digit;
    }
    return value;
  }

  static bool canStartPrimary(char ch) {
    return ch != '\0' && ch != ')' && ch != '|';
  }

  void skipSpace() {
    while (pos_ < text_.size() && std::isspace(static_cast<unsigned char>(text_[pos_])) != 0) {
      ++pos_;
    }
  }

  void expect(char expected) {
    if (peek() != expected) {
      throw std::runtime_error(std::string("expected '") + expected + "' in regular expression");
    }
    ++pos_;
  }

  char peek() const {
    if (pos_ >= text_.size()) {
      return '\0';
    }
    return text_[pos_];
  }

  std::string text_;
  std::size_t pos_ = 0;
};

std::string serializeAst(const RegexAst& ast) {
  switch (ast.kind) {
    case RegexAst::Kind::kLiteral:
      return "ch:" + std::to_string(static_cast<int>(static_cast<unsigned char>(ast.literal)));
    case RegexAst::Kind::kEpsilon:
      return "eps";
    case RegexAst::Kind::kSet: {
      if (ast.charset.empty()) {
        return "eps";
      }
      std::ostringstream oss;
      oss << "( ";
      for (std::size_t i = 0; i < ast.charset.size(); ++i) {
        if (i != 0) {
          oss << " | ";
        }
        oss << "ch:" << static_cast<int>(static_cast<unsigned char>(ast.charset[i]));
      }
      oss << " )";
      return oss.str();
    }
    case RegexAst::Kind::kConcat:
      return "( " + serializeAst(*ast.left) + " & " + serializeAst(*ast.right) + " )";
    case RegexAst::Kind::kUnion:
      return "( " + serializeAst(*ast.left) + " | " + serializeAst(*ast.right) + " )";
    case RegexAst::Kind::kStar:
      return "( " + serializeAst(*ast.left) + " * )";
  }
  throw std::runtime_error("unknown regex AST kind");
}

struct Fragment {
  node* start = nullptr;
  node* accept = nullptr;
};

std::string pickActionFromSet(const std::set<node*>& states) {
  std::size_t bestPriority = std::numeric_limits<std::size_t>::max();
  std::string chosen;
  for (node* state : states) {
    if (!state->IsAccepted()) {
      continue;
    }
    const int label = state->GetState();
    const std::size_t priority = g_nfaPriorityTable.at(label);
    if (priority < bestPriority) {
      bestPriority = priority;
      chosen = nfaterstatetoaction.at(label);
    }
  }
  return chosen;
}

}  // namespace

node::node() : label(0), accepted(false) {}

node::node(int state, bool accepttag) : label(state), accepted(accepttag) {}

void node::Addoutstate(char ch, node* nd) {
  outstate.insert({ch, nd});
}

bool node::IsAccepted() {
  return accepted;
}

bool node::IsAccepted() const {
  return accepted;
}

void node::SetAccept(bool tag) {
  accepted = tag;
}

mulit node::GetNextStates(char ch) {
  return outstate.find(ch);
}

int node::GetState() {
  return label;
}

int node::GetState() const {
  return label;
}

std::multimap<char, node*> node::getMultimap() {
  return outstate;
}

std::multimap<char, node*> node::getMultimap() const {
  return outstate;
}

void node::setNextState(myMul next) {
  outstate = next;
}

void node::Setstate(int state) {
  label = state;
}

dfa::dfa(node* st) : start(st) {}

void dfa::Eclosure(std::set<node*>& x) {
  std::queue<node*> work;
  for (node* item : x) {
    work.push(item);
  }
  while (!work.empty()) {
    node* current = work.front();
    work.pop();
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

void resetGlobalTables() {
  char_set.clear();
  idreTable.clear();
  nfaTable.clear();
  dfaterminals.clear();
  nfaterstatetoaction.clear();
  TerStateActionTable.clear();
  mindfareturn.clear();
  g_nfaPriorityTable.clear();
  g_nodeArena.clear();
  g_nextStateLabel = 1;
}

std::string REExpander::expandRE(const std::string& raw) const {
  std::unordered_set<std::string> recursionGuard;
  const std::string expanded = expandNamedDefinitions(raw, recursionGuard);
  ExtendedRegexParser parser(expanded);
  return serializeAst(*parser.parse());
}

std::string NFABuilder::toPostfix(const std::string& infix) const {
  const std::vector<std::string> tokens = splitSpaceTokens(infix);
  std::vector<std::string> output;
  std::stack<std::string> operators;
  auto precedence = [](const std::string& token) {
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
    } else if (token == "*") {
      output.push_back(token);
    } else if (token == "(") {
      operators.push(token);
    } else if (token == ")") {
      while (!operators.empty() && operators.top() != "(") {
        output.push_back(operators.top());
        operators.pop();
      }
      if (operators.empty()) {
        throw std::runtime_error("mismatched parentheses in RE");
      }
      operators.pop();
    } else if (token == "|" || token == "&") {
      while (!operators.empty() && operators.top() != "(" &&
             precedence(operators.top()) >= precedence(token)) {
        output.push_back(operators.top());
        operators.pop();
      }
      operators.push(token);
    } else {
      throw std::runtime_error("unknown infix RE token: " + token);
    }
  }
  while (!operators.empty()) {
    if (operators.top() == "(") {
      throw std::runtime_error("mismatched parentheses in RE");
    }
    output.push_back(operators.top());
    operators.pop();
  }
  std::ostringstream oss;
  for (std::size_t i = 0; i < output.size(); ++i) {
    if (i != 0) {
      oss << ' ';
    }
    oss << output[i];
  }
  return oss.str();
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
    } else if (token.rfind("ch:", 0) == 0) {
      const int ascii = std::stoi(token.substr(3));
      if (ascii == 0 || ascii >= kAsciiLimit) {
        throw std::runtime_error("literal out of supported ASCII range");
      }
      const char ch = static_cast<char>(ascii);
      char_set.insert(ch);
      node* start = createState(false);
      node* accept = createState(true);
      start->Addoutstate(ch, accept);
      fragments.push({start, accept});
    } else if (token == "&") {
      if (fragments.size() < 2) {
        throw std::runtime_error("malformed postfix RE around '&'");
      }
      Fragment rhs = fragments.top();
      fragments.pop();
      Fragment lhs = fragments.top();
      fragments.pop();
      lhs.accept->SetAccept(false);
      lhs.accept->Addoutstate(kEpsilon, rhs.start);
      fragments.push({lhs.start, rhs.accept});
    } else if (token == "|") {
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
    } else if (token == "*") {
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
    } else {
      throw std::runtime_error("unknown postfix RE token: " + token);
    }
  }
  if (fragments.size() != 1) {
    throw std::runtime_error("postfix RE did not reduce to a single NFA");
  }

  Fragment result = fragments.top();
  nfaterstatetoaction[result.accept->GetState()] = action;
  g_nfaPriorityTable[result.accept->GetState()] = priority;
  nfa built;
  built.start = result.start;
  built.terminal.push_back(result.accept);
  return built;
}

nfa NFABuilder::mergeNFA(const std::vector<nfa>& automata) const {
  node* start = createState(false);
  nfa merged;
  merged.start = start;
  for (const nfa& item : automata) {
    start->Addoutstate(kEpsilon, item.start);
    for (node* terminal : item.terminal) {
      merged.terminal.push_back(terminal);
    }
  }
  return merged;
}

dfa DFABuilder::subsetConstruct(const nfa& automaton) const {
  std::set<node*> startSet = {automaton.start};
  dfa helper;
  helper.Eclosure(startSet);

  std::vector<std::set<node*>> stateSets;
  std::vector<std::map<char, int>> transitions;
  std::vector<std::string> acceptActions;
  std::unordered_map<std::string, int> stateIndex;
  std::queue<int> work;

  auto keyFromSet = [](const std::set<node*>& states) {
    std::vector<int> ids;
    ids.reserve(states.size());
    for (node* item : states) {
      ids.push_back(item->GetState());
    }
    return joinKey(ids);
  };

  stateSets.push_back(startSet);
  transitions.emplace_back();
  acceptActions.push_back(pickActionFromSet(startSet));
  stateIndex[keyFromSet(startSet)] = 0;
  work.push(0);

  while (!work.empty()) {
    const int currentId = work.front();
    work.pop();
    for (char symbol : char_set) {
      std::set<node*> moved;
      for (node* state : stateSets[currentId]) {
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
      int targetId = -1;
      const auto found = stateIndex.find(key);
      if (found == stateIndex.end()) {
        targetId = static_cast<int>(stateSets.size());
        stateIndex[key] = targetId;
        stateSets.push_back(moved);
        transitions.emplace_back();
        acceptActions.push_back(pickActionFromSet(moved));
        work.push(targetId);
      } else {
        targetId = found->second;
      }
      transitions[currentId][symbol] = targetId;
    }
  }

  dfa result;
  result.nodeVec.resize(stateSets.size());
  result.endNode.clear();
  dfaterminals.clear();
  TerStateActionTable.clear();
  for (std::size_t stateId = 0; stateId < result.nodeVec.size(); ++stateId) {
    result.nodeVec[stateId].Setstate(static_cast<int>(stateId));
    result.nodeVec[stateId].SetAccept(!acceptActions[stateId].empty());
  }
  for (std::size_t stateId = 0; stateId < transitions.size(); ++stateId) {
    for (const auto& entry : transitions[stateId]) {
      result.nodeVec[stateId].Addoutstate(entry.first, &result.nodeVec[entry.second]);
    }
  }
  if (!result.nodeVec.empty()) {
    result.start = &result.nodeVec.front();
  }
  for (std::size_t stateId = 0; stateId < acceptActions.size(); ++stateId) {
    if (!acceptActions[stateId].empty()) {
      TerStateActionTable[static_cast<int>(stateId)] = acceptActions[stateId];
      dfaterminals.push_back(&result.nodeVec[stateId]);
      result.endNode.push_back(result.nodeVec[stateId]);
    }
  }
  return result;
}

}  // namespace seu_lex
