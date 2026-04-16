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
 * @brief 定义分析表构造、语法分析器代码生成以及总控驱动接口。
 */

namespace seu_yacc {

/**
 * @brief 中期报告中定义的一行分析表结构。
 */
class parse_table_item {
 public:
  /**
   * @brief 根据一个 LR 状态构造空的分析表行。
   *
   * 时间复杂度：O(1)。
   */
  explicit parse_table_item(LRnode x);

  /**
   * @brief 返回当前行中的 ACTION 表项。
   *
   * 时间复杂度：O(A)，其中 A 为 ACTION 区域宽度。
   */
  std::map<std::string, std::string> GetAction() const;

  /**
   * @brief 返回当前行中的 GOTO 表项。
   *
   * 时间复杂度：O(G)，其中 G 为 GOTO 区域宽度。
   */
  std::map<std::string, int> GetGoto() const;

  /**
   * @brief 添加或覆盖一个 ACTION 表项。
   *
   * 时间复杂度：O(log A)。
   */
  void AddtoAction(const std::string& ter, const std::string& rs);

  /**
   * @brief 添加或覆盖一个 GOTO 表项。
   *
   * 时间复杂度：O(log G)。
   */
  void AddtoGoto(const std::string& nonter, int s);

  /**
   * @brief 返回该行对应的 LR 状态编号。
   *
   * 时间复杂度：O(1)。
   */
  int State() const;

 private:
  int state = 0;
  std::map<std::string, std::string> action;
  std::map<std::string, int> gotos;
};

/**
 * @brief 根据 LR 自动机构造 ACTION/GOTO 分析表。
 */
class ParseTableBuilder {
 public:
  /**
   * @brief 构造规范 LR(1) 分析表。
   *
   * 时间复杂度：O(S * I * log M)，其中 S 为状态数，I 为项目数，
   * M 为单行表项规模。
   */
  std::vector<parse_table_item> buildLR1Table(
      const LRPDA& automaton,
      const YaccSpecification& specification,
      const std::string& start_symbol,
      std::vector<std::string>* conflicts = nullptr) const;

  /**
   * @brief 构造 LALR(1) 分析表。
   *
   * 时间复杂度：与合并后状态上的 `buildLR1Table` 同阶。
   */
  std::vector<parse_table_item> buildLALRTable(
      const LRPDA& automaton,
      const YaccSpecification& specification,
      const std::string& start_symbol,
      std::vector<std::string>* conflicts = nullptr) const;
};

/**
 * @brief 负责输出可独立编译的语法分析器源文件与 token 头文件。
 */
class ParserCodeGenerator {
 public:
  /**
   * @brief 生成独立的语法分析器实现代码。
   *
   * 时间复杂度：O(S * T + P)，其中 S 为状态数，T 为终结符数量，
   * P 为输出的产生式元数据总规模。
   */
  void emitParser(const std::vector<parse_table_item>& table,
                  const LRPDA& automaton,
                  const YaccSpecification& specification,
                  const std::string& start_symbol,
                  const std::string& out_cpp_path,
                  const std::string& out_header_path) const;
};

/**
 * @brief 封装 seuYacc 全流程的总控入口。
 */
class SeuYaccDriver {
 public:
  /**
   * @brief 执行完整的 seuYacc 生成流程。
   *
   * 时间复杂度：由 LR(1) 自动机构造阶段主导。
   */
  void generate(const std::string& yacc_path,
                const std::string& out_cpp_path,
                const std::string& out_header_path,
                const std::string& mode) const;

  /**
   * @brief 执行内置冒烟测试。
   *
   * 时间复杂度：与内置测试的生成工作量成正比。
   */
  bool runSelfTests(const std::string& workspace_root) const;
};

}  // 命名空间 seu_yacc
