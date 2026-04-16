/**
 * @file symbol_table.cpp
 * @brief 实现 seuYacc 的文法符号表和语义作用域辅助逻辑。
 */

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

/**
 * @brief 判断一个字符串是否已存在于顺序表中。
 */
bool contains(const std::vector<std::string>& data, const std::string& value) {
  return std::find(data.begin(), data.end(), value) != data.end();
}

/**
 * @brief 返回与报告兼容的稳定结合性字符串指针。
 */
char* stableAssoc(const std::string& assoc) {
  if (assoc == "left") {
    return const_cast<char*>("left");
  }
  if (assoc == "right") {
    return const_cast<char*>("right");
  }
  return const_cast<char*>("nonassoc");
}

}  // 匿名命名空间

/**
 * @brief 重置文法层和语义层的全部符号表状态。
 */
void SymbolTableManager::reset() {
  // 这些全局向量直接复用报告中的数据结构，因此完整重置时既要清空
  // 新增的辅助映射，也要同步清掉旧版向量表。
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

/**
 * @brief 注册一个新的终结符，并为其分配编号。
 */
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

/**
 * @brief 注册一个新的非终结符，并为其分配编号。
 */
void SymbolTableManager::registerNonterminal(const std::string& symbol) {
  if (symbol.empty() || contains(nonterminals, symbol)) {
    return;
  }
  nonterminals.push_back(symbol);
  nonterminal_ids_[symbol] = next_nonterminal_id_++;
}

/**
 * @brief 追加一个优先级分组，并建立符号到优先级的映射。
 */
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

/**
 * @brief 向产生式表追加一条产生式。
 */
void SymbolTableManager::addProducer(const producer& production) {
  producers.push_back(production);
}

/**
 * @brief 设置文法开始符号。
 */
void SymbolTableManager::setStartSymbol(const std::string& symbol) {
  start_symbol_ = symbol;
}

/**
 * @brief 返回当前文法开始符号。
 */
const std::string& SymbolTableManager::startSymbol() const {
  return start_symbol_;
}

/**
 * @brief 记录某个 token 的语义类型。
 */
void SymbolTableManager::setTokenType(const std::string& symbol, const std::string& type_name) {
  token_types_[symbol] = type_name;
}

/**
 * @brief 记录某个非终结符的语义类型。
 */
void SymbolTableManager::setNonterminalType(const std::string& symbol,
                                            const std::string& type_name) {
  nonterminal_types_[symbol] = type_name;
}

/**
 * @brief 判断给定符号是否是终结符。
 */
bool SymbolTableManager::isTerminal(const std::string& symbol) const {
  return terminal_ids_.find(symbol) != terminal_ids_.end();
}

/**
 * @brief 判断给定符号是否是非终结符。
 */
bool SymbolTableManager::isNonterminal(const std::string& symbol) const {
  return nonterminal_ids_.find(symbol) != nonterminal_ids_.end();
}

/**
 * @brief 返回终结符对应的整数编号。
 */
int SymbolTableManager::terminalId(const std::string& symbol) const {
  const auto found = terminal_ids_.find(symbol);
  if (found == terminal_ids_.end()) {
    throw std::runtime_error("unknown terminal symbol: " + symbol);
  }
  return found->second;
}

/**
 * @brief 返回非终结符对应的整数编号。
 */
int SymbolTableManager::nonterminalId(const std::string& symbol) const {
  const auto found = nonterminal_ids_.find(symbol);
  if (found == nonterminal_ids_.end()) {
    throw std::runtime_error("unknown nonterminal symbol: " + symbol);
  }
  return found->second;
}

/**
 * @brief 返回符号声明时记录的语义类型字符串。
 */
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

/**
 * @brief 返回符号对应的优先级与结合性信息。
 */
std::pair<int, std::string> SymbolTableManager::precedenceOf(const std::string& symbol) const {
  const auto found = precedence_.find(symbol);
  if (found == precedence_.end()) {
    return {-1, ""};
  }
  return found->second;
}

/**
 * @brief 进入新的语义作用域。
 */
void SymbolTableManager::enterScope() {
  scopes_.push_back({});
}

/**
 * @brief 退出当前语义作用域。
 */
void SymbolTableManager::exitScope() {
  if (!scopes_.empty()) {
    scopes_.pop_back();
  }
}

/**
 * @brief 在当前语义作用域中登记一个符号。
 */
void SymbolTableManager::declareSymbol(const SemanticSymbol& symbol) {
  if (scopes_.empty()) {
    scopes_.push_back({});
  }
  scopes_.back()[symbol.name] = symbol;
}

/**
 * @brief 按由内到外的顺序查找一个语义符号。
 */
const SemanticSymbol* SymbolTableManager::lookupSymbol(const std::string& name) const {
  for (auto it = scopes_.rbegin(); it != scopes_.rend(); ++it) {
    const auto found = it->find(name);
    if (found != it->end()) {
      return &found->second;
    }
  }
  return nullptr;
}

/**
 * @brief 返回全部语义作用域，供调试和测试检查使用。
 */
const std::vector<std::unordered_map<std::string, SemanticSymbol>>&
SymbolTableManager::semanticScopes() const {
  return scopes_;
}

}  // 命名空间 seu_yacc
