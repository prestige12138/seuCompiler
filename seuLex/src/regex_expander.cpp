/**
 * @file regex_expander.cpp
 * @brief Expansion of Lex-style extended RE syntax into the explicit token
 *        stream consumed by Thompson construction.
 */

#include "regex_expander.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include "nfa.h"

namespace seu_lex {
namespace {

constexpr int kAsciiLimit = 128;
constexpr int kMaxRepeatBound = 10000;

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

std::vector<char> buildAsciiUniverse(bool include_newline) {
  std::vector<char> chars;
  for (int value = 1; value < kAsciiLimit; ++value) {
    const char ch = static_cast<char>(value);
    if (!include_newline && ch == '\n') {
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

// The extended RE parser first lowers every feature into a small AST and only
// then serializes that AST into the explicit infix language expected by the
// postfix/NFA stages.

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
                                   std::unordered_set<std::string>& recursion_guard) {
  std::ostringstream oss;
  bool in_quote = false;
  bool in_class = false;
  bool escaping = false;
  for (std::size_t index = 0; index < raw.size(); ++index) {
    const char ch = raw[index];
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
    if (!in_class && ch == '"') {
      in_quote = !in_quote;
      oss << ch;
      continue;
    }
    if (!in_quote && ch == '[') {
      in_class = true;
      oss << ch;
      continue;
    }
    if (!in_quote && ch == ']') {
      in_class = false;
      oss << ch;
      continue;
    }
    if (!in_quote && !in_class && ch == '{') {
      const std::size_t close = raw.find('}', index + 1);
      if (close == std::string::npos) {
        throw std::runtime_error("unmatched '{' in regular expression: " + raw);
      }
      const std::string body = raw.substr(index + 1, close - index - 1);
      if (isIdentifierLike(body) && idreTable.count(body) != 0) {
        if (recursion_guard.count(body) != 0) {
          throw std::runtime_error("cyclic regular definition reference: " + body);
        }
        recursion_guard.insert(body);
        // Definitions are expanded recursively, but each reference is wrapped
        // in parentheses so precedence is preserved after substitution.
        oss << "( " << expandNamedDefinitions(idreTable.at(body), recursion_guard) << " )";
        recursion_guard.erase(body);
        index = close;
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
    auto parsed = parseUnion();
    skipSpace();
    if (pos_ != text_.size()) {
      throw std::runtime_error("unexpected trailing input in regular expression");
    }
    return parsed;
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
    for (std::size_t index = 1; index < parts.size(); ++index) {
      result = makeConcat(std::move(result), std::move(parts[index]));
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
        // `r+` is lowered to `r r*`, which lets the NFA stage stay small and
        // only implement concatenation, union, star, and epsilon.
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
    bool open_ended = false;
    if (peek() == ',') {
      ++pos_;
      skipSpace();
      if (peek() == '}') {
        open_ended = true;
      } else {
        upper = parseInteger();
      }
    }
    skipSpace();
    expect('}');
    if (!open_ended && upper < lower) {
      throw std::runtime_error("invalid repetition bound: upper < lower");
    }
    if (lower > kMaxRepeatBound || (!open_ended && upper > kMaxRepeatBound)) {
      throw std::runtime_error("repetition bound exceeds implementation limit");
    }

    // Bounded repetition is desugared into concatenation plus optional tails.
    // This keeps the downstream NFA builder independent from `{m,n}` syntax.
    std::unique_ptr<RegexAst> result;
    if (lower == 0) {
      result = makeEpsilon();
    } else {
      result = cloneAst(*base);
      for (int repeat = 1; repeat < lower; ++repeat) {
        result = makeConcat(std::move(result), cloneAst(*base));
      }
    }

    if (open_ended) {
      auto star_tail = makeStar(cloneAst(*base));
      return lower == 0 ? std::move(star_tail) : makeConcat(std::move(result), std::move(star_tail));
    }

    for (int repeat = lower; repeat < upper; ++repeat) {
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
    for (std::size_t index = 1; index < chars.size(); ++index) {
      result = makeConcat(std::move(result), makeLiteral(chars[index]));
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
      // Character classes are lowered into an explicit union of literals so
      // the postfix/NFA stages never need a dedicated "set" operator.
      std::ostringstream oss;
      oss << "( ";
      for (std::size_t index = 0; index < ast.charset.size(); ++index) {
        if (index != 0) {
          oss << " | ";
        }
        oss << "ch:" << static_cast<int>(static_cast<unsigned char>(ast.charset[index]));
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

}  // namespace

std::string REExpander::expandRE(const std::string& raw) const {
  std::unordered_set<std::string> recursion_guard;
  const std::string expanded = expandNamedDefinitions(raw, recursion_guard);
  ExtendedRegexParser parser(expanded);
  return serializeAst(*parser.parse());
}

}  // namespace seu_lex
