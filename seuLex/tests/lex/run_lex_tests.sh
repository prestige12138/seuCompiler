#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TEST_ROOT="$ROOT_DIR/tests/lex"
CASE_ROOT="$TEST_ROOT/test_cases"
EXPECT_ROOT="$TEST_ROOT/expected"
RESULTS_ROOT="$TEST_ROOT/results"
RUN_ID="$(date '+%Y%m%d-%H%M%S')"
RUN_DIR="$RESULTS_ROOT/$RUN_ID"
SEULEX_DIR="$ROOT_DIR/seuLex"
SEULEX_BIN="$SEULEX_DIR/build/seuLex"
CXX_BIN="${CXX:-c++}"
FAILURES=0

mkdir -p "$RUN_DIR"
printf '%s\n' "$RUN_ID" > "$RESULTS_ROOT/LATEST.txt"

SUMMARY_TSV="$RUN_DIR/summary.tsv"
SUMMARY_MD="$RUN_DIR/summary.md"
printf "name\tstatus\tkind\tnotes\n" > "$SUMMARY_TSV"
printf "# seuLex Test Summary\n\n| Test | Status | Kind | Notes |\n|---|---|---|---|\n" > "$SUMMARY_MD"

append_summary() {
  local name="$1"
  local status="$2"
  local kind="$3"
  local notes="$4"
  printf "%s\t%s\t%s\t%s\n" "$name" "$status" "$kind" "$notes" >> "$SUMMARY_TSV"
  printf "| %s | %s | %s | %s |\n" "$name" "$status" "$kind" "$notes" >> "$SUMMARY_MD"
}

count_states() {
  local dot_file="$1"
  grep -E '^[[:space:]]+[0-9]+ \[shape=' "$dot_file" | wc -l | tr -d ' '
}

write_assert_driver() {
  local driver_path="$1"
  cat > "$driver_path" <<'EOF'
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
EOF
}

run_success_case() {
  local name="$1"
  local spec="$2"
  local manifest="$3"
  local case_dir="$RUN_DIR/$name"
  local notes=""
  local status="PASS"

  mkdir -p "$case_dir/dot"

  if ! "$SEULEX_BIN" "$spec" "$case_dir/generated.cpp" "$case_dir/dot" \
      > "$case_dir/generate.stdout" 2> "$case_dir/generate.stderr"; then
    status="FAIL"
    notes="generator failed"
  fi

  if [[ "$status" == "PASS" ]]; then
    if ! "$CXX_BIN" -std=c++17 "$case_dir/generated.cpp" "$ASSERT_DRIVER_SRC" -o "$case_dir/assert_driver" \
        > "$case_dir/compile.stdout" 2> "$case_dir/compile.stderr"; then
      status="FAIL"
      notes="generated lexer compile failed"
    fi
  fi

  if [[ "$status" == "PASS" ]]; then
    if ! "$case_dir/assert_driver" "$manifest" > "$case_dir/assertions.log" 2> "$case_dir/assertions.err"; then
      status="FAIL"
      notes="analysis/tokenize assertions failed"
    fi
  fi

  if [[ "$status" == "PASS" ]]; then
    while IFS='|' read -r kind arg1 arg2; do
      [[ -z "$kind" || "$kind" == \#* ]] && continue
      case "$kind" in
        dot_exists)
          if [[ ! -f "$case_dir/dot/$arg1" ]]; then
            status="FAIL"
            notes="${notes:+$notes; }missing dot file $arg1"
          else
            printf 'dot_exists|%s|ok\n' "$arg1" >> "$case_dir/assertions.log"
          fi
          ;;
        state_count)
          if [[ ! -f "$case_dir/dot/$arg1" ]]; then
            status="FAIL"
            notes="${notes:+$notes; }missing dot file $arg1"
          else
            local actual_count
            actual_count="$(count_states "$case_dir/dot/$arg1")"
            printf 'state_count|%s|%s\n' "$arg1" "$actual_count" >> "$case_dir/assertions.log"
            if [[ "$actual_count" != "$arg2" ]]; then
              status="FAIL"
              notes="${notes:+$notes; }state count mismatch for $arg1"
            fi
          fi
          ;;
      esac
    done < "$manifest"
  fi

  append_summary "$name" "$status" "success" "${notes:-ok}"
  [[ "$status" == "PASS" ]]
}

