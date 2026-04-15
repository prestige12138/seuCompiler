/**
 * @file main.cpp
 * @brief CLI entry for the standalone seuYacc generator.
 */

#include "parse_table.h"

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
    seu_yacc::SeuYaccDriver driver;
    const std::string workspace_root = currentWorkingDirectory();
    if (argc >= 2 && std::string(argv[1]) == "--self-test") {
      return driver.runSelfTests(workspace_root) ? 0 : 1;
    }
    if (argc < 2 || argc > 5) {
      std::cerr << "Usage: " << argv[0]
                << " <yacc-file> [generated-parser.cpp] [generated-tokens.h] [lalr|lr1]\n"
                << "   or: " << argv[0] << " --self-test\n";
      return 1;
    }
    const std::string yacc_path = argv[1];
    const std::string out_cpp = argc >= 3 ? argv[2] : "generated_parser.cpp";
    const std::string out_header = argc >= 4 ? argv[3] : "generated_tokens.h";
    const std::string mode = argc >= 5 ? argv[4] : "lalr";
    driver.generate(yacc_path, out_cpp, out_header, mode);
    std::cout << "Generated parser source: " << out_cpp << '\n'
              << "Generated token header: " << out_header << '\n'
              << "Mode: " << mode << '\n';
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "seuYacc error: " << ex.what() << '\n';
    return 1;
  }
}
