#include "ast_builder.h"

#include <utility>

namespace seu_icg {
namespace {

ASTNode* g_parse_root = nullptr;

}  // namespace

ASTNode* ASTBuilder::makeNode(ASTNodeType type,
                              const std::string& value,
                              const std::string& var_type) const {
  ASTNode* node = new ASTNode(type, value);
  node->varType = var_type;
  return node;
}

ASTNode* ASTBuilder::makeIdentifier(const std::string& name,
                                    const std::string& var_type) const {
  return makeNode(NODE_ID, name, var_type);
}

ASTNode* ASTBuilder::makeConstant(const std::string& value,
                                  const std::string& var_type) const {
  return makeNode(NODE_CONSTANT, value, var_type);
}

ASTNode* ASTBuilder::makeVarDecl(const std::string& name,
                                 const std::string& var_type,
                                 ASTNode* initializer) const {
  ASTNode* node = makeNode(NODE_VAR_DECL, name, var_type);
  if (initializer != nullptr) {
    node->children.push_back(initializer);
  }
  return node;
}

ASTNode* ASTBuilder::makeAssignment(ASTNode* lhs, ASTNode* rhs) const {
  ASTNode* node = makeNode(NODE_ASSIGN, "=");
  appendChild(node, lhs);
  appendChild(node, rhs);
  return node;
}

ASTNode* ASTBuilder::makeUnary(ASTNodeType type,
                               ASTNode* child,
                               const std::string& op,
                               const std::string& var_type) const {
  ASTNode* node = makeNode(type, op, var_type);
  appendChild(node, child);
  return node;
}

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

ASTNode* ASTBuilder::makeTernary(ASTNodeType type,
                                 ASTNode* first,
                                 ASTNode* second,
                                 ASTNode* third,
                                 const std::string& value,
                                 const std::string& var_type) const {
  ASTNode* node = makeNode(type, value, var_type);
  appendChild(node, first);
  appendChild(node, second);
  appendChild(node, third);
  return node;
}

ASTNode* ASTBuilder::makeCall(const std::string& function_name,
                              const std::vector<ASTNode*>& args,
                              const std::string& return_type) const {
  ASTNode* node = makeNode(NODE_FUNC_CALL, function_name, return_type);
  for (ASTNode* arg : args) {
    appendChild(node, arg);
  }
  return node;
}

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

ASTNode* ASTBuilder::makeIf(ASTNode* condition,
                            ASTNode* then_branch,
                            ASTNode* else_branch) const {
  ASTNode* node = makeNode(NODE_IF, "if");
  appendChild(node, condition);
  appendChild(node, then_branch);
  appendChild(node, else_branch);
  return node;
}

ASTNode* ASTBuilder::makeWhile(ASTNode* condition, ASTNode* body) const {
  ASTNode* node = makeNode(NODE_WHILE, "while");
  appendChild(node, condition);
  appendChild(node, body);
  return node;
}

ASTNode* ASTBuilder::makeReturn(ASTNode* value) const {
  ASTNode* node = makeNode(NODE_RETURN, "return");
  appendChild(node, value);
  return node;
}

ASTNode* ASTBuilder::makeProgram(const std::vector<ASTNode*>& nodes) const {
  ASTNode* node = makeNode(NODE_PROGRAM, "program");
  for (ASTNode* child : nodes) {
    appendChild(node, child);
  }
  return node;
}

void ASTBuilder::appendChild(ASTNode* parent, ASTNode* child) const {
  if (parent == nullptr || child == nullptr) {
    return;
  }
  parent->children.push_back(child);
}

void ASTBuilder::setNodeType(ASTNode* node, const std::string& var_type) const {
  if (node != nullptr) {
    node->varType = var_type;
  }
}

void ASTBuilder::destroyTree(ASTNode* root) const {
  if (root == nullptr) {
    return;
  }
  for (ASTNode* child : root->children) {
    destroyTree(child);
  }
  delete root;
}

void setParseRoot(ASTNode* root) {
  g_parse_root = root;
}

ASTNode* getParseRoot() {
  return g_parse_root;
}

ASTNode* releaseParseRoot() {
  ASTNode* root = g_parse_root;
  g_parse_root = nullptr;
  return root;
}

}  // namespace seu_icg
