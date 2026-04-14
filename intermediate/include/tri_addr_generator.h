#pragma once

#include <string>

#include "intermediate_code.h"
#include "symbol_table.h"

/**
 * @file tri_addr_generator.h
 * @brief Three-address-code generation from the report-defined AST.
 */

namespace seu_icg {

/**
 * @brief Generate three-address code from one AST.
 */
class TriAddrGenerator {
 public:
  /**
   * @brief Construct one generator bound to an optional symbol table.
   *
   * Complexity: O(1).
   */
  explicit TriAddrGenerator(SymbolTable* symbols = nullptr);

  /**
   * @brief Generate intermediate code for one AST root.
   *
   * Complexity: O(N), where N is the AST node count.
   */
  IntermediateCode generate(ASTNode* root);

 private:
  IntermediateCode code_;
  SymbolTable* symbols_ = nullptr;
  int tempCounter_ = 0;

  std::string emitExpr(ASTNode* node);
  std::string emitCondition(ASTNode* node);
  void emitStmt(ASTNode* node);
  void emitBlock(ASTNode* node);
  void emitDecl(ASTNode* node);
  void emitFunction(ASTNode* node);
  void emitIf(ASTNode* node);
  void emitWhile(ASTNode* node);
  void emitReturn(ASTNode* node);
  void emitAssignment(ASTNode* node);
  std::string newTemp();
  int emitJump(TriOp op, const std::string& arg1 = "");
  void patchTarget(int stmt_index, int target_stmt_no);
  void emit(TriOp op,
            const std::string& result = "",
            const std::string& arg1 = "",
            const std::string& arg2 = "");
};

}  // namespace seu_icg
