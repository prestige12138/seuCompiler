#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
YACC_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
REPO_ROOT="$(cd "${YACC_ROOT}/.." && pwd)"
CASE_DIR="${SCRIPT_DIR}/test_cases"
EXPECTED_DIR="${SCRIPT_DIR}/expected"
RESULTS_ROOT="${SCRIPT_DIR}/results"
BUILD_DIR="${YACC_ROOT}/build"

mkdir -p "${RESULTS_ROOT}"
RUN_ID="$(date '+%Y%m%d_%H%M%S')"
RESULT_DIR="${RESULTS_ROOT}/${RUN_ID}"
BIN_DIR="${RESULT_DIR}/bin"
TMP_DIR="${RESULT_DIR}/tmp"
LOG_DIR="${RESULT_DIR}/logs"

mkdir -p "${BIN_DIR}" "${TMP_DIR}" "${LOG_DIR}"

SUMMARY_TSV="${RESULT_DIR}/summary.tsv"
SUMMARY_MD="${RESULT_DIR}/SUMMARY.md"
printf "name\tstatus\tartifact\n" > "${SUMMARY_TSV}"
{
  echo "# seuYacc Test Summary"
  echo
  echo "| Test | Status | Artifact |"
  echo "| --- | --- | --- |"
} > "${SUMMARY_MD}"

TOTAL=0
PASSED=0
FAILED=0

record_result() {
  local name="$1"
  local status="$2"
  local artifact="$3"
  printf "%s\t%s\t%s\n" "${name}" "${status}" "${artifact}" >> "${SUMMARY_TSV}"
  printf "| %s | %s | %s |\n" "${name}" "${status}" "${artifact}" >> "${SUMMARY_MD}"
  TOTAL=$((TOTAL + 1))
  if [[ "${status}" == "PASS" ]]; then
    PASSED=$((PASSED + 1))
  else
    FAILED=$((FAILED + 1))
  fi
}

compare_exact() {
  local name="$1"
  local expected="$2"
  local actual="$3"
  local diff_file="${LOG_DIR}/${name}.diff"
  if diff -u "${expected}" "${actual}" > "${diff_file}"; then
    rm -f "${diff_file}"
    record_result "${name}" "PASS" "results/${RUN_ID}/$(basename "${actual}")"
  else
    record_result "${name}" "FAIL" "results/${RUN_ID}/$(basename "${diff_file}")"
  fi
}

normalize_exit_and_streams() {
  local output_file="$1"
  local exit_code="$2"
  local stdout_file="$3"
  local stderr_file="$4"
  {
    echo "exit_code=${exit_code}"
    if [[ -s "${stdout_file}" ]]; then
      echo "stdout=$(tr '\n' ' ' < "${stdout_file}" | sed 's/[[:space:]]\+/ /g' | sed 's/^ //; s/ $//')"
    else
      echo "stdout="
    fi
    if [[ -s "${stderr_file}" ]]; then
      echo "stderr=$(tr '\n' ' ' < "${stderr_file}" | sed 's/[[:space:]]\+/ /g' | sed 's/^ //; s/ $//')"
    else
      echo "stderr="
    fi
  } > "${output_file}"
}

probe_exact() {
  local name="$1"
  shift
  local actual="${RESULT_DIR}/${name}.txt"
  "${PROBE_BIN}" "$@" > "${actual}"
  compare_exact "${name}" "${EXPECTED_DIR}/${name}.txt" "${actual}"
}

build_generator() {
  cmake -S "${YACC_ROOT}" -B "${BUILD_DIR}" > "${LOG_DIR}/cmake_configure.log"
  cmake --build "${BUILD_DIR}" -j > "${LOG_DIR}/cmake_build.log"
  YACC_BIN="${BUILD_DIR}/seuYacc"
  PROBE_BIN="${BIN_DIR}/yacc_probe"
  c++ -std=c++17 -Wall -Wextra -Wpedantic \
    -I"${YACC_ROOT}/include" \
    "${SCRIPT_DIR}/yacc_probe.cpp" \
    "${YACC_ROOT}/src/yacc_parser.cpp" \
    "${YACC_ROOT}/src/lr1_pda.cpp" \
    "${YACC_ROOT}/src/parse_table.cpp" \
    "${YACC_ROOT}/src/lalr_converter.cpp" \
    "${YACC_ROOT}/src/symbol_table.cpp" \
    -o "${PROBE_BIN}" > "${LOG_DIR}/probe_build.log" 2>&1
}

