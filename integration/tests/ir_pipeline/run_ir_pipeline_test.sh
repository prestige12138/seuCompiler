#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
RESULTS_ROOT="${SCRIPT_DIR}/results"
RUN_ID="$(date '+%Y%m%d_%H%M%S')"
RUN_DIR="${RESULTS_ROOT}/${RUN_ID}"
TMP_DIR="${RUN_DIR}/tmp"
LOG_DIR="${RUN_DIR}/logs"
ACTUAL_DIR="${RUN_DIR}/actual"

mkdir -p "${TMP_DIR}" "${LOG_DIR}" "${ACTUAL_DIR}"
printf '%s\n' "${RUN_ID}" > "${RESULTS_ROOT}/LATEST.txt"

LEX_SPEC="${SCRIPT_DIR}/test_cases/demo_ir.l"
YACC_SPEC="${SCRIPT_DIR}/test_cases/demo_ir.y"
SOURCE_FILE="${SCRIPT_DIR}/test_cases/demo_ir.c"
EXPECTED_TAC="${SCRIPT_DIR}/expected/demo_ir.tac"
EXPECTED_LLVM="${SCRIPT_DIR}/expected/demo_ir.ll"
EXPECTED_JIMPLE="${SCRIPT_DIR}/expected/demo_ir.jimple"

PIPELINE_TOKENS="${TMP_DIR}/demo_ir_tokens.h"
PIPELINE_PARSER="${TMP_DIR}/demo_ir_parser.cpp"
PIPELINE_LEXER="${TMP_DIR}/demo_ir_lexer.cpp"
PIPELINE_DRIVER="${TMP_DIR}/demo_ir_driver.cpp"
PIPELINE_BIN="${TMP_DIR}/demo_ir_driver"

CXX_BIN="${SEU_TEST_CXX:-${CXX:-c++}}"
SEU_LEX_BIN="${SEU_LEX_BIN:-}"
SEU_YACC_BIN="${SEU_YACC_BIN:-}"

if [[ -n "${CI:-}" && "${SEU_TRUSTED_SPECS:-0}" != "1" ]]; then
  echo "refusing to compile generated parser/lexer in CI without SEU_TRUSTED_SPECS=1" >&2
  exit 1
fi

if [[ "${SEU_TRUSTED_SPECS:-0}" != "1" ]]; then
  echo "warning: this script compiles and runs C/C++ emitted from trusted .l/.y specs" >&2
fi

check_generated_file_for_repo_path() {
  local file_path="$1"
  if grep -q "${REPO_ROOT}" "${file_path}"; then
    echo "generated file contains absolute repository paths: ${file_path}" >&2
    exit 1
  fi
}

