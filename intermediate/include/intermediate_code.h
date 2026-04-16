#pragma once

#include <iosfwd>
#include <string>
#include <vector>

/**
 * @file intermediate_code.h
 * @brief 定义中期报告要求的 AST、三地址语句以及中间代码容器结构。
 */

namespace seu_icg {

/**
 * @brief 中期报告中定义的 AST 结点类型枚举。
 *
 * 时间复杂度：拷贝和比较均为 O(1)。
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
 * @brief 中期报告中定义的 AST 结点结构。
 *
 * `children` 继续使用裸指针，以保持与报告中的数据布局以及
 * `%union { void* node; }` 这类 Yacc 语义值兼容。
 */
typedef struct ASTNode {
  ASTNodeType type;
  std::string value;
  std::string varType;
  std::vector<ASTNode*> children;

  /**
   * @brief 构造一个 AST 结点，并可附带初始值字符串。
   *
   * 时间复杂度：O(1)。
   */
  ASTNode(ASTNodeType t, std::string val);
} ASTNode;

/**
 * @brief 中期报告中定义的三地址操作类型枚举。
 *
 * 时间复杂度：拷贝和比较均为 O(1)。
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
 * @brief 中期报告中定义的一条三地址语句。
 */
typedef struct TriAddrStmt {
  int stmtNo;
  TriOp op;
  std::string arg1;
  std::string arg2;
  std::string result;

  /**
   * @brief 构造一条包含两个参数的完整三地址语句。
   *
   * 时间复杂度：O(|a1| + |a2| + |res|)。
   */
  TriAddrStmt(int no, TriOp o, std::string a1, std::string a2, std::string res);

  /**
   * @brief 构造一条简化形式的三地址语句。
   *
   * 时间复杂度：O(|a1| + |res|)。
   */
  TriAddrStmt(int no, TriOp o, std::string a1, std::string res);
} TriAddrStmt;

/**
 * @brief 中期报告中定义的中间代码序列容器。
 */
typedef struct IntermediateCode {
  std::vector<TriAddrStmt> stmts;
  int stmtCount;

  /**
   * @brief 构造一个空的中间代码容器。
   *
   * 时间复杂度：O(1)。
   */
  IntermediateCode();

  /**
   * @brief 追加一条语句；若编号未设置，则自动补齐语句号。
   *
   * 时间复杂度：均摊 O(1)。
   */
  void addStmt(TriAddrStmt stmt);
} IntermediateCode;

/**
 * @brief 返回 AST 结点类型对应的稳定展示名称。
 *
 * 时间复杂度：O(1)。
 */
const char* astNodeTypeName(ASTNodeType type);

/**
 * @brief 返回三地址操作类型对应的稳定展示名称。
 *
 * 时间复杂度：O(1)。
 */
const char* triOpName(TriOp op);

/**
 * @brief 将一条三地址语句格式化为标准文本。
 *
 * 时间复杂度：O(|arg1| + |arg2| + |result|)。
 */
std::string formatTriAddrStmt(const TriAddrStmt& stmt);

/**
 * @brief 将整段三地址代码序列格式化为文本。
 *
 * 时间复杂度：O(N + total_text)，其中 N 为语句数。
 */
std::string formatIntermediateCode(const IntermediateCode& code);

/**
 * @brief 将一段三地址代码切分为基本块序列。
 *
 * 入口语句遵循标准规则：第一条语句、跳转目标语句以及紧随跳转语句后的语句。
 *
 * 时间复杂度：O(N log N)，其中 N 为语句数。
 */
std::vector<IntermediateCode> splitBasicBlocks(const IntermediateCode& code);

/**
 * @brief 将基本块划分结果格式化为稳定文本。
 *
 * 时间复杂度：O(B + N + total_text)，其中 B 为基本块数量，N 为语句数。
 */
std::string formatBasicBlocks(const std::vector<IntermediateCode>& blocks);

/**
 * @brief 将整段三地址代码输出到给定流。
 *
 * 时间复杂度：O(N + total_text)。
 */
void dumpIntermediateCode(const IntermediateCode& code, std::ostream& out);

/**
 * @brief 将基本块划分结果输出到给定流。
 *
 * 时间复杂度：O(B + N + total_text)。
 */
void dumpBasicBlocks(const std::vector<IntermediateCode>& blocks, std::ostream& out);

}  // 命名空间 seu_icg
