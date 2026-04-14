#include "symbol_table.h"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace seu_yacc {

std::vector<operators> ops;
std::vector<std::string> terminals;
std::vector<std::string> nonterminals;
std::vector<producer> producers;

namespace {

bool contains(const std::vector<std::string>& data, const std::string& value) {
  return std::find(data.begin(), data.end(), value) != data.end();
}

char* stableAssoc(const std::string& assoc) {
  if (assoc == "left") {
    return const_cast<char*>("left");
  }
  if (assoc == "right") {
    return const_cast<char*>("right");
  }
  return const_cast<char*>("nonassoc");
}

}  // namespace

void SymbolTableManager::reset() {
  ops.clear();
  terminals.clear();
  nonterminals.clear();
  producers.clear();
  terminal_ids_.clear();
  nonterminal_ids_.clear();
  token_types_.clear();
  nonterminal_types_.clear();
  precedence_.clear();
  scopes_.clear();
  start_symbol_.clear();
  next_terminal_id_ = 256;
  next_nonterminal_id_ = 1;
}

void SymbolTableManager::registerTerminal(const std::string& symbol) {
  if (symbol.empty() || contains(terminals, symbol)) {
    return;
  }
  terminals.push_back(symbol);
  if (symbol.size() >= 3 && symbol.front() == '\'' && symbol.back() == '\'' &&
      symbol[symbol.size() - 2] != '\\') {
    terminal_ids_[symbol] = static_cast<unsigned char>(symbol[1]);
    return;
  }
  if (symbol == "$") {
    terminal_ids_[symbol] = 0;
    return;
  }
  terminal_ids_[symbol] = next_terminal_id_++;
}

void SymbolTableManager::registerNonterminal(const std::string& symbol) {
  if (symbol.empty() || contains(nonterminals, symbol)) {
    return;
  }
  nonterminals.push_back(symbol);
  nonterminal_ids_[symbol] = next_nonterminal_id_++;
}

void SymbolTableManager::addOperatorGroup(const operators& group,
                                          const std::vector<std::string>& precedence_symbols) {
  operators copied = group;
  if (copied.rl == nullptr) {
    copied.rl = stableAssoc("nonassoc");
  }
  ops.push_back(copied);
  for (const std::string& symbol : precedence_symbols) {
    precedence_[symbol] = {copied.level, copied.rl == nullptr ? "" : copied.rl};
  }
}

void SymbolTableManager::addProducer(const producer& production) {
  producers.push_back(production);
}

void SymbolTableManager::setStartSymbol(const std::string& symbol) {
  start_symbol_ = symbol;
}

const std::string& SymbolTableManager::startSymbol() const {
  return start_symbol_;
}

void SymbolTableManager::setTokenType(const std::string& symbol, const std::string& type_name) {
  token_types_[symbol] = type_name;
}

void SymbolTableManager::setNonterminalType(const std::string& symbol,
                                            const std::string& type_name) {
  nonterminal_types_[symbol] = type_name;
}

bool SymbolTableManager::isTerminal(const std::string& symbol) const {
  return terminal_ids_.find(symbol) != terminal_ids_.end();
}

bool SymbolTableManager::isNonterminal(const std::string& symbol) const {
  return nonterminal_ids_.find(symbol) != nonterminal_ids_.end();
}

int SymbolTableManager::terminalId(const std::string& symbol) const {
  const auto found = terminal_ids_.find(symbol);
  if (found == terminal_ids_.end()) {
    throw std::runtime_error("unknown terminal symbol: " + symbol);
  }
  return found->second;
}

int SymbolTableManager::nonterminalId(const std::string& symbol) const {
  const auto found = nonterminal_ids_.find(symbol);
  if (found == nonterminal_ids_.end()) {
    throw std::runtime_error("unknown nonterminal symbol: " + symbol);
  }
  return found->second;
}

std::string SymbolTableManager::symbolType(const std::string& symbol) const {
  const auto token = token_types_.find(symbol);
  if (token != token_types_.end()) {
    return token->second;
  }
  const auto nonterminal = nonterminal_types_.find(symbol);
  if (nonterminal != nonterminal_types_.end()) {
    return nonterminal->second;
  }
  return "";
}

std::pair<int, std::string> SymbolTableManager::precedenceOf(const std::string& symbol) const {
  const auto found = precedence_.find(symbol);
  if (found == precedence_.end()) {
    return {-1, ""};
  }
  return found->second;
}

void SymbolTableManager::enterScope() {
  scopes_.push_back({});
}

void SymbolTableManager::exitScope() {
  if (!scopes_.empty()) {
    scopes_.pop_back();
  }
}

void SymbolTableManager::declareSymbol(const SemanticSymbol& symbol) {
  if (scopes_.empty()) {
    scopes_.push_back({});
  }
  scopes_.back()[symbol.name] = symbol;
}

const SemanticSymbol* SymbolTableManager::lookupSymbol(const std::string& name) const {
  for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
    const auto found = it->find(name);
    if (found != it->end()) {
      return &found->second;
    }
  }
  return nullptr;
}

const std::vector<std::unordered_map<std::string, SemanticSymbol>>&
SymbolTableManager::semanticScopes() const {
  return scopes_;
}

}  // namespace seu_yacc
