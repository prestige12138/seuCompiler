#pragma once

#include <string>
#include <unordered_map>
#include <vector>

/**
 * @file symbol_table.h
 * @brief Semantic symbol-table support for the intermediate-code stage.
 */

namespace seu_icg {

/**
 * @brief One semantic symbol entry.
 */
struct SymbolEntry {
  std::string name;
  std::string type;
  int scope_level = 0;
  int offset = -1;
  bool is_function = false;
  bool is_parameter = false;
};

/**
 * @brief Scope-aware semantic symbol table.
 */
class SymbolTable {
 public:
  /**
   * @brief Reset the symbol table to one empty global scope.
   *
   * Complexity: O(S), where S is the total number of stored symbols.
   */
  void reset();

  /**
   * @brief Enter one nested local scope.
   *
   * Complexity: amortized O(1).
   */
  void enterScope();

  /**
   * @brief Exit the current scope while preserving outer scopes.
   *
   * Complexity: O(K), where K is the number of symbols in the current scope.
   */
  void exitScope();

  /**
   * @brief Declare one symbol in the current scope.
   *
   * Duplicate declarations in the same scope are rejected.
   * Complexity: amortized O(1).
   */
  bool declare(const SymbolEntry& entry);

  /**
   * @brief Declare one variable.
   *
   * Offsets are auto-assigned when `offset < 0`.
   * Complexity: amortized O(1).
   */
  bool declareVariable(const std::string& name, const std::string& type);

  /**
   * @brief Declare one function in the global scope.
   *
   * Complexity: amortized O(1).
   */
  bool declareFunction(const std::string& name, const std::string& return_type);

  /**
   * @brief Declare one parameter in the current scope.
   *
   * Complexity: amortized O(1).
   */
  bool declareParameter(const std::string& name, const std::string& type);

  /**
   * @brief Look up one symbol from inner to outer scope.
   *
   * Complexity: O(D), where D is the scope depth.
   */
  const SymbolEntry* lookup(const std::string& name) const;

  /**
   * @brief Look up one symbol only in the current scope.
   *
   * Complexity: amortized O(1).
   */
  const SymbolEntry* lookupCurrentScope(const std::string& name) const;

  /**
   * @brief Return the current scope level.
   *
   * Complexity: O(1).
   */
  int currentScopeLevel() const;

 private:
  struct ScopeFrame {
    std::unordered_map<std::string, SymbolEntry> symbols;
    int nextLocalOffset = 0;
    int nextParameterOffset = 0;
  };

  std::vector<ScopeFrame> scopes_;
  int nextGlobalOffset_ = 0;
};

}  // namespace seu_icg