write_driver_left_assoc() {
  local path="$1"
  local header="$2"
  cat > "${path}" <<EOF
#include <iostream>
#include <vector>

#include "${header}"

extern int get_assoc_value();

int main() {
  using namespace left_assoc_parser_generated;
  Token t1;
  t1.type = NUM;
  t1.lexeme = "10";
  t1.semantic.ival = 10;
  Token t2;
  t2.type = NUM;
  t2.lexeme = "3";
  t2.semantic.ival = 3;
  Token t3;
  t3.type = NUM;
  t3.lexeme = "2";
  t3.semantic.ival = 2;
  std::vector<Token> tokens = {t1, {'-', "-", 1, 2, {}}, t2, {'-', "-", 1, 4, {}}, t3};
  const bool ok = yyparse(tokens);
  std::cout << "parse=" << (ok ? "true" : "false") << "\\n";
  std::cout << "value=" << get_assoc_value() << "\\n";
  return ok && get_assoc_value() == 5 ? 0 : 1;
}
EOF
}

write_driver_nonassoc() {
  local path="$1"
  local header="$2"
  cat > "${path}" <<EOF
#include <iostream>
#include <vector>

#include "${header}"

int main() {
  using namespace nonassoc_parser_generated;
  std::vector<Token> tokens = {
      {NUM, "1", 1, 1, {}},
      {'<', "<", 1, 2, {}},
      {NUM, "2", 1, 3, {}},
      {'<', "<", 1, 4, {}},
      {NUM, "3", 1, 5, {}},
  };
  const bool ok = yyparse(tokens);
  std::cout << "parse=" << (ok ? "true" : "false") << "\\n";
  return ok ? 1 : 0;
}
EOF
}

write_driver_accept() {
  local path="$1"
  local header="$2"
  cat > "${path}" <<EOF
#include <iostream>
#include <vector>

#include "${header}"

int main() {
  using namespace contract_parser_generated;
  std::vector<Token> tokens = {
      {'(', "(", 1, 1, {}},
      {ID, "lhs", 1, 2, {}},
      {'+', "+", 1, 3, {}},
      {ID, "rhs", 1, 4, {}},
      {')', ")", 1, 5, {}},
      {'*', "*", 1, 6, {}},
      {ID, "tail", 1, 7, {}},
  };
  const bool ok = yyparse(tokens);
  std::cout << "parse=" << (ok ? "true" : "false") << "\\n";
  return ok ? 0 : 1;
}
EOF
}

write_driver_reject() {
  local path="$1"
  local header="$2"
  cat > "${path}" <<EOF
#include <iostream>
#include <vector>

#include "${header}"

int main() {
  using namespace reject_parser_generated;
  std::vector<Token> tokens = {
      {ID, "lhs", 1, 1, {}},
      {'+', "+", 1, 2, {}},
      {'*', "*", 1, 3, {}},
      {ID, "rhs", 1, 4, {}},
  };
  const bool ok = yyparse(tokens);
  std::cout << "parse=" << (ok ? "true" : "false") << "\\n";
  return ok ? 1 : 0;
}
EOF
}

write_driver_semantic() {
  local path="$1"
  local header="$2"
  cat > "${path}" <<EOF
#include <iostream>
#include <vector>

#include "${header}"

extern int read_semantic_total();

int main() {
  using namespace semantic_parser_generated;
  Token t1;
  t1.type = NUM;
  t1.lexeme = "1";
  t1.semantic.ival = 1;
  Token t2;
  t2.type = NUM;
  t2.lexeme = "2";
  t2.semantic.ival = 2;
  Token t3;
  t3.type = NUM;
  t3.lexeme = "4";
  t3.semantic.ival = 4;
  std::vector<Token> tokens = {t1, {'+', "+", 1, 2, {}}, t2, {'+', "+", 1, 4, {}}, t3};
  const bool ok = yyparse(tokens);
  std::cout << "parse=" << (ok ? "true" : "false") << "\\n";
  std::cout << "semantic_total=" << read_semantic_total() << "\\n";
  return ok && read_semantic_total() == 7 ? 0 : 1;
}
EOF
}