run_error_case() {
  local name="$1"
  local spec="$2"
  local error_file="$3"
  local case_dir="$RUN_DIR/$name"
  local status="PASS"
  local notes=""
  local expected

  mkdir -p "$case_dir"
  expected="$(< "$error_file")"

  if "$SEULEX_BIN" "$spec" "$case_dir/generated.cpp" "$case_dir/dot" \
      > "$case_dir/generate.stdout" 2> "$case_dir/generate.stderr"; then
    status="FAIL"
    notes="generator unexpectedly succeeded"
  else
    if ! grep -Fq "$expected" "$case_dir/generate.stderr"; then
      status="FAIL"
      notes="stderr missing expected substring"
    fi
  fi

  append_summary "$name" "$status" "negative" "${notes:-ok}"
  [[ "$status" == "PASS" ]]
}

run_generate_only_case() {
  local name="$1"
  local spec="$2"
  local max_seconds="$3"
  local case_dir="$RUN_DIR/$name"
  local status="PASS"
  local notes=""
  local elapsed=0

  mkdir -p "$case_dir/dot"
  SECONDS=0
  if ! "$SEULEX_BIN" "$spec" "$case_dir/generated.cpp" "$case_dir/dot" \
      > "$case_dir/generate.stdout" 2> "$case_dir/generate.stderr"; then
    status="FAIL"
    notes="generator failed"
  else
    elapsed=$SECONDS
    printf 'elapsed_seconds=%s\n' "$elapsed" > "$case_dir/perf.txt"
    for dot_name in merged_nfa.dot dfa.dot min_dfa.dot; do
      if [[ ! -f "$case_dir/dot/$dot_name" ]]; then
        status="FAIL"
        notes="${notes:+$notes; }missing dot file $dot_name"
      fi
    done
    if [[ ! -f "$case_dir/generated.cpp" ]]; then
      status="FAIL"
      notes="${notes:+$notes; }missing generated.cpp"
    fi
    if (( elapsed > max_seconds )); then
      status="FAIL"
      notes="${notes:+$notes; }generation took ${elapsed}s > ${max_seconds}s"
    fi
  fi

  append_summary "$name" "$status" "regression" "${notes:-${elapsed}s}"
  [[ "$status" == "PASS" ]]
}

run_runtime_perf_case() {
  local name="$1"
  local generated_cpp="$2"
  local iterations="$3"
  local expected_tokens="$4"
  local max_seconds="$5"
  local case_dir="$RUN_DIR/$name"
  local status="PASS"
  local notes=""
  local elapsed=0

  mkdir -p "$case_dir"
  cat > "$case_dir/perf_driver.cpp" <<'EOF'
#include <iostream>
#include <string>
#include <vector>

std::vector<int> tokenize(const std::string& source);

int main(int argc, char** argv) {
  if (argc != 3) {
    return 2;
  }
  const int iterations = std::stoi(argv[1]);
  const int expected = std::stoi(argv[2]);
  const std::string unit = "if ifa == = 42 @ ";
  std::string input;
  input.reserve(unit.size() * static_cast<std::size_t>(iterations));
  for (int index = 0; index < iterations; ++index) {
    input += unit;
  }
  const auto tokens = tokenize(input);
  std::cout << tokens.size() << '\n';
  return static_cast<int>(tokens.size()) == expected ? 0 : 1;
}
EOF

  if ! "$CXX_BIN" -std=c++17 "$generated_cpp" "$case_dir/perf_driver.cpp" -o "$case_dir/perf_driver" \
      > "$case_dir/compile.stdout" 2> "$case_dir/compile.stderr"; then
    status="FAIL"
    notes="performance driver compile failed"
  fi

  if [[ "$status" == "PASS" ]]; then
    SECONDS=0
    if ! "$case_dir/perf_driver" "$iterations" "$expected_tokens" \
        > "$case_dir/perf.stdout" 2> "$case_dir/perf.stderr"; then
      status="FAIL"
      notes="runtime token count mismatch"
    else
      elapsed=$SECONDS
      printf 'elapsed_seconds=%s\n' "$elapsed" > "$case_dir/perf.txt"
      if (( elapsed > max_seconds )); then
        status="FAIL"
        notes="runtime took ${elapsed}s > ${max_seconds}s"
      fi
    fi
  fi

  append_summary "$name" "$status" "perf" "${notes:-${elapsed}s}"
  [[ "$status" == "PASS" ]]
}

