#include "code_generator.h"

#include <iostream>
#include <stdexcept>
#include <string>
#include <unistd.h>

namespace {

std::string currentWorkingDirectory() {
  char buffer[4096];
  if (::getcwd(buffer, sizeof(buffer)) == nullptr) {
    throw std::runtime_error("failed to obtain current working directory");
  }
  return std::string(buffer);
}

}  // namespace

int main(int argc, char** argv) {
  try {
    seu_lex::SeuLexDriver driver;
    const std::string workspaceRoot = currentWorkingDirectory();
    if (argc >= 2 && std::string(argv[1]) == "--self-test") {
      return driver.runSelfTests(workspaceRoot) ? 0 : 1;
    }
    if (argc < 2 || argc > 6) {
      std::cerr << "Usage: " << argv[0]
                << " <lex-file> [generated-lexer.cpp] [dot-output-dir] [--token-header <generated-tokens.h>]\n"
                << "   or: " << argv[0] << " --self-test\n";
      return 1;
    }
    const std::string lexPath = argv[1];
    std::string outPath = "generated_lexer.cpp";
    std::string dotDir = "dot";
    std::string tokenHeaderPath;
    int positionalIndex = 0;
    for (int index = 2; index < argc; ++index) {
      const std::string arg = argv[index];
      if (arg == "--token-header") {
        if (index + 1 >= argc) {
          throw std::runtime_error("missing path after --token-header");
        }
        tokenHeaderPath = argv[++index];
        continue;
      }
      if (positionalIndex == 0) {
        outPath = arg;
      } else if (positionalIndex == 1) {
        dotDir = arg;
      } else {
        throw std::runtime_error("too many positional arguments");
      }
      ++positionalIndex;
    }
    driver.generate(lexPath, outPath, dotDir, tokenHeaderPath);
    std::cout << "Generated lexer source: " << outPath << '\n'
              << "DOT visualizations in: " << dotDir << '\n';
    if (!tokenHeaderPath.empty()) {
      std::cout << "Token ABI header: " << tokenHeaderPath << '\n';
    }
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "seuLex error: " << ex.what() << '\n';
    return 1;
  }
}
