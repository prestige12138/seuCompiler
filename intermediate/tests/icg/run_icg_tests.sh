#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ICG_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
CASE_DIR="${SCRIPT_DIR}/test_cases"
EXPECTED_DIR="${SCRIPT_DIR}/expected"
RESULTS_ROOT="${SCRIPT_DIR}/results"
BUILD_DIR="${ICG_ROOT}/build"

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
  echo "# seuIntermediate Test Summary"
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
    record_result "${name}" "FAIL" "results/${RUN_ID}/logs/$(basename "${diff_file}")"
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

build_artifacts() {
  cmake -S "${ICG_ROOT}" -B "${BUILD_DIR}" > "${LOG_DIR}/cmake_configure.log"
  cmake --build "${BUILD_DIR}" -j > "${LOG_DIR}/cmake_build.log"

  ICG_BIN="${BUILD_DIR}/seuIntermediate"
  PROBE_BIN="${BIN_DIR}/icg_probe"
  c++ -std=c++17 -Wall -Wextra -Wpedantic \
    -I"${ICG_ROOT}/include" \
    "${SCRIPT_DIR}/icg_probe.cpp" \
    "${ICG_ROOT}/src/ast_builder.cpp" \
    "${ICG_ROOT}/src/symbol_table.cpp" \
    "${ICG_ROOT}/src/tri_addr_generator.cpp" \
    "${ICG_ROOT}/src/intermediate_code.cpp" \
    -o "${PROBE_BIN}" > "${LOG_DIR}/probe_build.log" 2>&1
}

run_probe_case() {
  local name="$1"
  shift

  local actual="${RESULT_DIR}/${name}.txt"
  local stderr_file="${TMP_DIR}/${name}.stderr"

  if "${PROBE_BIN}" "$@" > "${actual}" 2> "${stderr_file}"; then
    compare_exact "${name}" "${EXPECTED_DIR}/${name}.txt" "${actual}"
  else
    cp "${stderr_file}" "${LOG_DIR}/${name}.stderr"
    record_result "${name}" "FAIL" "results/${RUN_ID}/logs/${name}.stderr"
  fi
}

run_cli_self_test() {
  local name="12_cli_self_test"
  local stdout_file="${TMP_DIR}/${name}.stdout"
  local stderr_file="${TMP_DIR}/${name}.stderr"
  local actual="${RESULT_DIR}/${name}.txt"

  set +e
  "${ICG_BIN}" --self-test > "${stdout_file}" 2> "${stderr_file}"
  local exit_code=$?
  set -e

  normalize_exit_and_streams "${actual}" "${exit_code}" "${stdout_file}" "${stderr_file}"
  compare_exact "${name}" "${EXPECTED_DIR}/${name}.txt" "${actual}"
}

run_perf_case() {
  local name="13_perf_batch_generation"
  local actual="${RESULT_DIR}/${name}.txt"
  local stderr_file="${TMP_DIR}/${name}.stderr"
  local metrics_file="${LOG_DIR}/${name}.metrics"
  local start_seconds
  local end_seconds

  start_seconds="$(date '+%s')"
  if "${PROBE_BIN}" perf-batch 2000 > "${actual}" 2> "${stderr_file}"; then
    end_seconds="$(date '+%s')"
    echo "elapsed_seconds=$((end_seconds - start_seconds))" > "${metrics_file}"
    compare_exact "${name}" "${EXPECTED_DIR}/${name}.txt" "${actual}"
  else
    cp "${stderr_file}" "${LOG_DIR}/${name}.stderr"
    record_result "${name}" "FAIL" "results/${RUN_ID}/logs/${name}.stderr"
  fi
}

build_artifacts

run_probe_case "01_ast_construction_basic" ast-shape
run_probe_case "02_parse_root_handoff" parse-root
run_probe_case "03_symbol_scope_shadowing" symbol-scope
run_probe_case "04_symbol_offsets_global_local_param" symbol-offsets
run_probe_case "05_format_helpers" formatters
run_probe_case "06_ir_constant_assignment" ir-constant-assign
run_probe_case "07_ir_arithmetic_assignment" ir-arithmetic
run_probe_case "08_ir_function_call_and_control_flow" ir-control-flow
run_probe_case "09_ir_function_body" ir-function
run_probe_case "10_error_unsupported_operator" ir-unsupported-op
run_probe_case "11_generate_empty_root" generate-empty
run_probe_case "14_basic_block_empty_code" basic-block-empty
run_probe_case "15_basic_block_linear_fallthrough" basic-block-linear
run_probe_case "16_basic_block_conditional_branch" basic-block-conditional
run_probe_case "17_basic_block_mixed_control_flow" basic-block-mixed
run_probe_case "18_basic_block_sparse_stmt_numbers" basic-block-sparse
run_probe_case "19_basic_block_invalid_target" basic-block-invalid-target
run_cli_self_test
run_perf_case

echo "${RUN_ID}" > "${RESULTS_ROOT}/LATEST.txt"

{
  echo
  echo "Passed: ${PASSED}"
  echo "Failed: ${FAILED}"
  echo
  echo "Case directory: ${CASE_DIR}"
  echo "Result directory: ${RESULT_DIR}"
} >> "${SUMMARY_MD}"

echo "Run ID: ${RUN_ID}"
echo "Passed: ${PASSED}/${TOTAL}"
echo "Failed: ${FAILED}/${TOTAL}"
echo "Summary: ${RESULT_DIR}/SUMMARY.md"

if [[ "${FAILED}" -ne 0 ]]; then
  exit 1
fi
