#pragma once

#include <string>

#include "intermediate_code.h"
#include "symbol_table.h"

/**
 * @file tri_addr_generator.h
 * @brief 定义从报告规定 AST 生成三地址码的接口。
 */

namespace seu_icg {

/**
 * @brief 根据 AST 生成三地址码。
 */
class TriAddrGenerator {
 public:
  /**
   * @brief 构造一个三地址码生成器，并可绑定外部符号表。
   *
   * 时间复杂度：O(1)。
   */
  explicit TriAddrGenerator(SymbolTable* symbols = nullptr);

  /**
   * @brief 为给定 AST 根结点生成完整的中间代码。
   *
   * 时间复杂度：O(N)，其中 N 为 AST 结点总数。
   */
  IntermediateCode generate(ASTNode* root);

 private:
  IntermediateCode code_;
  SymbolTable* symbols_ = nullptr;
  int tempCounter_ = 0;

  /**
   * @brief 递归生成表达式，并返回结果所在的临时变量或字面值。
   */
  std::string emitExpr(ASTNode* node);

  /**
   * @brief 生成条件表达式所需的求值代码，并返回条件值名。
   */
  std::string emitCondition(ASTNode* node);

  /**
   * @brief 根据结点类型分发语句生成逻辑。
   */
  void emitStmt(ASTNode* node);

  /**
   * @brief 顺序展开一个程序结点或语句块结点中的所有子语句。
   */
  void emitBlock(ASTNode* node);

  /**
   * @brief 处理变量声明结点及其可选初始化表达式。
   */
  void emitDecl(ASTNode* node);

  /**
   * @brief 处理函数定义结点。
   */
  void emitFunction(ASTNode* node);

  /**
   * @brief 为 `if` 语句生成跳转和分支代码。
   */
  void emitIf(ASTNode* node);

  /**
   * @brief 为 `while` 循环生成回边与退出跳转。
   */
  void emitWhile(ASTNode* node);

  /**
   * @brief 为 `return` 语句生成返回指令。
   */
  void emitReturn(ASTNode* node);

  /**
   * @brief 为赋值语句生成目标和源值之间的传递代码。
   */
  void emitAssignment(ASTNode* node);

  /**
   * @brief 生成一个新的临时变量名。
   */
  std::string newTemp();

  /**
   * @brief 先发出一条目标待回填的跳转语句，并返回其在序列中的位置。
   */
  int emitJump(TriOp op, const std::string& arg1 = "");

  /**
   * @brief 回填此前跳转语句的目标语句号。
   */
  void patchTarget(int stmt_index, int target_stmt_no);

  /**
   * @brief 向中间代码序列末尾追加一条三地址语句。
   */
  void emit(TriOp op,
            const std::string& result = "",
            const std::string& arg1 = "",
            const std::string& arg2 = "");
};

}  // 命名空间 seu_icg
