#pragma once

#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "yacc_parser.h"

/**
 * @file symbol_table.h
 * @brief 定义文法符号表和语义符号表的统一管理接口。
 */

namespace seu_yacc {

extern std::vector<operators> ops;
extern std::vector<std::string> terminals;
extern std::vector<std::string> nonterminals;
extern std::vector<producer> producers;

/**
 * @brief 生成的语法分析器运行时维护的一条语义符号记录。
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
 * @brief 维护文法层符号信息以及语义作用域栈。
 */
class SymbolTableManager {
 public:
  /**
   * @brief 重置全部文法表和语义表。
   *
   * 时间复杂度：O(S)，其中 S 为所有已存储状态的总规模。
   */
  void reset();

  /**
   * @brief 若终结符尚未出现，则将其登记到终结符表中。
   *
   * 时间复杂度：均摊 O(1)。
   */
  void registerTerminal(const std::string& symbol);

  /**
   * @brief 若非终结符尚未出现，则将其登记到非终结符表中。
   *
   * 时间复杂度：均摊 O(1)。
   */
  void registerNonterminal(const std::string& symbol);

  /**
   * @brief 追加一个优先级与结合性分组。
   *
   * 时间复杂度：O(K)，其中 K 为该分组中的符号数量。
   */
  void addOperatorGroup(const operators& group,
                        const std::vector<std::string>& precedence_symbols);

  /**
   * @brief 向报告规定的产生式表中追加一条产生式。
   *
   * 时间复杂度：O(R)，其中 R 为产生式右部长度。
   */
  void addProducer(const producer& production);

  /**
   * @brief 设置文法开始符号。
   *
   * 时间复杂度：O(1)。
   */
  void setStartSymbol(const std::string& symbol);

  /**
   * @brief 返回当前文法开始符号。
   *
   * 时间复杂度：O(1)。
   */
  const std::string& startSymbol() const;

  /**
   * @brief 为 token 声明记录语义类型。
   *
   * 时间复杂度：均摊 O(1)。
   */
  void setTokenType(const std::string& symbol, const std::string& type_name);

  /**
   * @brief 为非终结符声明记录语义类型。
   *
   * 时间复杂度：均摊 O(1)。
   */
  void setNonterminalType(const std::string& symbol, const std::string& type_name);

  /**
   * @brief 查询某个符号是否为终结符。
   *
   * 时间复杂度：均摊 O(1)。
   */
  bool isTerminal(const std::string& symbol) const;

  /**
   * @brief 查询某个符号是否为非终结符。
   *
   * 时间复杂度：均摊 O(1)。
   */
  bool isNonterminal(const std::string& symbol) const;

  /**
   * @brief 返回终结符对应的整数编号。
   *
   * 时间复杂度：均摊 O(1)。
   */
  int terminalId(const std::string& symbol) const;

  /**
   * @brief 返回非终结符对应的整数编号。
   *
   * 时间复杂度：均摊 O(1)。
   */
  int nonterminalId(const std::string& symbol) const;

  /**
   * @brief 返回某个符号声明过的语义类型。
   *
   * 时间复杂度：均摊 O(1)。
   */
  std::string symbolType(const std::string& symbol) const;

  /**
   * @brief 返回终结符对应的优先级和结合性信息。
   *
   * 时间复杂度：均摊 O(1)。
   */
  std::pair<int, std::string> precedenceOf(const std::string& symbol) const;

  /**
   * @brief 进入一个新的语义作用域。
   *
   * 时间复杂度：均摊 O(1)。
   */
  void enterScope();

  /**
   * @brief 退出当前语义作用域。
   *
   * 时间复杂度：均摊 O(1)。
   */
  void exitScope();

  /**
   * @brief 在当前作用域中声明一个语义符号。
   *
   * 时间复杂度：均摊 O(1)。
   */
  void declareSymbol(const SemanticSymbol& symbol);

  /**
   * @brief 按由内到外的顺序查找一个语义符号。
   *
   * 时间复杂度：O(D)，其中 D 为作用域深度。
   */
  const SemanticSymbol* lookupSymbol(const std::string& name) const;

  /**
   * @brief 返回全部语义作用域，便于调试或测试检查。
   *
   * 时间复杂度：O(1)。
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

}  // 命名空间 seu_yacc
