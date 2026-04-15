/**
 * @file lex_parser.cpp
 * @brief Lex three-section parsing and rule extraction implementation.
 */

#include "lex_parser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <vector>

#include "nfa.h"

namespace seu_lex {
namespace {

std::size_t findSectionDelimiter(const std::string& content, std::size_t startPos) {
  bool inVerbatim = false;
  bool lineStart = true;
  for (std::size_t index = startPos; index + 1 < content.size(); ++index) {
    // `%%` only splits sections when it appears at the beginning of a logical
    // line and outside `%{ ... %}` verbatim blocks.
    if (lineStart) {
      std::size_t marker = index;
      while (marker < content.size() &&
             (content[marker] == ' ' || content[marker] == '\t' || content[marker] == '\r')) {
        ++marker;
      }
      if (!inVerbatim && marker + 1 < content.size() && content[marker] == '%' &&
          content[marker + 1] == '%') {
        std::size_t tail = marker + 2;
        while (tail < content.size() &&
               (content[tail] == ' ' || content[tail] == '\t' || content[tail] == '\r')) {
          ++tail;
        }
        if (tail == content.size() || content[tail] == '\n') {
          return marker;
        }
      }
    }
    if (!inVerbatim && content[index] == '%' && content[index + 1] == '{') {
      inVerbatim = true;
      ++index;
      lineStart = false;
      continue;
    }
    if (inVerbatim && content[index] == '%' && content[index + 1] == '}') {
      inVerbatim = false;
      ++index;
      lineStart = false;
      continue;
    }
    lineStart = content[index] == '\n';
  }
  return std::string::npos;
}

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
  // Actions may contain strings, chars, or comments. The parser therefore
  // tracks lexical context instead of naively counting braces.
  int braceDepth = 0;
  bool inString = false;
  bool inChar = false;
  bool escaping = false;
  bool inLineComment = false;
  bool inBlockComment = false;
  for (std::size_t index = 0; index < action.size(); ++index) {
    const char ch = action[index];
    if (escaping) {
      escaping = false;
      continue;
    }
    if (inLineComment) {
      if (ch == '\n') {
        inLineComment = false;
      }
      continue;
    }
    if (inBlockComment) {
      if (ch == '*' && index + 1 < action.size() && action[index + 1] == '/') {
        inBlockComment = false;
        ++index;
      }
      continue;
    }
    if ((inString || inChar) && ch == '\\') {
      escaping = true;
      continue;
    }
    if (!inChar && ch == '"') {
      inString = !inString;
      continue;
    }
    if (!inString && ch == '\'') {
      inChar = !inChar;
      continue;
    }
    if (inString || inChar) {
      continue;
    }
    if (ch == '/' && index + 1 < action.size()) {
      if (action[index + 1] == '*') {
        inBlockComment = true;
        ++index;
        continue;
      }
      if (action[index + 1] == '/') {
        inLineComment = true;
        ++index;
        continue;
      }
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
      // The first whitespace outside literals and character classes separates
      // the rule's regex from its action block.
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
    // The extracted body is preserved verbatim and also removed from the
    // normalized definitions view so directive parsing does not see it twice.
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

}  // namespace

LexSpecification LexParser::parseLexFile(const std::string& path) const {
  LexSpecification spec;
  idreTable.clear();

  const std::string content = readWholeFile(path);
  const std::size_t first = findSectionDelimiter(content, 0);
  if (first == std::string::npos) {
    throw std::runtime_error("missing first %% section delimiter");
  }
  const std::size_t second = findSectionDelimiter(content, first + 2);
  if (second == std::string::npos) {
    throw std::runtime_error("missing second %% section delimiter");
  }

  spec.definitionsSection = content.substr(0, first);
  spec.rulesSection = content.substr(first + 2, second - (first + 2));
  spec.userSubroutines = content.substr(second + 2);

  // Parse named definitions first so later RE expansion can resolve `{NAME}`
  // references without revisiting the original source file.
  std::vector<std::pair<std::size_t, std::size_t>> verbatimRanges;
  spec.verbatimDefinitions =
      takeBetweenMarkers(spec.definitionsSection, "%{", "%}", &verbatimRanges);
  const std::string normalizedDefinitions = removeRanges(spec.definitionsSection, verbatimRanges);
  for (const std::string& rawLine : splitByLines(normalizedDefinitions)) {
    const std::string line = trim(stripInlineComment(rawLine));
    if (line.empty()) {
      continue;
    }
    std::istringstream iss(line);
    std::string name;
    iss >> name;
    std::string translation;
    std::getline(iss, translation);
    translation = trim(translation);
    if (!name.empty() && !translation.empty()) {
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
    const auto parts = splitRegexAndAction(pendingRule);
    if (parts.first.empty() || !isActionBalanced(parts.second)) {
      // Multi-line actions are accumulated until the block is structurally
      // complete, which keeps parsing simple without losing source fidelity.
      continue;
    }
    LexRule rule;
    rule.regex = parts.first;
    rule.action = parts.second;
    rule.priority = priority++;
    spec.rules.push_back(rule);
    pendingRule.clear();
  }
  if (!trim(pendingRule).empty()) {
    throw std::runtime_error("unterminated or malformed Lex rule near: " + pendingRule);
  }
  return spec;
}

}  // namespace seu_lex
