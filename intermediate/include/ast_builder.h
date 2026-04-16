#pragma once

#include <string>
#include <vector>

#include "intermediate_code.h"

/**
 * @file ast_builder.h
 * @brief 定义 AST 构造辅助接口，供生成出的 Yacc 语义动作直接调用。
 */

namespace seu_icg {

/**
 * @brief 无状态 AST 构造器，负责按报告定义创建各类语法树结点。
 */
class ASTBuilder {
 public:
  /**
   * @brief 创建一个通用 AST 结点。
   *
   * 时间复杂度：O(|value| + |var_type|)。
   */
  ASTNode* makeNode(ASTNodeType type,
                    const std::string& value = "",
                    const std::string& var_type = "") const;

  /**
   * @brief 创建一个标识符叶子结点。
   *
   * 时间复杂度：O(|name| + |var_type|)。
   */
  ASTNode* makeIdentifier(const std::string& name,
                          const std::string& var_type = "") const;

  /**
   * @brief 创建一个常量叶子结点。
   *
   * 时间复杂度：O(|value| + |var_type|)。
   */
  ASTNode* makeConstant(const std::string& value,
                        const std::string& var_type = "") const;

  /**
   * @brief 创建一个变量声明结点。
   *
   * 当 `initializer` 非空时，会作为唯一子结点挂到声明结点上。
   * 时间复杂度：O(|name| + |var_type|)。
   */
  ASTNode* makeVarDecl(const std::string& name,
                       const std::string& var_type,
                       ASTNode* initializer = nullptr) const;

  /**
   * @brief 创建一个赋值结点。
   *
   * `lhs` 和 `rhs` 会依次成为第 0、1 个子结点。
   * 时间复杂度：O(1)。
   */
  ASTNode* makeAssignment(ASTNode* lhs, ASTNode* rhs) const;

  /**
   * @brief 创建一个二元运算结点。
   *
   * 时间复杂度：O(|op| + |var_type|)。
   */
  ASTNode* makeBinary(ASTNodeType type,
                      ASTNode* lhs,
                      ASTNode* rhs,
                      const std::string& op,
                      const std::string& var_type = "") const;

  /**
   * @brief 创建一个函数调用结点。
   *
   * `args` 会按顺序附加为子结点。
   * 时间复杂度：O(|function_name| + |return_type| + args.size())。
   */
  ASTNode* makeCall(const std::string& function_name,
                    const std::vector<ASTNode*>& args,
                    const std::string& return_type = "") const;

  /**
   * @brief 创建一个函数定义结点。
   *
   * 形参声明会先加入子结点列表，随后再追加函数体 `body`。
   * 时间复杂度：O(|function_name| + |return_type| + parameters.size())。
   */
  ASTNode* makeFunction(const std::string& function_name,
                        const std::string& return_type,
                        const std::vector<ASTNode*>& parameters,
                        ASTNode* body) const;

  /**
   * @brief 创建一个 `if` 结点。
   *
   * 子结点依次为条件、then 分支以及可选的 else 分支。
   * 时间复杂度：O(1)。
   */
  ASTNode* makeIf(ASTNode* condition,
                  ASTNode* then_branch,
                  ASTNode* else_branch = nullptr) const;

  /**
   * @brief 创建一个 `while` 结点。
   *
   * 子结点依次为循环条件和循环体。
   * 时间复杂度：O(1)。
   */
  ASTNode* makeWhile(ASTNode* condition, ASTNode* body) const;

  /**
   * @brief 创建一个 `return` 结点。
   *
   * 当 `value` 非空时，它会成为唯一子结点。
   * 时间复杂度：O(1)。
   */
  ASTNode* makeReturn(ASTNode* value = nullptr) const;

  /**
   * @brief 创建一个程序根结点或语句列表结点。
   *
   * 由于报告中的枚举没有单独定义 block 结点，这里同时承担程序根结点
   * 和通用语句列表容器两种角色。
   * 时间复杂度：O(nodes.size())。
   */
  ASTNode* makeProgram(const std::vector<ASTNode*>& nodes) const;

  /**
   * @brief 向已有结点追加一个子结点。
   *
   * 时间复杂度：均摊 O(1)。
   */
  void appendChild(ASTNode* parent, ASTNode* child) const;

  /**
   * @brief 覆盖结点上记录的语义类型。
   *
   * 时间复杂度：O(|var_type|)。
   */
  void setNodeType(ASTNode* node, const std::string& var_type) const;

  /**
   * @brief 递归释放一棵 AST。
   *
   * 时间复杂度：O(N)，其中 N 为结点总数。
   */
  void destroyTree(ASTNode* root) const;
};

/**
 * @brief 保存当前语法分析阶段产出的根结点，供后续 IR 阶段使用。
 *
 * 时间复杂度：O(1)。
 */
void setParseRoot(ASTNode* root);

/**
 * @brief 返回当前保存的语法树根结点，但不转移所有权。
 *
 * 时间复杂度：O(1)。
 */
ASTNode* getParseRoot();

/**
 * @brief 取出当前语法树根结点，并清空全局保存槽位。
 *
 * 时间复杂度：O(1)。
 */
ASTNode* releaseParseRoot();

}  // 命名空间 seu_icg
