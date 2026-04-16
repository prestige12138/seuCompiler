/**
 * @file ast_builder.cpp
 * @brief 实现 AST 构造辅助函数以及语法树根结点交接工具。
 */

#include "ast_builder.h"

#include <utility>

namespace seu_icg {
namespace {

ASTNode* g_parse_root = nullptr;

}  // 匿名命名空间

/**
 * @brief 创建任意类型的基础 AST 结点。
 */
ASTNode* ASTBuilder::makeNode(ASTNodeType type,
                              const std::string& value,
                              const std::string& var_type) const {
  // 所有专用构造函数最终都走这里，确保测试代码和语义动作生成出的结点
  // 在字段布局上完全一致。
  ASTNode* node = new ASTNode(type, value);
  node->varType = var_type;
  return node;
}

/**
 * @brief 创建标识符结点。
 */
ASTNode* ASTBuilder::makeIdentifier(const std::string& name,
                                    const std::string& var_type) const {
  return makeNode(NODE_ID, name, var_type);
}

/**
 * @brief 创建常量结点。
 */
ASTNode* ASTBuilder::makeConstant(const std::string& value,
                                  const std::string& var_type) const {
  return makeNode(NODE_CONSTANT, value, var_type);
}

/**
 * @brief 创建变量声明结点，并在需要时挂接初始化表达式。
 */
ASTNode* ASTBuilder::makeVarDecl(const std::string& name,
                                 const std::string& var_type,
                                 ASTNode* initializer) const {
  ASTNode* node = makeNode(NODE_VAR_DECL, name, var_type);
  if (initializer != nullptr) {
    node->children.push_back(initializer);
  }
  return node;
}

/**
 * @brief 创建赋值结点，并依次连接左右子树。
 */
ASTNode* ASTBuilder::makeAssignment(ASTNode* lhs, ASTNode* rhs) const {
  ASTNode* node = makeNode(NODE_ASSIGN, "=");
  appendChild(node, lhs);
  appendChild(node, rhs);
  return node;
}

/**
 * @brief 创建二元运算结点。
 */
ASTNode* ASTBuilder::makeBinary(ASTNodeType type,
                                ASTNode* lhs,
                                ASTNode* rhs,
                                const std::string& op,
                                const std::string& var_type) const {
  ASTNode* node = makeNode(type, op, var_type);
  appendChild(node, lhs);
  appendChild(node, rhs);
  return node;
}

/**
 * @brief 创建函数调用结点，并按顺序追加实参结点。
 */
ASTNode* ASTBuilder::makeCall(const std::string& function_name,
                              const std::vector<ASTNode*>& args,
                              const std::string& return_type) const {
  ASTNode* node = makeNode(NODE_FUNC_CALL, function_name, return_type);
  for (ASTNode* arg : args) {
    appendChild(node, arg);
  }
  return node;
}

/**
 * @brief 创建函数定义结点，并依次挂接形参与函数体。
 */
ASTNode* ASTBuilder::makeFunction(const std::string& function_name,
                                  const std::string& return_type,
                                  const std::vector<ASTNode*>& parameters,
                                  ASTNode* body) const {
  ASTNode* node = makeNode(NODE_FUNC_DEF, function_name, return_type);
  for (ASTNode* parameter : parameters) {
    appendChild(node, parameter);
  }
  appendChild(node, body);
  return node;
}

/**
 * @brief 创建条件分支结点。
 */
ASTNode* ASTBuilder::makeIf(ASTNode* condition,
                            ASTNode* then_branch,
                            ASTNode* else_branch) const {
  ASTNode* node = makeNode(NODE_IF, "if");
  appendChild(node, condition);
  appendChild(node, then_branch);
  appendChild(node, else_branch);
  return node;
}

/**
 * @brief 创建 while 循环结点。
 */
ASTNode* ASTBuilder::makeWhile(ASTNode* condition, ASTNode* body) const {
  ASTNode* node = makeNode(NODE_WHILE, "while");
  appendChild(node, condition);
  appendChild(node, body);
  return node;
}

/**
 * @brief 创建 return 结点。
 */
ASTNode* ASTBuilder::makeReturn(ASTNode* value) const {
  ASTNode* node = makeNode(NODE_RETURN, "return");
  appendChild(node, value);
  return node;
}

/**
 * @brief 创建程序根结点或语句列表结点。
 */
ASTNode* ASTBuilder::makeProgram(const std::vector<ASTNode*>& nodes) const {
  ASTNode* node = makeNode(NODE_PROGRAM, "program");
  for (ASTNode* child : nodes) {
    appendChild(node, child);
  }
  return node;
}

/**
 * @brief 将一个子结点追加到父结点末尾。
 */
void ASTBuilder::appendChild(ASTNode* parent, ASTNode* child) const {
  if (parent == nullptr || child == nullptr) {
    return;
  }
  parent->children.push_back(child);
}

/**
 * @brief 修改结点记录的语义类型字符串。
 */
void ASTBuilder::setNodeType(ASTNode* node, const std::string& var_type) const {
  if (node != nullptr) {
    node->varType = var_type;
  }
}

/**
 * @brief 以后序遍历方式递归释放整棵 AST。
 */
void ASTBuilder::destroyTree(ASTNode* root) const {
  if (root == nullptr) {
    return;
  }
  // 当前子集的所有权关系保持树形结构，因此后序删除既足够安全，
  // 又能继续兼容 Yacc 语义值里使用的裸指针。
  for (ASTNode* child : root->children) {
    destroyTree(child);
  }
  delete root;
}

/**
 * @brief 保存当前解析得到的 AST 根结点。
 */
void setParseRoot(ASTNode* root) {
  g_parse_root = root;
}

/**
 * @brief 返回当前缓存的 AST 根结点，但不清空缓存。
 */
ASTNode* getParseRoot() {
  return g_parse_root;
}

/**
 * @brief 取出当前缓存的 AST 根结点，并清空全局缓存。
 */
ASTNode* releaseParseRoot() {
  ASTNode* root = g_parse_root;
  g_parse_root = nullptr;
  return root;
}

}  // 命名空间 seu_icg