generate_and_run() {
  local name="$1"
  local grammar="$2"
  local parser_cpp="$3"
  local parser_h="$4"
  local mode="$5"
  local driver_writer="$6"
  local namespace_stem="$7"

  local stdout_file="${LOG_DIR}/${name}.stdout"
  local stderr_file="${LOG_DIR}/${name}.stderr"
  local actual="${RESULT_DIR}/${name}.txt"
  local driver_cpp="${TMP_DIR}/${name}_driver.cpp"
  local driver_bin="${BIN_DIR}/${name}_driver"
  local compile_log="${LOG_DIR}/${name}.compile.log"
  local runtime_stdout="${LOG_DIR}/${name}.runtime.stdout"
  local runtime_stderr="${LOG_DIR}/${name}.runtime.stderr"

  set +e
  "${YACC_BIN}" "${grammar}" "${parser_cpp}" "${parser_h}" "${mode}" > "${stdout_file}" 2> "${stderr_file}"
  local generate_exit=$?
  set -e

  if [[ ${generate_exit} -eq 0 ]]; then
    "${driver_writer}" "${driver_cpp}" "${parser_h}"
    set +e
    c++ -std=c++17 "${parser_cpp}" "${driver_cpp}" -o "${driver_bin}" > "${compile_log}" 2>&1
    local compile_exit=$?
    set -e

    local runtime_exit=99
    if [[ ${compile_exit} -eq 0 ]]; then
      set +e
      "${driver_bin}" > "${runtime_stdout}" 2> "${runtime_stderr}"
      runtime_exit=$?
      set -e
    else
      : > "${runtime_stdout}"
      : > "${runtime_stderr}"
    fi

    {
      echo "generate_exit=${generate_exit}"
      echo "compile_exit=${compile_exit}"
      echo "runtime_exit=${runtime_exit}"
      if [[ -s "${runtime_stdout}" ]]; then
        cat "${runtime_stdout}"
      fi
    } > "${actual}"
  else
    {
      echo "generate_exit=${generate_exit}"
      echo "compile_exit=not_run"
      echo "runtime_exit=not_run"
    } > "${actual}"
  fi

  compare_exact "${name}" "${EXPECTED_DIR}/${name}.txt" "${actual}"
}

generate_compile_only() {
  local name="$1"
  local grammar="$2"
  local parser_cpp="$3"
  local parser_h="$4"
  local mode="$5"

  local stdout_file="${LOG_DIR}/${name}.stdout"
  local stderr_file="${LOG_DIR}/${name}.stderr"
  local actual="${RESULT_DIR}/${name}.txt"
  local object_file="${TMP_DIR}/${name}.o"
  local compile_log="${LOG_DIR}/${name}.compile.log"

  set +e
  "${YACC_BIN}" "${grammar}" "${parser_cpp}" "${parser_h}" "${mode}" > "${stdout_file}" 2> "${stderr_file}"
  local generate_exit=$?
  set -e

  local compile_exit=99
  if [[ ${generate_exit} -eq 0 ]]; then
    set +e
    c++ -std=c++17 -c "${parser_cpp}" -o "${object_file}" > "${compile_log}" 2>&1
    compile_exit=$?
    set -e
  fi

  {
    echo "generate_exit=${generate_exit}"
    echo "compile_exit=${compile_exit}"
    echo "mode=${mode}"
  } > "${actual}"

  compare_exact "${name}" "${EXPECTED_DIR}/${name}.txt" "${actual}"
}

run_negative_cli() {
  local name="$1"
  local grammar="$2"
  local actual="${RESULT_DIR}/${name}.txt"
  local stdout_file="${LOG_DIR}/${name}.stdout"
  local stderr_file="${LOG_DIR}/${name}.stderr"
  local parser_cpp="${TMP_DIR}/${name}.cpp"
  local parser_h="${TMP_DIR}/${name}.h"

  set +e
  "${YACC_BIN}" "${grammar}" "${parser_cpp}" "${parser_h}" "lalr" > "${stdout_file}" 2> "${stderr_file}"
  local exit_code=$?
  set -e

  normalize_exit_and_streams "${actual}" "${exit_code}" "${stdout_file}" "${stderr_file}"
  compare_exact "${name}" "${EXPECTED_DIR}/${name}.txt" "${actual}"
}

