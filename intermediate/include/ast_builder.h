#pragma once

#include <string>
#include <vector>

#include "intermediate_code.h"

/**
 * @file ast_builder.h
 * @brief AST construction helpers that can be called from generated Yacc
 *        semantic actions.
 */

namespace seu_icg {

/**
 * @brief Stateless helper for constructing report-defined AST nodes.
 */
class ASTBuilder {
 public:
  /**
   * @brief Allocate one generic AST node.
   *
   * Complexity: O(|value| + |var_type|).
   */
  ASTNode* makeNode(ASTNodeType type,
                    const std::string& value = "",
                    const std::string& var_type = "") const;

  /**
   * @brief Allocate one identifier leaf node.
   *
   * Complexity: O(|name| + |var_type|).
   */
  ASTNode* makeIdentifier(const std::string& name,
                          const std::string& var_type = "") const;

  /**
   * @brief Allocate one constant leaf node.
   *
   * Complexity: O(|value| + |var_type|).
   */
  ASTNode* makeConstant(const std::string& value,
                        const std::string& var_type = "") const;

  /**
   * @brief Allocate one variable-declaration node.
   *
   * When `initializer` is non-null, it becomes the only child.
   * Complexity: O(|name| + |var_type|).
   */
  ASTNode* makeVarDecl(const std::string& name,
                       const std::string& var_type,
                       ASTNode* initializer = nullptr) const;

  /**
   * @brief Allocate one assignment node.
   *
   * `lhs` and `rhs` become children 0 and 1.
   * Complexity: O(1).
   */
  ASTNode* makeAssignment(ASTNode* lhs, ASTNode* rhs) const;

  /**
   * @brief Allocate one unary node.
   *
   * Complexity: O(|op| + |var_type|).
   */
  ASTNode* makeUnary(ASTNodeType type,
                     ASTNode* child,
                     const std::string& op,
                     const std::string& var_type = "") const;

  /**
   * @brief Allocate one binary node.
   *
   * Complexity: O(|op| + |var_type|).
   */
  ASTNode* makeBinary(ASTNodeType type,
                      ASTNode* lhs,
                      ASTNode* rhs,
                      const std::string& op,
                      const std::string& var_type = "") const;

  /**
   * @brief Allocate one ternary node.
   *
   * Complexity: O(|value| + |var_type|).
   */
  ASTNode* makeTernary(ASTNodeType type,
                       ASTNode* first,
                       ASTNode* second,
                       ASTNode* third,
                       const std::string& value = "",
                       const std::string& var_type = "") const;

  /**
   * @brief Allocate one function-call node.
   *
   * `args` are appended in order as children.
   * Complexity: O(|function_name| + |return_type| + args.size()).
   */
  ASTNode* makeCall(const std::string& function_name,
                    const std::vector<ASTNode*>& args,
                    const std::string& return_type = "") const;

  /**
   * @brief Allocate one function-definition node.
   *
   * Parameter declarations are appended first, then `body`.
   * Complexity: O(|function_name| + |return_type| + parameters.size()).
   */
  ASTNode* makeFunction(const std::string& function_name,
                        const std::string& return_type,
                        const std::vector<ASTNode*>& parameters,
                        ASTNode* body) const;

  /**
   * @brief Allocate one `if` node.
   *
   * Children are condition, then-branch, and optional else-branch.
   * Complexity: O(1).
   */
  ASTNode* makeIf(ASTNode* condition,
                  ASTNode* then_branch,
                  ASTNode* else_branch = nullptr) const;

  /**
   * @brief Allocate one `while` node.
   *
   * Children are condition and body.
   * Complexity: O(1).
   */
  ASTNode* makeWhile(ASTNode* condition, ASTNode* body) const;

  /**
   * @brief Allocate one return node.
   *
   * When `value` is non-null, it becomes the only child.
   * Complexity: O(1).
   */
  ASTNode* makeReturn(ASTNode* value = nullptr) const;

  /**
   * @brief Allocate one program/list node.
   *
   * This is used both as the program root and as a generic statement-list
   * container because the report's enum does not define a dedicated block node.
   * Complexity: O(nodes.size()).
   */
  ASTNode* makeProgram(const std::vector<ASTNode*>& nodes) const;

  /**
   * @brief Append one child to an existing node.
   *
   * Complexity: amortized O(1).
   */
  void appendChild(ASTNode* parent, ASTNode* child) const;

  /**
   * @brief Overwrite the semantic type associated with a node.
   *
   * Complexity: O(|var_type|).
   */
  void setNodeType(ASTNode* node, const std::string& var_type) const;

  /**
   * @brief Recursively destroy an AST tree.
   *
   * Complexity: O(N), where N is the node count.
   */
  void destroyTree(ASTNode* root) const;
};

/**
 * @brief Export the current parse root for downstream IR generation.
 *
 * Complexity: O(1).
 */
void setParseRoot(ASTNode* root);

/**
 * @brief Return the current parse root without transferring ownership.
 *
 * Complexity: O(1).
 */
ASTNode* getParseRoot();

/**
 * @brief Return the current parse root and clear the global slot.
 *
 * Complexity: O(1).
 */
ASTNode* releaseParseRoot();

}  // namespace seu_icg
