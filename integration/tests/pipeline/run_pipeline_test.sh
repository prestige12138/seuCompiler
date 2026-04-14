#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/../../.." && pwd)"
RESULTS_ROOT="${SCRIPT_DIR}/results"
RUN_ID="$(date '+%Y%m%d_%H%M%S')"
RUN_DIR="${RESULTS_ROOT}/${RUN_ID}"
TMP_DIR="${RUN_DIR}/tmp"
LOG_DIR="${RUN_DIR}/logs"

mkdir -p "${TMP_DIR}" "${LOG_DIR}"
printf '%s\n' "${RUN_ID}" > "${RESULTS_ROOT}/LATEST.txt"

LEX_SPEC="${SCRIPT_DIR}/test_cases/pipeline_expr.l"
YACC_SPEC="${SCRIPT_DIR}/test_cases/pipeline_expr.y"
EXPECTED_IR="${SCRIPT_DIR}/expected/pipeline_expr.ir"

PIPELINE_TOKENS="${TMP_DIR}/pipeline_tokens.h"
PIPELINE_PARSER="${TMP_DIR}/pipeline_parser.cpp"
PIPELINE_LEXER="${TMP_DIR}/pipeline_lexer.cpp"
PIPELINE_DRIVER="${TMP_DIR}/pipeline_driver.cpp"
PIPELINE_BIN="${TMP_DIR}/pipeline_driver"
ACTUAL_IR="${RUN_DIR}/pipeline_expr.ir"

CXX_BIN="${SEU_TEST_CXX:-${CXX:-c++}}"
SEU_LEX_BIN="${SEU_LEX_BIN:-}"
SEU_YACC_BIN="${SEU_YACC_BIN:-}"

if [[ -z "${SEU_LEX_BIN}" || -z "${SEU_YACC_BIN}" ]]; then
  SEULEX_BUILD_DIR="${TMP_DIR}/seuLex-build"
  SEUYACC_BUILD_DIR="${TMP_DIR}/seuYacc-build"
  CMAKE_ARGS=()
  if [[ -n "${SEU_TEST_CXX:-}" ]]; then
    CMAKE_ARGS+=("-DCMAKE_CXX_COMPILER=${SEU_TEST_CXX}")
  fi
  cmake -S "${REPO_ROOT}/seuLex" -B "${SEULEX_BUILD_DIR}" "${CMAKE_ARGS[@]}" > "${LOG_DIR}/seulex.configure.log"
  cmake --build "${SEULEX_BUILD_DIR}" -j > "${LOG_DIR}/seulex.build.log"
  cmake -S "${REPO_ROOT}/seuYacc" -B "${SEUYACC_BUILD_DIR}" "${CMAKE_ARGS[@]}" > "${LOG_DIR}/seuyacc.configure.log"
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

"${SEU_LEX_BIN}" \
  "${LEX_SPEC}" "${PIPELINE_LEXER}" "${TMP_DIR}/dot" \
  > "${LOG_DIR}/seulex.generate.stdout" 2> "${LOG_DIR}/seulex.generate.stderr"

cat > "${PIPELINE_DRIVER}" <<'EOF'
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ast_builder.h"
#include "intermediate_code.h"
#include "symbol_table.h"
#include "tri_addr_generator.h"
#include "pipeline_tokens.h"

struct SeuLexToken {
  int type = 0;
  std::string lexeme;
  int line = 0;
  int column = 0;
};

std::vector<SeuLexToken> tokenize_detailed(const std::string& source);

int main() {
  const std::string source = "x = 1 + 2 * 3; return x;";
  std::vector<SeuLexToken> lex_tokens = tokenize_detailed(source);
  std::vector<pipeline_parser_generated::Token> tokens;
  tokens.reserve(lex_tokens.size());

  for (const SeuLexToken& token : lex_tokens) {
    pipeline_parser_generated::Token parser_token{};
    parser_token.type = token.type;
    parser_token.lexeme = token.lexeme;
    parser_token.line = token.line;
    parser_token.column = token.column;
    if (token.type == pipeline_parser_generated::NUMBER) {
      parser_token.semantic.ival = std::stoi(token.lexeme);
    }
    if (token.type == pipeline_parser_generated::IDENTIFIER) {
      parser_token.semantic.str = token.lexeme.c_str();
    }
    tokens.push_back(parser_token);
  }

  if (!pipeline_parser_generated::yyparse(tokens)) {
    throw std::runtime_error("yyparse returned false");
  }

  seu_icg::ASTNode* root = seu_icg::releaseParseRoot();
  if (root == nullptr) {
    throw std::runtime_error("parse root is null");
  }

  seu_icg::SymbolTable symbols;
  seu_icg::TriAddrGenerator generator(&symbols);
  const seu_icg::IntermediateCode code = generator.generate(root);
  std::cout << seu_icg::formatIntermediateCode(code) << '\n';

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
  "${REPO_ROOT}/intermediate/src/tri_addr_generator.cpp" \
  -o "${PIPELINE_BIN}" > "${LOG_DIR}/pipeline.compile.log" 2>&1

"${PIPELINE_BIN}" > "${ACTUAL_IR}" 2> "${LOG_DIR}/pipeline.run.stderr"
diff -u "${EXPECTED_IR}" "${ACTUAL_IR}" > "${LOG_DIR}/pipeline.diff"
rm -f "${LOG_DIR}/pipeline.diff"

echo "Run ID: ${RUN_ID}"
echo "Result: ${ACTUAL_IR}"
