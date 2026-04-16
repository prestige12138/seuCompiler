#pragma once

#include <string>
#include <unordered_map>
#include <vector>

/**
 * @file symbol_table.h
 * @brief 定义中间代码阶段使用的语义符号表接口。
 */

namespace seu_icg {

/**
 * @brief 一条语义符号记录。
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
 * @brief 支持作用域嵌套的语义符号表。
 */
class SymbolTable {
 public:
  /**
   * @brief 将符号表重置为仅包含空全局作用域的初始状态。
   *
   * 时间复杂度：O(S)，其中 S 为当前保存的符号总数。
   */
  void reset();

  /**
   * @brief 进入一个新的局部作用域。
   *
   * 时间复杂度：均摊 O(1)。
   */
  void enterScope();

  /**
   * @brief 退出当前作用域，并保留外层作用域信息。
   *
   * 时间复杂度：O(K)，其中 K 为当前作用域内的符号数。
   */
  void exitScope();

  /**
   * @brief 在当前作用域中声明一个符号。
   *
   * 同一作用域内的重复声明会被拒绝。
   * 时间复杂度：均摊 O(1)。
   */
  bool declare(const SymbolEntry& entry);

  /**
   * @brief 声明一个变量符号。
   *
   * 当 `offset < 0` 时会自动分配偏移量。
   * 时间复杂度：均摊 O(1)。
   */
  bool declareVariable(const std::string& name, const std::string& type);

  /**
   * @brief 在全局作用域中声明一个函数。
   *
   * 时间复杂度：均摊 O(1)。
   */
  bool declareFunction(const std::string& name, const std::string& return_type);

  /**
   * @brief 在当前作用域中声明一个函数参数。
   *
   * 时间复杂度：均摊 O(1)。
   */
  bool declareParameter(const std::string& name, const std::string& type);

  /**
   * @brief 按由内到外的顺序查找符号。
   *
   * 时间复杂度：O(D)，其中 D 为作用域深度。
   */
  const SymbolEntry* lookup(const std::string& name) const;

  /**
   * @brief 只在当前作用域中查找符号。
   *
   * 时间复杂度：均摊 O(1)。
   */
  const SymbolEntry* lookupCurrentScope(const std::string& name) const;

  /**
   * @brief 返回当前作用域层级。
   *
   * 时间复杂度：O(1)。
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

}  // 命名空间 seu_icg