run_self_test_case() {
  local name="$1"
  local stdout_file="${LOG_DIR}/${name}.stdout"
  local stderr_file="${LOG_DIR}/${name}.stderr"
  local actual="${RESULT_DIR}/${name}.txt"

  set +e
  "${YACC_BIN}" --self-test > "${stdout_file}" 2> "${stderr_file}"
  local exit_code=$?
  set -e

  {
    echo "exit_code=${exit_code}"
    if grep -q "\\[self-test\\] generated sample parser:" "${stdout_file}"; then
      echo "has_sample=yes"
    else
      echo "has_sample=no"
    fi
    if grep -q "\\[self-test\\] generated semantic parser:" "${stdout_file}"; then
      echo "has_semantic=yes"
    else
      echo "has_semantic=no"
    fi
    if grep -q "\\[self-test\\] generated minic parser:" "${stdout_file}"; then
      echo "has_minic=yes"
    else
      echo "has_minic=no"
    fi
    echo "stderr_empty=$( [[ -s "${stderr_file}" ]] && echo no || echo yes )"
  } > "${actual}"

  compare_exact "${name}" "${EXPECTED_DIR}/${name}.txt" "${actual}"
}

build_generator

probe_exact "01_parse_sections_basic" parse-summary "${CASE_DIR}/01_parse_sections_basic.y"
probe_exact "02_parse_union_precedence_midrule" parse-summary "${CASE_DIR}/02_parse_union_precedence_midrule.y"
probe_exact "03_first_follow_expr" first-follow "${CASE_DIR}/03_first_follow_expr.y" E T F
probe_exact "04_first_follow_nullable_chain" first-follow "${CASE_DIR}/04_first_follow_nullable_chain.y" S A B
probe_exact "05_closure_lookahead" start-items "${CASE_DIR}/05_closure_lookahead.y"
probe_exact "06_direct_lalr_vs_merge_lalr" automaton-stats "${CASE_DIR}/03_first_follow_expr.y"
probe_exact "09_reduce_reduce_resolution" conflict-summary "${CASE_DIR}/08_reduce_reduce_order.y" lalr
probe_exact "10_symbol_table_scope_shadowing" symbol-scope

generate_and_run \
  "07_precedence_left_assoc_runtime" \
  "${CASE_DIR}/06_precedence_left_assoc.y" \
  "${TMP_DIR}/left_assoc_parser.cpp" \
  "${TMP_DIR}/left_assoc_tokens.h" \
  "lalr" \
  write_driver_left_assoc \
  "left_assoc_parser"

generate_and_run \
  "08_precedence_nonassoc_reject" \
  "${CASE_DIR}/07_precedence_nonassoc.y" \
  "${TMP_DIR}/nonassoc_parser.cpp" \
  "${TMP_DIR}/nonassoc_tokens.h" \
  "lalr" \
  write_driver_nonassoc \
  "nonassoc_parser"

generate_and_run \
  "11_lex_token_contract_accept" \
  "${CASE_DIR}/10_generated_parser_contract.y" \
  "${TMP_DIR}/contract_parser.cpp" \
  "${TMP_DIR}/contract_tokens.h" \
  "lalr" \
  write_driver_accept \
  "contract_parser"

generate_and_run \
  "12_generated_parse_failure" \
  "${CASE_DIR}/10_generated_parser_contract.y" \
  "${TMP_DIR}/reject_parser.cpp" \
  "${TMP_DIR}/reject_tokens.h" \
  "lalr" \
  write_driver_reject \
  "reject_parser"

generate_and_run \
  "13_generated_semantic_actions_and_user_code" \
  "${CASE_DIR}/11_generated_semantic_actions.y" \
  "${TMP_DIR}/semantic_parser.cpp" \
  "${TMP_DIR}/semantic_tokens.h" \
  "lalr" \
  write_driver_semantic \
  "semantic_parser"

generate_compile_only \
  "14_mode_lr1_generation_compile" \
  "${CASE_DIR}/12_mode_lr1_generation.y" \
  "${TMP_DIR}/lr1_parser.cpp" \
  "${TMP_DIR}/lr1_tokens.h" \
  "lr1"

run_negative_cli "15_error_missing_delimiters" "${CASE_DIR}/13_error_missing_delimiters.y"
run_negative_cli "16_error_unterminated_action" "${CASE_DIR}/14_error_unterminated_action.y"

generate_compile_only \
  "17_resource_minic_generation_regression" \
  "${REPO_ROOT}/resources/minic.y" \
  "${TMP_DIR}/minic_parser.cpp" \
  "${TMP_DIR}/minic_tokens.h" \
  "lalr"

run_self_test_case "18_cli_self_test"

{
  echo
  echo "Total: ${TOTAL}"
  echo "Passed: ${PASSED}"
  echo "Failed: ${FAILED}"
  echo "Result directory: ${RESULT_DIR}"
} | tee -a "${SUMMARY_MD}"

if [[ ${FAILED} -ne 0 ]]; then
  exit 1
fi
