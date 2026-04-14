#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

int analysis(std::string yytext);
std::vector<int> tokenize(const std::string& source);

namespace {

std::string unescape(const std::string& input) {
  std::string output;
  for (std::size_t index = 0; index < input.size(); ++index) {
    const char ch = input[index];
    if (ch != '\\') {
      output.push_back(ch);
      continue;
    }
    if (index + 1 >= input.size()) {
      output.push_back('\\');
      break;
    }
    const char escaped = input[++index];
    switch (escaped) {
      case 'n':
        output.push_back('\n');
        break;
      case 't':
        output.push_back('\t');
        break;
      case 'r':
        output.push_back('\r');
        break;
      case '\\':
        output.push_back('\\');
        break;
      case '|':
        output.push_back('|');
        break;
      case '"':
        output.push_back('"');
        break;
      case '0':
        output.push_back('\0');
        break;
      default:
        output.push_back(escaped);
        break;
    }
  }
  return output;
}

std::vector<std::string> split(const std::string& line) {
  std::vector<std::string> parts;
  std::string current;
  bool escaping = false;
  for (char ch : line) {
    if (escaping) {
      current.push_back('\\');
      current.push_back(ch);
      escaping = false;
      continue;
    }
    if (ch == '\\') {
      escaping = true;
      continue;
    }
    if (ch == '|') {
      parts.push_back(current);
      current.clear();
      continue;
    }
    current.push_back(ch);
  }
  if (escaping) {
    current.push_back('\\');
  }
  parts.push_back(current);
  return parts;
}

std::string join(const std::vector<int>& values) {
  std::ostringstream oss;
  for (std::size_t index = 0; index < values.size(); ++index) {
    if (index != 0) {
      oss << ',';
    }
    oss << values[index];
  }
  return oss.str();
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 2) {
    std::cerr << "usage: assert_driver <manifest>\n";
    return 2;
  }

  std::ifstream input(argv[1]);
  if (!input) {
    std::cerr << "failed to open manifest: " << argv[1] << '\n';
    return 2;
  }

  bool ok = true;
  std::string line;
  while (std::getline(input, line)) {
    if (line.empty() || line[0] == '#') {
      continue;
    }
    const auto parts = split(line);
    if (parts.empty()) {
      continue;
    }
    if (parts[0] == "analysis") {
      if (parts.size() < 3) {
        std::cerr << "invalid analysis line: " << line << '\n';
        ok = false;
        continue;
      }
      const std::string lexeme = unescape(parts[1]);
      const int expected = std::stoi(parts[2]);
      const int actual = analysis(lexeme);
      std::cout << "analysis|" << parts[1] << "|" << actual << '\n';
      if (actual != expected) {
        ok = false;
      }
    } else if (parts[0] == "tokenize") {
      if (parts.size() < 3) {
        std::cerr << "invalid tokenize line: " << line << '\n';
        ok = false;
        continue;
      }
      const std::string source = unescape(parts[1]);
      const std::string expected = parts[2];
      const std::string actual = join(tokenize(source));
      std::cout << "tokenize|" << parts[1] << "|" << actual << '\n';
      if (actual != expected) {
        ok = false;
      }
    }
  }

  return ok ? 0 : 1;
}
