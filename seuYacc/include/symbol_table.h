#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "yacc_parser.h"

/**
 * @file symbol_table.h
 * @brief Grammar symbol tables and semantic symbol-table management.
 */

namespace seu_yacc {

extern std::vector<operators> ops;
extern std::vector<std::string> terminals;
extern std::vector<std::string> nonterminals;
extern std::vector<producer> producers;

/**
 * @brief One semantic symbol entry managed by the generated parser runtime.
 */
struct SemanticSymbol {
  std::string name;
  std::string type;
  int scope_level = 0;
  int offset = 0;
  bool is_function = false;
  bool is_parameter = false;
};

/**
 * @brief Maintain grammar symbol tables and semantic scopes.
 */
class SymbolTableManager {
 public:
  /**
   * @brief Reset all grammar and semantic tables.
   *
   * Complexity: O(S), where S is the total stored state size.
   */
  void reset();

  /**
   * @brief Register one terminal symbol if it is new.
   *
   * Complexity: amortized O(1).
   */
  void registerTerminal(const std::string& symbol);

  /**
   * @brief Register one nonterminal symbol if it is new.
   *
   * Complexity: amortized O(1).
   */
  void registerNonterminal(const std::string& symbol);

  /**
   * @brief Append one precedence group.
   *
   * Complexity: O(K), where K is the number of symbols in the group.
   */
  void addOperatorGroup(const operators& group,
                        const std::vector<std::string>& precedence_symbols);

  /**
   * @brief Append one production to the report-defined table.
   *
   * Complexity: O(R), where R is the right-hand-side length.
   */
  void addProducer(const producer& production);

  /**
   * @brief Set the grammar start symbol.
   *
   * Complexity: O(1).
   */
  void setStartSymbol(const std::string& symbol);

  /**
   * @brief Return the grammar start symbol.
   *
   * Complexity: O(1).
   */
  const std::string& startSymbol() const;

  /**
   * @brief Record a semantic type for a token declaration.
   *
   * Complexity: amortized O(1).
   */
  void setTokenType(const std::string& symbol, const std::string& type_name);

  /**
   * @brief Record a semantic type for a nonterminal declaration.
   *
   * Complexity: amortized O(1).
   */
  void setNonterminalType(const std::string& symbol, const std::string& type_name);

  /**
   * @brief Query whether a symbol is terminal.
   *
   * Complexity: amortized O(1).
   */
  bool isTerminal(const std::string& symbol) const;

  /**
   * @brief Query whether a symbol is nonterminal.
   *
   * Complexity: amortized O(1).
   */
  bool isNonterminal(const std::string& symbol) const;

  /**
   * @brief Return the integer code for a terminal symbol.
   *
   * Complexity: amortized O(1).
   */
  int terminalId(const std::string& symbol) const;

  /**
   * @brief Return the integer code for a nonterminal symbol.
   *
   * Complexity: amortized O(1).
   */
  int nonterminalId(const std::string& symbol) const;

  /**
   * @brief Return the declared semantic type of a symbol if present.
   *
   * Complexity: amortized O(1).
   */
  std::string symbolType(const std::string& symbol) const;

  /**
   * @brief Return precedence level and associativity for one terminal if present.
   *
   * Complexity: amortized O(1).
   */
  std::pair<int, std::string> precedenceOf(const std::string& symbol) const;

  /**
   * @brief Enter a new semantic scope.
   *
   * Complexity: amortized O(1).
   */
  void enterScope();

  /**
   * @brief Exit the current semantic scope.
   *
   * Complexity: amortized O(1).
   */
  void exitScope();

  /**
   * @brief Declare one semantic symbol in the current scope.
   *
   * Complexity: amortized O(1).
   */
  void declareSymbol(const SemanticSymbol& symbol);

  /**
   * @brief Look up one semantic symbol from inner to outer scope.
   *
   * Complexity: O(D), where D is the scope depth.
   */
  const SemanticSymbol* lookupSymbol(const std::string& name) const;

  /**
   * @brief Return all semantic scopes for inspection.
   *
   * Complexity: O(1).
   */
  const std::vector<std::unordered_map<std::string, SemanticSymbol>>& semanticScopes() const;

 private:
  std::map<std::string, int> terminal_ids_;
  std::map<std::string, int> nonterminal_ids_;
  std::map<std::string, std::string> token_types_;
  std::map<std::string, std::string> nonterminal_types_;
  std::map<std::string, std::pair<int, std::string>> precedence_;
  std::vector<std::unordered_map<std::string, SemanticSymbol>> scopes_;
  std::string start_symbol_;
  int next_terminal_id_ = 256;
  int next_nonterminal_id_ = 1;
};

}  // namespace seu_yacc
