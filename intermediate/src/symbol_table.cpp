/**
 * @file symbol_table.cpp
 * @brief 实现中间代码阶段使用的分层语义符号表。
 */

#include "symbol_table.h"

#include <algorithm>

namespace seu_icg {
namespace {

/**
 * @brief 根据类型名返回默认字宽，用于自动计算偏移量。
 */
int typeWidth(const std::string& type) {
  if (type == "char" || type == "bool") {
    return 1;
  }
  if (type == "short") {
    return 2;
  }
  if (type == "double" || type == "long long") {
    return 8;
  }
  return 4;
}

}  // 匿名命名空间

/**
 * @brief 将符号表重置为仅包含全局作用域的初始状态。
 */
void SymbolTable::reset() {
  scopes_.clear();
  scopes_.push_back({});
  nextGlobalOffset_ = 0;
}

/**
 * @brief 进入新的局部作用域。
 */
void SymbolTable::enterScope() {
  if (scopes_.empty()) {
    reset();
  }
  scopes_.push_back({});
}

/**
 * @brief 退出当前局部作用域。
 */
void SymbolTable::exitScope() {
  if (scopes_.size() > 1) {
    scopes_.pop_back();
  }
}

/**
 * @brief 在当前作用域中登记一个符号。
 */
bool SymbolTable::declare(const SymbolEntry& entry) {
  if (scopes_.empty()) {
    reset();
  }
  ScopeFrame& scope = scopes_.back();
  if (scope.symbols.find(entry.name) != scope.symbols.end()) {
    return false;
  }

  SymbolEntry stored = entry;
  stored.scope_level = currentScopeLevel();

  if (!stored.is_function && stored.offset < 0) {
    // 偏移量按需分配，这样调用方既可以显式指定布局，
    // 也可以让符号表按照类型宽度自动推导。
    const int width = typeWidth(stored.type);
    if (stored.is_parameter) {
      stored.offset = scope.nextParameterOffset;
      scope.nextParameterOffset += width;
    } else if (stored.scope_level == 0) {
      stored.offset = nextGlobalOffset_;
      nextGlobalOffset_ += width;
    } else {
      stored.offset = scope.nextLocalOffset;
      scope.nextLocalOffset += width;
    }
  }

  scope.symbols[stored.name] = stored;
  return true;
}

/**
 * @brief 以普通变量的形式声明一个符号。
 */
bool SymbolTable::declareVariable(const std::string& name, const std::string& type) {
  SymbolEntry entry;
  entry.name = name;
  entry.type = type;
  return declare(entry);
}

/**
 * @brief 在全局作用域中声明一个函数符号。
 */
bool SymbolTable::declareFunction(const std::string& name, const std::string& return_type) {
  if (scopes_.empty()) {
    reset();
  }
  if (scopes_.size() > 1) {
    return false;
  }
  SymbolEntry entry;
  entry.name = name;
  entry.type = return_type;
  entry.is_function = true;
  entry.offset = 0;
  return declare(entry);
}

/**
 * @brief 在当前作用域中声明一个参数符号。
 */
bool SymbolTable::declareParameter(const std::string& name, const std::string& type) {
  SymbolEntry entry;
  entry.name = name;
  entry.type = type;
  entry.is_parameter = true;
  return declare(entry);
}

/**
 * @brief 从内层到外层逐层查找指定名字的符号。
 */
const SymbolEntry* SymbolTable::lookup(const std::string& name) const {
  for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
    const auto found = it->symbols.find(name);
    if (found != it->symbols.end()) {
      return &found->second;
    }
  }
  return nullptr;
}

/**
 * @brief 只在当前作用域中查找指定名字的符号。
 */
const SymbolEntry* SymbolTable::lookupCurrentScope(const std::string& name) const {
  if (scopes_.empty()) {
    return nullptr;
  }
  const auto found = scopes_.back().symbols.find(name);
  if (found == scopes_.back().symbols.end()) {
    return nullptr;
  }
  return &found->second;
}

/**
 * @brief 返回当前所处的作用域层级。
 */
int SymbolTable::currentScopeLevel() const {
  if (scopes_.empty()) {
    return 0;
  }
  return static_cast<int>(scopes_.size()) - 1;
}

}  // 命名空间 seu_icg