if [[ -z "${SEU_LEX_BIN}" || -z "${SEU_YACC_BIN}" ]]; then
  SEULEX_BUILD_DIR="${TMP_DIR}/seuLex-build"
  SEUYACC_BUILD_DIR="${TMP_DIR}/seuYacc-build"
  CMAKE_ARGS=()
  if [[ -n "${SEU_TEST_CXX:-}" ]]; then
    CMAKE_ARGS+=("-DCMAKE_CXX_COMPILER=${SEU_TEST_CXX}")
  fi
  if [[ ${#CMAKE_ARGS[@]} -gt 0 ]]; then
    cmake -S "${REPO_ROOT}/seuLex" -B "${SEULEX_BUILD_DIR}" "${CMAKE_ARGS[@]}" > "${LOG_DIR}/seulex.configure.log"
  else
    cmake -S "${REPO_ROOT}/seuLex" -B "${SEULEX_BUILD_DIR}" > "${LOG_DIR}/seulex.configure.log"
  fi
  cmake --build "${SEULEX_BUILD_DIR}" -j > "${LOG_DIR}/seulex.build.log"
  if [[ ${#CMAKE_ARGS[@]} -gt 0 ]]; then
    cmake -S "${REPO_ROOT}/seuYacc" -B "${SEUYACC_BUILD_DIR}" "${CMAKE_ARGS[@]}" > "${LOG_DIR}/seuyacc.configure.log"
  else
    cmake -S "${REPO_ROOT}/seuYacc" -B "${SEUYACC_BUILD_DIR}" > "${LOG_DIR}/seuyacc.configure.log"
  fi
  cmake --build "${SEUYACC_BUILD_DIR}" -j > "${LOG_DIR}/seuyacc.build.log"
  SEU_LEX_BIN="${SEULEX_BUILD_DIR}/seuLex"
  SEU_YACC_BIN="${SEUYACC_BUILD_DIR}/seuYacc"
fi

"${SEU_YACC_BIN}" \
  "${YACC_SPEC}" "${PIPELINE_PARSER}" "${PIPELINE_TOKENS}" lalr \
  > "${LOG_DIR}/seuyacc.generate.stdout" 2> "${LOG_DIR}/seuyacc.generate.stderr"

if grep -q "${REPO_ROOT}" "${PIPELINE_PARSER}"; then
  echo "generated parser contains absolute repository paths" >&2
  exit 1
fi
check_generated_file_for_repo_path "${PIPELINE_TOKENS}"

"${SEU_LEX_BIN}" \
  "${LEX_SPEC}" "${PIPELINE_LEXER}" "${TMP_DIR}/dot" --token-header "${PIPELINE_TOKENS}" \
  > "${LOG_DIR}/seulex.generate.stdout" 2> "${LOG_DIR}/seulex.generate.stderr"

check_generated_file_for_repo_path "${PIPELINE_PARSER}"
check_generated_file_for_repo_path "${PIPELINE_TOKENS}"
check_generated_file_for_repo_path "${PIPELINE_LEXER}"

cat > "${PIPELINE_DRIVER}" <<'EOF'
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ast_builder.h"
#include "intermediate_code.h"
#include "symbol_table.h"
#include "target_ir_emitter.h"
#include "tri_addr_generator.h"
#include "demo_ir_tokens.h"

std::vector<demo_ir_parser_generated::Token> tokenize_for_parser(const std::string& source);

namespace {

std::string readFile(const std::string& path) {
  std::ifstream input(path);
  if (!input) {
    throw std::runtime_error("failed to open source file: " + path);
  }
  return std::string((std::istreambuf_iterator<char>(input)),
                     std::istreambuf_iterator<char>());
}

void writeFile(const std::string& path, const std::string& text) {
  std::ofstream output(path);
  if (!output) {
    throw std::runtime_error("failed to open output file: " + path);
  }
  output << text << '\n';
}

}  // namespace

int main(int argc, char** argv) {
  if (argc != 3) {
    throw std::runtime_error("usage: demo_ir_driver <source-file> <output-dir>");
  }

  const std::string source = readFile(argv[1]);
  const std::string output_dir = argv[2];
  std::vector<demo_ir_parser_generated::Token> tokens = tokenize_for_parser(source);

  if (!demo_ir_parser_generated::yyparse(tokens)) {
    throw std::runtime_error("yyparse returned false");
  }

  seu_icg::ASTNode* root = seu_icg::releaseParseRoot();
  if (root == nullptr) {
    throw std::runtime_error("parse root is null");
  }

  seu_icg::SymbolTable symbols;
  seu_icg::TriAddrGenerator generator(&symbols);
  const seu_icg::IntermediateCode code = generator.generate(root);

  seu_icg::TargetIrOptions options;
  options.moduleName = "demo_ir_pipeline";
  options.className = "DemoIrPipeline";
  options.entryFunction = "main";

  writeFile(output_dir + "/demo_ir.tac", seu_icg::formatIntermediateCode(code));
  writeFile(output_dir + "/demo_ir.ll", seu_icg::formatLlvmIr(root, code, options));
  writeFile(output_dir + "/demo_ir.jimple", seu_icg::formatJimple(root, code, options));

  seu_icg::ASTBuilder builder;
  builder.destroyTree(root);
  return 0;
}
EOF

"${CXX_BIN}" -std=c++17 -Wall -Wextra -Wpedantic \
  -I"${REPO_ROOT}/intermediate/include" \
  -I"${TMP_DIR}" \
  "${PIPELINE_DRIVER}" \
  "${PIPELINE_PARSER}" \
  "${PIPELINE_LEXER}" \
  "${REPO_ROOT}/intermediate/src/ast_builder.cpp" \
  "${REPO_ROOT}/intermediate/src/intermediate_code.cpp" \
  "${REPO_ROOT}/intermediate/src/symbol_table.cpp" \
  "${REPO_ROOT}/intermediate/src/target_ir_emitter.cpp" \
  "${REPO_ROOT}/intermediate/src/tri_addr_generator.cpp" \
  -o "${PIPELINE_BIN}" > "${LOG_DIR}/pipeline.compile.log" 2>&1

"${PIPELINE_BIN}" "${SOURCE_FILE}" "${ACTUAL_DIR}" > "${LOG_DIR}/pipeline.run.stdout" 2> "${LOG_DIR}/pipeline.run.stderr"

diff -u "${EXPECTED_TAC}" "${ACTUAL_DIR}/demo_ir.tac" > "${LOG_DIR}/demo_ir.tac.diff"
rm -f "${LOG_DIR}/demo_ir.tac.diff"
diff -u "${EXPECTED_LLVM}" "${ACTUAL_DIR}/demo_ir.ll" > "${LOG_DIR}/demo_ir.ll.diff"
rm -f "${LOG_DIR}/demo_ir.ll.diff"
diff -u "${EXPECTED_JIMPLE}" "${ACTUAL_DIR}/demo_ir.jimple" > "${LOG_DIR}/demo_ir.jimple.diff"
rm -f "${LOG_DIR}/demo_ir.jimple.diff"

echo "Run ID: ${RUN_ID}"
echo "Actual directory: ${ACTUAL_DIR}"
