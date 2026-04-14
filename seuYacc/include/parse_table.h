#pragma once

#include <map>
#include <string>
#include <vector>

#include "lalr_converter.h"
#include "lr1_pda.h"
#include "symbol_table.h"
#include "yacc_parser.h"

/**
 * @file parse_table.h
 * @brief Parse-table construction, parser emission, and driver interfaces.
 */

namespace seu_yacc {

/**
 * @brief Report-defined parse-table row.
 */
class parse_table_item {
 public:
  /**
   * @brief Construct one empty row from an LR node.
   *
   * Complexity: O(1).
   */
  explicit parse_table_item(LRnode x);

  /**
   * @brief Return ACTION entries.
   *
   * Complexity: O(A), where A is the row width.
   */
  std::map<std::string, std::string> GetAction() const;

  /**
   * @brief Return GOTO entries.
   *
   * Complexity: O(G), where G is the row width.
   */
  std::map<std::string, int> GetGoto() const;

  /**
   * @brief Add or overwrite one ACTION entry.
   *
   * Complexity: O(log A).
   */
  void AddtoAction(const std::string& ter, const std::string& rs);

  /**
   * @brief Add or overwrite one GOTO entry.
   *
   * Complexity: O(log G).
   */
  void AddtoGoto(const std::string& nonter, int s);

  /**
   * @brief Return the source LR-state index.
   *
   * Complexity: O(1).
   */
  int State() const;

 private:
  int state = 0;
  std::map<std::string, std::string> action;
  std::map<std::string, int> gotos;
};

/**
 * @brief Build ACTION/GOTO tables from LR automata.
 */
class ParseTableBuilder {
 public:
  /**
   * @brief Build the canonical LR(1) parse table.
   *
   * Complexity: O(S * I * log M), where S is states, I is item count, M is row size.
   */
  std::vector<parse_table_item> buildLR1Table(
      const LRPDA& automaton,
      const YaccSpecification& specification,
      const std::string& start_symbol,
      std::vector<std::string>* conflicts = nullptr) const;

  /**
   * @brief Build the LALR(1) parse table.
   *
   * Complexity: same order as buildLR1Table on merged states.
   */
  std::vector<parse_table_item> buildLALRTable(
      const LRPDA& automaton,
      const YaccSpecification& specification,
      const std::string& start_symbol,
      std::vector<std::string>* conflicts = nullptr) const;
};

/**
 * @brief Emit standalone parser source and token header code.
 */
class ParserCodeGenerator {
 public:
  /**
   * @brief Emit a standalone parser implementation.
   *
   * Complexity: O(S * T + P), where S is state count, T is terminal count, and
   * P is total emitted production metadata size.
   */
  void emitParser(const std::vector<parse_table_item>& table,
                  const LRPDA& automaton,
                  const YaccSpecification& specification,
                  const std::string& start_symbol,
                  const std::string& out_cpp_path,
                  const std::string& out_header_path) const;
};

/**
 * @brief End-to-end seuYacc orchestration facade.
 */
class SeuYaccDriver {
 public:
  /**
   * @brief Run the full seuYacc generation pipeline.
   *
   * Complexity: dominated by LR(1) construction.
   */
  void generate(const std::string& yacc_path,
                const std::string& out_cpp_path,
                const std::string& out_header_path,
                const std::string& mode) const;

  /**
   * @brief Run built-in smoke tests.
   *
   * Complexity: proportional to the embedded generation workload.
   */
  bool runSelfTests(const std::string& workspace_root) const;
};

}  // namespace seu_yacc
