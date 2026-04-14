#pragma once

#include <iosfwd>
#include <string>
#include <vector>

/**
 * @file intermediate_code.h
 * @brief Report-defined AST and three-address-code data structures for
 *        intermediate-code generation.
 */

namespace seu_icg {

/**
 * @brief Report-defined AST node kinds.
 *
 * Complexity: O(1) to copy or compare.
 */
enum ASTNodeType {
  NODE_PROGRAM,
  NODE_FUNC_DEF,
  NODE_VAR_DECL,
  NODE_ASSIGN,
  NODE_ARITH,
  NODE_FUNC_CALL,
  NODE_IF,
  NODE_WHILE,
  NODE_RETURN,
  NODE_ID,
  NODE_CONSTANT
};

/**
 * @brief Report-defined AST node.
 *
 * `children` uses raw pointers to remain compatible with the report's layout
 * and with Yacc semantic values such as `%union { void* node; }`.
 */
typedef struct ASTNode {
  ASTNodeType type;
  std::string value;
  std::string varType;
  std::vector<ASTNode*> children;

  /**
   * @brief Construct one AST node with an optional payload value.
   *
   * Complexity: O(1).
   */
  ASTNode(ASTNodeType t, std::string val);
} ASTNode;

/**
 * @brief Report-defined three-address operation kinds.
 *
 * Complexity: O(1) to copy or compare.
 */
enum TriOp {
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_MOD,
  OP_ASSIGN,
  OP_FUNC_CALL,
  OP_GOTO,
  OP_IF_GOTO,
  OP_RETURN
};

/**
 * @brief Report-defined three-address statement.
 */
typedef struct TriAddrStmt {
  int stmtNo;
  TriOp op;
  std::string arg1;
  std::string arg2;
  std::string result;

  /**
   * @brief Construct a full three-address statement.
   *
   * Complexity: O(|a1| + |a2| + |res|).
   */
  TriAddrStmt(int no, TriOp o, std::string a1, std::string a2, std::string res);

  /**
   * @brief Construct a compact three-address statement.
   *
   * Complexity: O(|a1| + |res|).
   */
  TriAddrStmt(int no, TriOp o, std::string a1, std::string res);
} TriAddrStmt;

/**
 * @brief Report-defined intermediate-code container.
 */
typedef struct IntermediateCode {
  std::vector<TriAddrStmt> stmts;
  int stmtCount;

  /**
   * @brief Construct an empty intermediate-code container.
   *
   * Complexity: O(1).
   */
  IntermediateCode();

  /**
   * @brief Append one statement and auto-assign its number when needed.
   *
   * Complexity: amortized O(1).
   */
  void addStmt(TriAddrStmt stmt);
} IntermediateCode;

/**
 * @brief Return a stable display name for one AST node kind.
 *
 * Complexity: O(1).
 */
const char* astNodeTypeName(ASTNodeType type);

/**
 * @brief Return a stable display name for one three-address operator.
 *
 * Complexity: O(1).
 */
const char* triOpName(TriOp op);

/**
 * @brief Format one three-address statement in standard textual form.
 *
 * Complexity: O(|arg1| + |arg2| + |result|).
 */
std::string formatTriAddrStmt(const TriAddrStmt& stmt);

/**
 * @brief Format a whole three-address-code sequence.
 *
 * Complexity: O(N + total_text), where N is the statement count.
 */
std::string formatIntermediateCode(const IntermediateCode& code);

/**
 * @brief Split one three-address-code sequence into basic blocks.
 *
 * Leaders follow the standard rule set: the first statement, jump targets, and
 * statements that immediately follow jumps.
 *
 * Complexity: O(N log N), where N is the statement count.
 */
std::vector<IntermediateCode> splitBasicBlocks(const IntermediateCode& code);

/**
 * @brief Format a basic-block partition in stable text form.
 *
 * Complexity: O(B + N + total_text), where B is the block count and N is the
 * statement count.
 */
std::string formatBasicBlocks(const std::vector<IntermediateCode>& blocks);

/**
 * @brief Dump a whole three-address-code sequence to one output stream.
 *
 * Complexity: O(N + total_text).
 */
void dumpIntermediateCode(const IntermediateCode& code, std::ostream& out);

/**
 * @brief Dump a basic-block partition to one output stream.
 *
 * Complexity: O(B + N + total_text).
 */
void dumpBasicBlocks(const std::vector<IntermediateCode>& blocks, std::ostream& out);

}  // namespace seu_icg
