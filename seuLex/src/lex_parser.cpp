/**
 * @file lex_parser.cpp
 * @brief 实现 Lex 三段结构解析、命名定义提取和规则抽取逻辑。
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

/**
 * @brief 在 `.l` 文件文本中定位顶层 `%%` 分段标记。
 */
std::size_t findSectionDelimiter(const std::string& content, std::size_t startPos) {
  bool inVerbatim = false;
  bool lineStart = true;
  for (std::size_t index = startPos; index + 1 < content.size(); ++index) {
    // 只有当 `%%` 出现在逻辑行开头且不在 `%{ ... %}` 原样代码块内部时，
    // 才能作为定义段、规则段和用户代码段的分隔符。
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

/**
 * @brief 去掉字符串首尾空白字符。
 */
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

/**
 * @brief 按行切分字符串，同时保留结尾空行语义。
 */
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

/**
 * @brief 去除一行中位于字符串字面量之外的块注释起始部分。
 */
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

/**
 * @brief 判断一个动作代码块的大括号是否已经完整闭合。
 */
bool isActionBalanced(const std::string& action) {
  if (action.empty()) {
    return false;
  }
  if (action.front() != '{') {
    return true;
  }
  // 动作中可能出现字符串、字符常量和注释，所以这里不能只数大括号，
  // 必须同时跟踪当前所处的词法上下文。
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

/**
 * @brief 把一条规则文本拆成“正则部分”和“动作部分”。
 */
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
      // 只有位于普通上下文中的首个空白符才能切开正则和动作，
      // 字符串和字符类里的空白都必须原样保留。
      separator = i;
      break;
    }
  }
  if (separator == std::string::npos) {
    return {trim(ruleText), ""};
  }
  return {trim(ruleText.substr(0, separator)), trim(ruleText.substr(separator))};
}

/**
 * @brief 读取整个 `.l` 文件内容。
 */
std::string readWholeFile(const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open file: " + path);
  }
  std::ostringstream buffer;
  buffer << input.rdbuf();
  return buffer.str();
}

/**
 * @brief 抽取指定左右标记之间的原样代码块，并记录其原始区间。
 */
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
    // 原样代码既要保留下来供最终代码生成使用，也要从标准化视图中移除，
    // 避免后续把它再次误当成定义段指令进行解析。
    extracted << text.substr(begin + left.size(), end - begin - left.size()) << '\n';
    if (ranges != nullptr) {
      ranges->push_back({begin, end + right.size()});
    }
    searchPos = end + right.size();
  }
  return extracted.str();
}

/**
 * @brief 从文本中删除若干指定区间，得到适合继续解析的规范化内容。
 */
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

}  // 匿名命名空间

/**
 * @brief 解析一个 Lex 输入文件，并返回三段结构化结果。
 */
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

  // 先解析命名定义，后续扩展正则时才能直接替换 `{NAME}` 引用，
  // 不需要再回头重新扫描原始文件。
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
      // 多行动作会持续累积，直到代码块结构完整为止，
      // 这样既能保持实现简单，也不会破坏原始代码内容。
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

}  // 命名空间 seu_lex
