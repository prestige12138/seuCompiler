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
