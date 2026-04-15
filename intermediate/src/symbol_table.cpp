/**
 * @file symbol_table.cpp
 * @brief Scope-aware symbol table used by the intermediate-code generator.
 */

#include "symbol_table.h"

#include <algorithm>

namespace seu_icg {
namespace {

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

}  // namespace

void SymbolTable::reset() {
  scopes_.clear();
  scopes_.push_back({});
  nextGlobalOffset_ = 0;
}

void SymbolTable::enterScope() {
  if (scopes_.empty()) {
    reset();
  }
  scopes_.push_back({});
}

void SymbolTable::exitScope() {
  if (scopes_.size() > 1) {
    scopes_.pop_back();
  }
}

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
    // Offsets are assigned lazily so callers can either provide explicit layout
    // information or let the table synthesize one from the declared type width.
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

bool SymbolTable::declareVariable(const std::string& name, const std::string& type) {
  SymbolEntry entry;
  entry.name = name;
  entry.type = type;
  return declare(entry);
}

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

bool SymbolTable::declareParameter(const std::string& name, const std::string& type) {
  SymbolEntry entry;
  entry.name = name;
  entry.type = type;
  entry.is_parameter = true;
  return declare(entry);
}

const SymbolEntry* SymbolTable::lookup(const std::string& name) const {
  for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
    const auto found = it->symbols.find(name);
    if (found != it->symbols.end()) {
      return &found->second;
    }
  }
  return nullptr;
}

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

int SymbolTable::currentScopeLevel() const {
  if (scopes_.empty()) {
    return 0;
  }
  return static_cast<int>(scopes_.size()) - 1;
}

}  // namespace seu_icg