run_cli_self_test_case() {
  local name="$1"
  local max_seconds="$2"
  local case_dir="$RUN_DIR/$name"
  local status="PASS"
  local notes=""
  local elapsed=0

  mkdir -p "$case_dir"
  SECONDS=0
  if ! "$SEULEX_BIN" --self-test > "$case_dir/self_test.stdout" 2> "$case_dir/self_test.stderr"; then
    status="FAIL"
    notes="--self-test failed"
  else
    elapsed=$SECONDS
    printf 'elapsed_seconds=%s\n' "$elapsed" > "$case_dir/perf.txt"
    if (( elapsed > max_seconds )); then
      status="FAIL"
      notes="self-test took ${elapsed}s > ${max_seconds}s"
    fi
  fi

  append_summary "$name" "$status" "regression" "${notes:-${elapsed}s}"
  [[ "$status" == "PASS" ]]
}

cmake -S "$SEULEX_DIR" -B "$SEULEX_DIR/build" > "$RUN_DIR/cmake.configure.log" 2>&1
cmake --build "$SEULEX_DIR/build" > "$RUN_DIR/cmake.build.log" 2>&1

ASSERT_DRIVER_SRC="$RUN_DIR/assert_driver.cpp"
write_assert_driver "$ASSERT_DRIVER_SRC"

run_success_case "01_parser_sections" \
  "$CASE_ROOT/01_parser_sections.l" \
  "$EXPECT_ROOT/01_parser_sections.assert" || FAILURES=$((FAILURES + 1))

run_success_case "02_regex_features" \
  "$CASE_ROOT/02_regex_features.l" \
  "$EXPECT_ROOT/02_regex_features.assert" || FAILURES=$((FAILURES + 1))

run_success_case "03_negated_escape" \
  "$CASE_ROOT/03_negated_escape.l" \
  "$EXPECT_ROOT/03_negated_escape.assert" || FAILURES=$((FAILURES + 1))

run_success_case "04_longest_match_priority" \
  "$CASE_ROOT/04_longest_match_priority.l" \
  "$EXPECT_ROOT/04_longest_match_priority.assert" || FAILURES=$((FAILURES + 1))

run_success_case "05_minimization_merge" \
  "$CASE_ROOT/05_minimization_merge.l" \
  "$EXPECT_ROOT/05_minimization_merge.assert" || FAILURES=$((FAILURES + 1))

run_error_case "06_error_undefined_definition" \
  "$CASE_ROOT/06_error_undefined_definition.l" \
  "$EXPECT_ROOT/06_error_undefined_definition.error" || FAILURES=$((FAILURES + 1))

run_error_case "07_error_invalid_regex" \
  "$CASE_ROOT/07_error_invalid_regex.l" \
  "$EXPECT_ROOT/07_error_invalid_regex.error" || FAILURES=$((FAILURES + 1))

run_error_case "08_error_repeat_overflow" \
  "$CASE_ROOT/08_error_repeat_overflow.l" \
  "$EXPECT_ROOT/08_error_repeat_overflow.error" || FAILURES=$((FAILURES + 1))

run_error_case "09_error_unterminated_action" \
  "$CASE_ROOT/09_error_unterminated_action.l" \
  "$EXPECT_ROOT/09_error_unterminated_action.error" || FAILURES=$((FAILURES + 1))

run_generate_only_case "10_regression_minic_generate" \
  "$ROOT_DIR/resources/minic.l" \
  30 || FAILURES=$((FAILURES + 1))

run_generate_only_case "11_regression_c99_generate" \
  "$ROOT_DIR/resources/c99.l" \
  60 || FAILURES=$((FAILURES + 1))

run_runtime_perf_case "12_runtime_perf_long_stream" \
  "$RUN_DIR/04_longest_match_priority/generated.cpp" \
  5000 \
  30000 \
  10 || FAILURES=$((FAILURES + 1))

run_cli_self_test_case "13_cli_self_test" 120 || FAILURES=$((FAILURES + 1))

printf '\nTotal failures: %s\n' "$FAILURES" | tee -a "$SUMMARY_MD"
exit "$FAILURES"
