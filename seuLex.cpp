#include "lex_generator.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <optional>
#include <queue>
#include <sstream>
#include <stack>
#include <stdexcept>
#include <unordered_map>
#include <unordered_set>

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

int g_nextStateLabel = 1;
std::vector<std::unique_ptr<node>> g_nodeArena;
std::map<int, std::size_t> g_nfaPriorityTable;

std::string trim(const std::string& input) {
  const auto begin = std::find_if_not(input.begin(), input.end(), [](unsigned char ch) {
    return std::isspace(ch) != 0;
  });
  if (begin == input.end()) {
    return "";
  }
  const auto end = std::find_if_not(input.rbegin(), input.rend(), [](unsigned char ch) {
    return std::isspace(ch) != 0;
  }).base();
  return std::string(begin, end);
}

std::vector<std::string> splitByLines(const std::string& text) {
  std::vector<std::string> lines;
  std::stringstream ss(text);
  std::string line;
  while (std::getline(ss, line)) {
    lines.push_back(line);
  }
  if (!text.empty() && text.back() == '\n') {
    lines.emplace_back();
  }
  return lines;
}

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

std::string escapeDotLabel(char ch) {
  switch (ch) {
    case '\n':
      return "\\n";
    case '\t':
      return "\\t";
    case '\r':
      return "\\r";
    case '\f':
      return "\\f";
    case '\v':
      return "\\v";
    case '"':
      return "\\\"";
    case '\\':
      return "\\\\";
    case kEpsilon:
      return "eps";
    default:
      if (std::isprint(static_cast<unsigned char>(ch)) != 0) {
        return std::string(1, ch);
      }
      std::ostringstream oss;
      oss << "\\x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
          << (static_cast<int>(static_cast<unsigned char>(ch)));
      return oss.str();
  }
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

std::vector<std::string> splitSpaceTokens(const std::string& text) {
  std::stringstream ss(text);
  std::vector<std::string> tokens;
  std::string token;
  while (ss >> token) {
    tokens.push_back(token);
  }
  return tokens;
}

node* createState(bool accepted = false) {
  g_nodeArena.push_back(std::make_unique<node>(g_nextStateLabel++, accepted));
  return g_nodeArena.back().get();
}

void resetAutomataGlobals() {
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

std::string stripInlineComment(const std::string& line) {
  bool inQuote = false;
  for (std::size_t i = 0; i + 1 < line.size(); ++i) {
    if (line[i] == '"' && (i == 0 || line[i - 1] != '\\')) {
      inQuote = !inQuote;
    }
    if (!inQuote && line[i] == '/' && line[i + 1] == '*') {
      return line.substr(0, i);
    }
  }
  return line;
}

bool isActionBalanced(const std::string& action) {
  if (action.empty()) {
    return false;
  }
  if (action.front() != '{') {
    return true;
  }
  int braceDepth = 0;
  bool inString = false;
  bool inChar = false;
  bool escaping = false;
  for (std::size_t i = 0; i < action.size(); ++i) {
    const char ch = action[i];
    if (escaping) {
      escaping = false;
      continue;
    }
    if ((inString || inChar) && ch == '\\') {
      escaping = true;
      continue;
    }
    if (!inChar && ch == '"' && (i == 0 || action[i - 1] != '\\')) {
      inString = !inString;
      continue;
    }
    if (!inString && ch == '\'' && (i == 0 || action[i - 1] != '\\')) {
      inChar = !inChar;
      continue;
    }
    if (inString || inChar) {
      continue;
    }
    if (ch == '{') {
      ++braceDepth;
    } else if (ch == '}') {
      --braceDepth;
    }
  }
  return braceDepth == 0;
}

std::pair<std::string, std::string> splitRegexAndAction(const std::string& ruleText) {
  bool inQuote = false;
  bool inClass = false;
  bool escaping = false;
  std::size_t separator = std::string::npos;
  for (std::size_t i = 0; i < ruleText.size(); ++i) {
    const char ch = ruleText[i];
    if (escaping) {
      escaping = false;
      continue;
    }
    if (ch == '\\') {
      escaping = true;
      continue;
    }
    if (!inClass && ch == '"') {
      inQuote = !inQuote;
      continue;
    }
    if (!inQuote && ch == '[') {
      inClass = true;
      continue;
    }
    if (!inQuote && ch == ']') {
      inClass = false;
      continue;
    }
    if (!inQuote && !inClass && std::isspace(static_cast<unsigned char>(ch)) != 0) {
      separator = i;
      break;
    }
  }
  if (separator == std::string::npos) {
    return {trim(ruleText), ""};
  }
  return {trim(ruleText.substr(0, separator)), trim(ruleText.substr(separator))};
}

std::string readWholeFile(const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open file: " + path);
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

std::string takeBetweenMarkers(const std::string& text,
                               const std::string& left,
                               const std::string& right,
                               std::vector<std::pair<std::size_t, std::size_t>>* ranges) {
  std::ostringstream extracted;
  std::size_t searchPos = 0;
  while (true) {
    const std::size_t begin = text.find(left, searchPos);
    if (begin == std::string::npos) {
      break;
    }
    const std::size_t end = text.find(right, begin + left.size());
    if (end == std::string::npos) {
      throw std::runtime_error("unterminated marker block in definitions section");
    }
    extracted << text.substr(begin + left.size(), end - begin - left.size()) << '\n';
    if (ranges != nullptr) {
      ranges->push_back({begin, end + right.size()});
    }
    searchPos = end + right.size();
  }
  return extracted.str();
}

std::string removeRanges(const std::string& text,
                         const std::vector<std::pair<std::size_t, std::size_t>>& ranges) {
  if (ranges.empty()) {
    return text;
  }
  std::ostringstream oss;
  std::size_t cursor = 0;
  for (const auto& range : ranges) {
    if (cursor < range.first) {
      oss << text.substr(cursor, range.first - cursor);
    }
    cursor = range.second;
  }
  if (cursor < text.size()) {
    oss << text.substr(cursor);
  }
  return oss.str();
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
      value = value * 10 + (text_[pos_++] - '0');
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

std::string actionForState(int stateId) {
  if (mindfareturn.count(stateId) != 0) {
    return mindfareturn.at(stateId);
  }
  if (TerStateActionTable.count(stateId) != 0) {
    return TerStateActionTable.at(stateId);
  }
  return "";
}

int evaluateDFA(const dfa& automaton, const std::string& yytext) {
  if (automaton.start == nullptr) {
    return -1;
  }
  const node* current = automaton.start;
  for (unsigned char byte : yytext) {
    bool matched = false;
    const auto transitions = current->getMultimap();
    const auto range = transitions.equal_range(static_cast<char>(byte));
    for (auto it = range.first; it != range.second; ++it) {
      current = it->second;
      matched = true;
      break;
    }
    if (!matched) {
      return -1;
    }
  }
  if (!current->IsAccepted()) {
    return -1;
  }
  const std::string action = actionForState(current->GetState());
  if (action.find("return") == std::string::npos) {
    return 0;
  }
  std::size_t pos = action.find("return");
  pos += 6;
  while (pos < action.size() && std::isspace(static_cast<unsigned char>(action[pos])) != 0) {
    ++pos;
  }
  std::size_t end = pos;
  while (end < action.size() && (std::isalnum(static_cast<unsigned char>(action[end])) != 0 ||
                                 action[end] == '_' || action[end] == '(' || action[end] == ')')) {
    ++end;
  }
  const std::string token = trim(action.substr(pos, end - pos));
  if (token.empty()) {
    return 0;
  }
  if (std::all_of(token.begin(), token.end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0;
      })) {
    return std::stoi(token);
  }
  static std::unordered_map<std::string, int> tokenIds;
  static int nextTokenId = 1;
  auto [it, inserted] = tokenIds.emplace(token, nextTokenId);
  if (inserted) {
    ++nextTokenId;
  }
  return it->second;
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
  for (node* state : x) {
    work.push(state);
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
    for (const auto& [ch, target] : transitions) {
      std::cout << "  --" << escapeDotLabel(ch) << "--> " << target->GetState() << '\n';
    }
  }
}

LexSpecification LexParser::parseLexFile(const std::string& path) const {
  LexSpecification spec;
  const std::string content = readWholeFile(path);
  const std::size_t firstSep = content.find("\n%%");
  const std::size_t firstSepAlt = content.find("%%");
  std::size_t first = std::string::npos;
  if (firstSep == std::string::npos) {
    first = firstSepAlt;
  } else {
    first = firstSep + 1;
  }
  if (first == std::string::npos) {
    throw std::runtime_error("missing first %% section delimiter");
  }
  const std::size_t second = content.find("%%", first + 2);
  if (second == std::string::npos) {
    throw std::runtime_error("missing second %% section delimiter");
  }

  spec.definitionsSection = content.substr(0, first);
  spec.rulesSection = content.substr(first + 2, second - (first + 2));
  spec.userSubroutines = content.substr(second + 2);

  std::vector<std::pair<std::size_t, std::size_t>> verbatimRanges;
  spec.verbatimDefinitions =
      takeBetweenMarkers(spec.definitionsSection, "%{", "%}", &verbatimRanges);
  const std::string normalizedDefinitions = removeRanges(spec.definitionsSection, verbatimRanges);
  for (const std::string& rawLine : splitByLines(normalizedDefinitions)) {
    std::string line = trim(stripInlineComment(rawLine));
    if (line.empty()) {
      continue;
    }
    std::istringstream iss(line);
    std::string name;
    iss >> name;
    if (name.empty()) {
      continue;
    }
    std::string translation;
    std::getline(iss, translation);
    translation = trim(translation);
    if (!translation.empty()) {
      idreTable[name] = translation;
    }
  }

  const auto lines = splitByLines(spec.rulesSection);
  std::string pendingRule;
  std::size_t priority = 0;
  for (const std::string& rawLine : lines) {
    if (trim(rawLine).empty()) {
      continue;
    }
    if (!pendingRule.empty()) {
      pendingRule += '\n';
    }
    pendingRule += rawLine;
    const auto [regex, action] = splitRegexAndAction(pendingRule);
    if (regex.empty() || !isActionBalanced(action)) {
      continue;
    }
    LexRule rule;
    rule.regex = regex;
    rule.action = action;
    rule.priority = priority++;
    spec.rules.push_back(rule);
    pendingRule.clear();
  }
  if (!trim(pendingRule).empty()) {
    throw std::runtime_error("unterminated or malformed Lex rule near: " + pendingRule);
  }
  return spec;
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
  auto precedence = [](const std::string& token) -> int {
    if (token == "|") {
      return 1;
    }
    if (token == "&") {
      return 2;
    }
    return 0;
  };

  for (const std::string& token : tokens) {
    if (token == "ch:" || token.empty()) {
      continue;
    }
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
  acceptActions.emplace_back();
  stateIndex[keyFromSet(startSet)] = 0;
  work.push(0);

  auto pickAction = [](const std::set<node*>& states) -> std::string {
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
  };

  acceptActions[0] = pickAction(startSet);

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
      auto found = stateIndex.find(key);
      if (found == stateIndex.end()) {
        targetId = static_cast<int>(stateSets.size());
        stateIndex[key] = targetId;
        stateSets.push_back(moved);
        transitions.emplace_back();
        acceptActions.push_back(pickAction(moved));
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
    const bool accepted = !acceptActions[stateId].empty();
    result.nodeVec[stateId].SetAccept(accepted);
  }
  for (std::size_t stateId = 0; stateId < transitions.size(); ++stateId) {
    for (const auto& [symbol, target] : transitions[stateId]) {
      result.nodeVec[stateId].Addoutstate(symbol, &result.nodeVec[target]);
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
  for (auto& [_, states] : acceptingByAction) {
    partitions.push_back(states);
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
      for (auto& [_, states] : buckets) {
        refined.push_back(states);
      }
      if (buckets.size() > 1) {
        changed = true;
      }
    }
    partitions = std::move(refined);
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
    for (const auto& [symbol, target] : minimizedTransitions[stateId]) {
      result.nodeVec[stateId].Addoutstate(symbol, &result.nodeVec[target]);
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

void CodeGenerator::emitLexer(const dfa& automaton,
                              const LexSpecification& specification,
                              const std::string& outPath) const {
  if (automaton.start == nullptr) {
    throw std::runtime_error("cannot emit lexer from an empty DFA");
  }
  std::vector<std::array<int, kAsciiLimit>> table(automaton.nodeVec.size());
  for (auto& row : table) {
    row.fill(-1);
  }
  for (const node& state : automaton.nodeVec) {
    const auto transitions = state.getMultimap();
    for (const auto& [symbol, target] : transitions) {
      table[state.GetState()][static_cast<unsigned char>(symbol)] = target->GetState();
    }
  }

  std::ofstream output(outPath);
  if (!output) {
    throw std::runtime_error("failed to open generated lexer path: " + outPath);
  }

  output << "#include <array>\n"
         << "#include <iostream>\n"
         << "#include <string>\n"
         << "#include <vector>\n\n"
         << "using namespace std;\n\n"
         << specification.verbatimDefinitions << '\n'
         << "static string yytext_storage;\n"
         << "static char* yytext = nullptr;\n"
         << "static string yy_source;\n"
         << "static size_t yy_cursor = 0;\n"
         << "#ifndef ECHO\n"
         << "#define ECHO do { std::cout << yytext; } while (0)\n"
         << "#endif\n\n"
         << "int input() {\n"
         << "  if (yy_cursor >= yy_source.size()) {\n"
         << "    return 0;\n"
         << "  }\n"
         << "  return static_cast<unsigned char>(yy_source[yy_cursor++]);\n"
         << "}\n\n"
         << "static const int kStartState = " << automaton.start->GetState() << ";\n"
         << "static const vector<array<int, " << kAsciiLimit << ">> kTransitions = {\n";
  for (std::size_t row = 0; row < table.size(); ++row) {
    output << "  {";
    for (int col = 0; col < kAsciiLimit; ++col) {
      if (col != 0) {
        output << ", ";
      }
      output << table[row][col];
    }
    output << "}";
    if (row + 1 != table.size()) {
      output << ",";
    }
    output << '\n';
  }
  output << "};\n\n";

  output << "static const vector<int> kAcceptStates = {";
  for (std::size_t index = 0; index < automaton.nodeVec.size(); ++index) {
    if (index != 0) {
      output << ", ";
    }
    output << (actionForState(static_cast<int>(index)).empty() ? 0 : 1);
  }
  output << "};\n\n";

  output << specification.userSubroutines << '\n';

  output << "static int dispatch_action(int state) {\n"
         << "  switch (state) {\n";
  for (const auto& [stateId, action] : mindfareturn.empty() ? TerStateActionTable : mindfareturn) {
    output << "    case " << stateId << ":\n";
    if (trim(action) == ";" || trim(action).empty()) {
      output << "      return 0;\n";
    } else {
      output << "      do {\n"
             << "        " << action << "\n"
             << "      } while (false);\n"
             << "      return 0;\n";
    }
  }
  output << "    default:\n"
         << "      return -1;\n"
         << "  }\n"
         << "}\n\n"
         << "static void reset_source(const string& source) {\n"
         << "  yy_source = source;\n"
         << "  yy_cursor = 0;\n"
         << "}\n\n";

  output << "/**\n"
         << " * @brief Analyze one candidate lexeme with the minimized DFA.\n"
         << " *\n"
         << " * Complexity: O(|yytext|).\n"
         << " */\n"
         << "int analysis(string yytext) {\n"
         << "  yytext_storage = yytext;\n"
         << "  ::yytext = yytext_storage.data();\n"
         << "  int state = kStartState;\n"
         << "  for (unsigned char ch : yytext_storage) {\n"
         << "    if (ch >= " << kAsciiLimit << ") {\n"
         << "      return -1;\n"
         << "    }\n"
         << "    state = kTransitions[state][ch];\n"
         << "    if (state < 0) {\n"
         << "      return -1;\n"
         << "    }\n"
         << "  }\n"
         << "  return dispatch_action(state);\n"
         << "}\n\n"
         << "/**\n"
         << " * @brief Scan the next token from the current input stream.\n"
         << " *\n"
         << " * Complexity: O(L), where L is the matched lexeme length.\n"
         << " */\n"
         << "int next_token() {\n"
         << "  if (yy_cursor >= yy_source.size()) {\n"
         << "    return 0;\n"
         << "  }\n"
         << "  const size_t start = yy_cursor;\n"
         << "  size_t pos = yy_cursor;\n"
         << "  int state = kStartState;\n"
         << "  int last_accept_state = -1;\n"
         << "  size_t last_accept_pos = start;\n"
         << "  while (pos < yy_source.size()) {\n"
         << "    const unsigned char ch = static_cast<unsigned char>(yy_source[pos]);\n"
         << "    if (ch >= " << kAsciiLimit << ") {\n"
         << "      break;\n"
         << "    }\n"
         << "    const int next = kTransitions[state][ch];\n"
         << "    if (next < 0) {\n"
         << "      break;\n"
         << "    }\n"
         << "    state = next;\n"
         << "    ++pos;\n"
         << "    if (kAcceptStates[state] != 0) {\n"
         << "      last_accept_state = state;\n"
         << "      last_accept_pos = pos;\n"
         << "    }\n"
         << "  }\n"
         << "  if (last_accept_state < 0) {\n"
         << "    yytext_storage = yy_source.substr(start, 1);\n"
         << "    yytext = yytext_storage.data();\n"
         << "    ++yy_cursor;\n"
         << "    return -1;\n"
         << "  }\n"
         << "  yytext_storage = yy_source.substr(start, last_accept_pos - start);\n"
         << "  yytext = yytext_storage.data();\n"
         << "  yy_cursor = last_accept_pos;\n"
         << "  return dispatch_action(last_accept_state);\n"
         << "}\n\n"
         << "/**\n"
         << " * @brief Tokenize an entire source string with maximal munch.\n"
         << " *\n"
         << " * Complexity: O(N * A) in the worst case, where N is source length and\n"
         << " * A is the number of scanned DFA steps performed by maximal munch.\n"
         << " */\n"
         << "vector<int> tokenize(const string& source) {\n"
         << "  reset_source(source);\n"
         << "  vector<int> tokens;\n"
         << "  while (yy_cursor < yy_source.size()) {\n"
         << "    const int token = next_token();\n"
         << "    if (token == 0) {\n"
         << "      continue;\n"
         << "    }\n"
         << "    tokens.push_back(token);\n"
         << "  }\n"
         << "  return tokens;\n"
         << "}\n";
}

void Visualizer::dumpNFA(const nfa& automaton, const std::string& path) const {
  std::ofstream output(path);
  if (!output) {
    throw std::runtime_error("failed to write NFA dot file: " + path);
  }
  output << "digraph NFA {\n  rankdir=LR;\n";
  std::queue<node*> work;
  std::unordered_set<int> visited;
  work.push(automaton.start);
  visited.insert(automaton.start->GetState());
  while (!work.empty()) {
    node* current = work.front();
    work.pop();
    output << "  " << current->GetState()
           << " [shape=" << (current->IsAccepted() ? "doublecircle" : "circle") << "];\n";
    const auto transitions = current->getMultimap();
    for (const auto& [symbol, target] : transitions) {
      output << "  " << current->GetState() << " -> " << target->GetState() << " [label=\""
             << escapeDotLabel(symbol) << "\"];\n";
      if (visited.insert(target->GetState()).second) {
        work.push(target);
      }
    }
  }
  output << "}\n";
}

void Visualizer::dumpDFA(const dfa& automaton, const std::string& path) const {
  std::ofstream output(path);
  if (!output) {
    throw std::runtime_error("failed to write DFA dot file: " + path);
  }
  output << "digraph DFA {\n  rankdir=LR;\n";
  for (const node& state : automaton.nodeVec) {
    output << "  " << state.GetState()
           << " [shape=" << (state.IsAccepted() ? "doublecircle" : "circle") << "];\n";
    const auto transitions = state.getMultimap();
    for (const auto& [symbol, target] : transitions) {
      output << "  " << state.GetState() << " -> " << target->GetState() << " [label=\""
             << escapeDotLabel(symbol) << "\"];\n";
    }
  }
  output << "}\n";
}

void SeuLexDriver::generate(const std::string& lexPath,
                            const std::string& outCppPath,
                            const std::string& dotDir) const {
  resetAutomataGlobals();

  LexParser parser;
  REExpander expander;
  NFABuilder nfaBuilder;
  DFABuilder dfaBuilder;
  DFAMinimizer minimizer;
  CodeGenerator emitter;
  Visualizer visualizer;

  LexSpecification spec = parser.parseLexFile(lexPath);
  for (LexRule& rule : spec.rules) {
    rule.expandedRegex = expander.expandRE(rule.regex);
    rule.postfixRegex = nfaBuilder.toPostfix(rule.expandedRegex);
    nfaTable.push_back(nfaBuilder.buildNFA(rule.postfixRegex, rule.action, rule.priority));
  }

  const nfa mergedNfa = nfaBuilder.mergeNFA(nfaTable);
  const dfa rawDfa = dfaBuilder.subsetConstruct(mergedNfa);
  const dfa minDfa = minimizer.minimizeDFA(rawDfa);

  std::filesystem::create_directories(dotDir);
  visualizer.dumpNFA(mergedNfa, (std::filesystem::path(dotDir) / "merged_nfa.dot").string());
  visualizer.dumpDFA(rawDfa, (std::filesystem::path(dotDir) / "dfa.dot").string());
  visualizer.dumpDFA(minDfa, (std::filesystem::path(dotDir) / "min_dfa.dot").string());
  emitter.emitLexer(minDfa, spec, outCppPath);
}

bool SeuLexDriver::runSelfTests(const std::string& workspaceRoot) const {
  const std::filesystem::path tempDir =
      std::filesystem::path(workspaceRoot) / "tests" / "seuLex_runtime";
  std::filesystem::create_directories(tempDir);
  const std::filesystem::path specPath = tempDir / "sample.l";
  const std::filesystem::path outPath = tempDir / "generated_sample_lexer.cpp";
  const std::filesystem::path driverPath = tempDir / "generated_sample_driver.cpp";
  const std::filesystem::path binaryPath = tempDir / "generated_sample_driver";
  const std::filesystem::path dotDir = tempDir / "dot";

  const std::string sampleSpec =
      "%{\n"
      "#include <string>\n"
      "%}\n"
      "DIGIT [0-9]\n"
      "ALPHA [A-Za-z_]\n"
      "%%\n"
      "\"if\" { return 1; }\n"
      "\"else\" { return 2; }\n"
      "{ALPHA}({ALPHA}|{DIGIT})* { return 3; }\n"
      "{DIGIT}+ { return 4; }\n"
      "[ \\t\\n]+ ;\n"
      "%%\n"
      "int yywrap() { return 1; }\n";

  {
    std::ofstream sampleFile(specPath);
    sampleFile << sampleSpec;
  }

  generate(specPath.string(), outPath.string(), dotDir.string());
  {
    std::ofstream driverFile(driverPath);
    driverFile
        << "#include <iostream>\n"
        << "#include <string>\n"
        << "#include <vector>\n"
        << "int analysis(std::string yytext);\n"
        << "std::vector<int> tokenize(const std::string& source);\n"
        << "int main() {\n"
        << "  const int a = analysis(\"if\");\n"
        << "  const int b = analysis(\"abc123\");\n"
        << "  const int c = analysis(\"42\");\n"
        << "  const auto tokens = tokenize(\"if 42 abc123\");\n"
        << "  std::cout << a << ' ' << b << ' ' << c << ' ' << tokens.size() << '\\n';\n"
        << "  return (a == 1 && b == 3 && c == 4 && tokens.size() == 3 && tokens[0] == 1 && "
           "tokens[1] == 4 && tokens[2] == 3) ? 0 : 1;\n"
        << "}\n";
  }
  const std::string compileGeneratedCommand =
      "g++ -std=c++17 \"" + outPath.string() + "\" \"" + driverPath.string() + "\" -o \"" +
      binaryPath.string() + "\"";
  if (std::system(compileGeneratedCommand.c_str()) != 0) {
    throw std::runtime_error("generated sample lexer failed to compile");
  }
  if (std::system(binaryPath.string().c_str()) != 0) {
    throw std::runtime_error("generated sample lexer failed runtime validation");
  }

  resetAutomataGlobals();
  LexParser parser;
  REExpander expander;
  NFABuilder nfaBuilder;
  DFABuilder dfaBuilder;
  DFAMinimizer minimizer;

  LexSpecification spec = parser.parseLexFile(specPath.string());
  for (LexRule& rule : spec.rules) {
    rule.expandedRegex = expander.expandRE(rule.regex);
    rule.postfixRegex = nfaBuilder.toPostfix(rule.expandedRegex);
    nfaTable.push_back(nfaBuilder.buildNFA(rule.postfixRegex, rule.action, rule.priority));
  }

  const dfa minimized = minimizer.minimizeDFA(dfaBuilder.subsetConstruct(nfaBuilder.mergeNFA(nfaTable)));

  struct Expectation {
    std::string lexeme;
    int expected;
  };
  const std::vector<Expectation> cases = {
      {"if", 1},
      {"else", 2},
      {"abc123", 3},
      {"42", 4},
      {" \t", 0},
      {"@", -1},
  };

  bool allPassed = true;
  for (const auto& item : cases) {
    const int actual = evaluateDFA(minimized, item.lexeme);
    std::cout << "[self-test] " << std::quoted(item.lexeme) << " => " << actual
              << " (expected " << item.expected << ")\n";
    if (actual != item.expected) {
      allPassed = false;
    }
  }

  auto checkRegex = [&](const std::string& rawRegex,
                        const std::vector<Expectation>& localCases,
                        const std::map<std::string, std::string>& definitions = {}) {
    resetAutomataGlobals();
    idreTable = definitions;
    REExpander localExpander;
    NFABuilder localNfaBuilder;
    DFABuilder localDfaBuilder;
    DFAMinimizer localMinimizer;
    const std::string expanded = localExpander.expandRE(rawRegex);
    const std::string postfix = localNfaBuilder.toPostfix(expanded);
    const nfa localNfa = localNfaBuilder.buildNFA(postfix, "return 7;", 0);
    const dfa localDfa =
        localMinimizer.minimizeDFA(localDfaBuilder.subsetConstruct(localNfa));
    for (const auto& localCase : localCases) {
      const int actual = evaluateDFA(localDfa, localCase.lexeme);
      std::cout << "[regex-test] " << rawRegex << " / " << std::quoted(localCase.lexeme)
                << " => " << actual << " (expected " << localCase.expected << ")\n";
      if (actual != localCase.expected) {
        allPassed = false;
      }
    }
  };

  checkRegex(".", {{"a", 7}, {"\n", -1}});
  checkRegex("[^a-c]", {{"z", 7}, {"b", -1}});
  checkRegex("a?", {{"", 7}, {"a", 7}, {"aa", -1}});
  checkRegex("a{2,4}", {{"a", -1}, {"aa", 7}, {"aaa", 7}, {"aaaa", 7}, {"aaaaa", -1}});
  checkRegex("\"\"", {{"", 7}, {"x", -1}});
  checkRegex("\\\"", {{"\"", 7}, {"a", -1}});
  checkRegex("{DIGIT}{DIGIT}", {{"42", 7}, {"4", -1}}, {{"DIGIT", "[0-9]"}});

  auto expectExpandFailure = [&](const std::string& rawRegex,
                                 const std::map<std::string, std::string>& definitions = {}) {
    resetAutomataGlobals();
    idreTable = definitions;
    REExpander localExpander;
    bool failed = false;
    try {
      static_cast<void>(localExpander.expandRE(rawRegex));
    } catch (const std::exception&) {
      failed = true;
    }
    std::cout << "[regex-error] " << rawRegex << " => " << (failed ? "failed" : "unexpected-pass")
              << '\n';
    if (!failed) {
      allPassed = false;
    }
  };

  expectExpandFailure("|a");
  expectExpandFailure("a|");
  expectExpandFailure("a||b");
  expectExpandFailure("{MISSING}");

  const std::filesystem::path minicOutput = tempDir / "generated_minic_lexer.cpp";
  generate((std::filesystem::path(workspaceRoot) / "resources" / "minic.l").string(),
           minicOutput.string(),
           (tempDir / "minic_dot").string());
  std::cout << "[self-test] generated lexer: " << outPath << '\n';
  std::cout << "[self-test] generated minic lexer: " << minicOutput << '\n';
  return allPassed;
}

int main(int argc, char** argv) {
  try {
    SeuLexDriver driver;
    const std::filesystem::path workspaceRoot = std::filesystem::current_path();
    if (argc >= 2 && std::string(argv[1]) == "--self-test") {
      const bool passed = driver.runSelfTests(workspaceRoot.string());
      return passed ? 0 : 1;
    }
    if (argc < 2 || argc > 4) {
      std::cerr << "Usage: " << argv[0]
                << " <lex-file> [generated-lexer.cpp] [dot-output-dir]\n"
                << "   or: " << argv[0] << " --self-test\n";
      return 1;
    }
    const std::string lexPath = argv[1];
    const std::string outPath = argc >= 3 ? argv[2] : "generated_lexer.cpp";
    const std::string dotDir = argc >= 4 ? argv[3] : "dot";
    driver.generate(lexPath, outPath, dotDir);
    std::cout << "Generated lexer source: " << outPath << '\n'
              << "DOT visualizations in: " << dotDir << '\n';
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "seuLex error: " << ex.what() << '\n';
    return 1;
  }
}
