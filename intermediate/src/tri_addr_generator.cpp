/**
 * @file tri_addr_generator.cpp
 * @brief 实现从 AST 到三地址码的逐步翻译逻辑。
 */

#include "tri_addr_generator.h"

#include <sstream>
#include <stdexcept>
#include <utility>

namespace seu_icg {
namespace {

/**
 * @brief 判断运算符是否属于关系比较运算。
 */
bool isComparisonOperator(const std::string& op) {
  return op == "<" || op == "<=" || op == ">" || op == ">=" || op == "==" || op == "!=";
}

/**
 * @brief 将算术运算符字符串映射为三地址码操作类型。
 */
TriOp arithmeticOpFor(const std::string& op) {
  if (op == "+") {
    return OP_ADD;
  }
  if (op == "-") {
    return OP_SUB;
  }
  if (op == "*") {
    return OP_MUL;
  }
  if (op == "/") {
    return OP_DIV;
  }
  if (op == "%") {
    return OP_MOD;
  }
  throw std::runtime_error("unsupported arithmetic operator: " + op);
}

/**
 * @brief 将实参列表拼成 `a, b, c` 形式的文本。
 */
std::string joinArguments(const std::vector<std::string>& args) {
  std::ostringstream out;
  for (std::size_t index = 0; index < args.size(); ++index) {
    if (index != 0) {
      out << ", ";
    }
    out << args[index];
  }
  return out.str();
}

}  // 匿名命名空间

/**
 * @brief 构造三地址码生成器，并记录外部符号表指针。
 */
TriAddrGenerator::TriAddrGenerator(SymbolTable* symbols) : symbols_(symbols) {}

/**
 * @brief 从 AST 根结点出发生成完整中间代码序列。
 */
IntermediateCode TriAddrGenerator::generate(ASTNode* root) {
  code_ = IntermediateCode();
  tempCounter_ = 0;
  if (symbols_ != nullptr) {
    symbols_->reset();
  }
  // 整个翻译过程对 AST 做单趟遍历，控制流回填被限制在 `emitIf`
  // 与 `emitWhile` 内部，对外只暴露已经完成回填的最终三地址码。
  emitBlock(root);
  return code_;
}

/**
 * @brief 递归翻译表达式，并返回结果所在变量名或字面值。
 */
std::string TriAddrGenerator::emitExpr(ASTNode* node) {
  if (node == nullptr) {
    return "";
  }

  switch (node->type) {
    case NODE_CONSTANT:
      return node->value;
    case NODE_ID:
      return node->value;
    case NODE_FUNC_CALL: {
      std::vector<std::string> args;
      args.reserve(node->children.size());
      for (ASTNode* child : node->children) {
        args.push_back(emitExpr(child));
      }
      const std::string temp = newTemp();
      emit(OP_FUNC_CALL, temp, node->value, joinArguments(args));
      return temp;
    }
    case NODE_ASSIGN:
      emitAssignment(node);
      if (!node->children.empty() && node->children.front() != nullptr) {
        return node->children.front()->value;
      }
      return "";
    case NODE_ARITH: {
      if (isComparisonOperator(node->value)) {
        return emitCondition(node);
      }
      if (node->children.size() == 1) {
        const std::string rhs = emitExpr(node->children[0]);
        const std::string temp = newTemp();
        emit(OP_SUB, temp, "0", rhs);
        return temp;
      }
      if (node->children.size() >= 2) {
        const std::string lhs = emitExpr(node->children[0]);
        const std::string rhs = emitExpr(node->children[1]);
        const std::string temp = newTemp();
        emit(arithmeticOpFor(node->value), temp, lhs, rhs);
        return temp;
      }
      return "";
    }
    default:
      return "";
  }
}

/**
 * @brief 翻译条件表达式，返回可直接用于条件跳转的文本。
 */
std::string TriAddrGenerator::emitCondition(ASTNode* node) {
  if (node == nullptr) {
    return "";
  }
  if (node->type == NODE_ARITH && isComparisonOperator(node->value) &&
      node->children.size() >= 2) {
    const std::string lhs = emitExpr(node->children[0]);
    const std::string rhs = emitExpr(node->children[1]);
    return lhs + " " + node->value + " " + rhs;
  }
  const std::string expr = emitExpr(node);
  if (expr.empty()) {
    return "";
  }
  return expr + " != 0";
}

/**
 * @brief 根据结点类型分发对应的语句翻译逻辑。
 */
void TriAddrGenerator::emitStmt(ASTNode* node) {
  if (node == nullptr) {
    return;
  }

  switch (node->type) {
    case NODE_PROGRAM:
      emitBlock(node);
      return;
    case NODE_FUNC_DEF:
      emitFunction(node);
      return;
    case NODE_VAR_DECL:
      emitDecl(node);
      return;
    case NODE_ASSIGN:
      emitAssignment(node);
      return;
    case NODE_IF:
      emitIf(node);
      return;
    case NODE_WHILE:
      emitWhile(node);
      return;
    case NODE_RETURN:
      emitReturn(node);
      return;
    default:
      emitExpr(node);
      return;
  }
}

/**
 * @brief 翻译一个程序结点或语句块结点。
 */
void TriAddrGenerator::emitBlock(ASTNode* node) {
  if (node == nullptr) {
    return;
  }
  if (node->type != NODE_PROGRAM) {
    emitStmt(node);
    return;
  }
  for (ASTNode* child : node->children) {
    emitStmt(child);
  }
}

/**
 * @brief 翻译变量声明，并在需要时处理初始化表达式。
 */
void TriAddrGenerator::emitDecl(ASTNode* node) {
  if (node == nullptr) {
    return;
  }
  if (symbols_ != nullptr) {
    symbols_->declareVariable(node->value, node->varType.empty() ? "int" : node->varType);
  }
  if (!node->children.empty() && node->children[0] != nullptr) {
    emit(OP_ASSIGN, node->value, emitExpr(node->children[0]));
  }
}

/**
 * @brief 翻译函数定义，并维护函数级作用域。
 */
void TriAddrGenerator::emitFunction(ASTNode* node) {
  if (node == nullptr) {
    return;
  }
  if (symbols_ != nullptr) {
    symbols_->declareFunction(node->value, node->varType.empty() ? "int" : node->varType);
    symbols_->enterScope();
  }

  // 约定前面的子结点依次保存形参，最后一个子结点保存函数体。
  // 这里先注册形参，再翻译函数体，保证局部查表顺序正确。
  for (std::size_t index = 0; index + 1 < node->children.size(); ++index) {
    ASTNode* parameter = node->children[index];
    if (parameter != nullptr && parameter->type == NODE_VAR_DECL && symbols_ != nullptr) {
      symbols_->declareParameter(parameter->value,
                                 parameter->varType.empty() ? "int" : parameter->varType);
    }
  }

  if (!node->children.empty()) {
    emitBlock(node->children.back());
  }

  if (symbols_ != nullptr) {
    symbols_->exitScope();
  }
}

/**
 * @brief 翻译 if/else 语句并完成分支跳转回填。
 */
void TriAddrGenerator::emitIf(ASTNode* node) {
  if (node == nullptr || node->children.size() < 2) {
    return;
  }

  const std::string condition = emitCondition(node->children[0]);
  const int true_jump = emitJump(OP_IF_GOTO, condition);
  const int false_jump = emitJump(OP_GOTO);

  // 当 then/else 分支真正落地后，再把入口语句号回填进去，
  // 这与教材中的回填翻译方案保持一致。
  patchTarget(true_jump, code_.stmtCount + 1);
  emitBlock(node->children[1]);

  if (node->children.size() >= 3 && node->children[2] != nullptr) {
    const int end_jump = emitJump(OP_GOTO);
    patchTarget(false_jump, code_.stmtCount + 1);
    emitBlock(node->children[2]);
    patchTarget(end_jump, code_.stmtCount + 1);
  } else {
    patchTarget(false_jump, code_.stmtCount + 1);
  }
}

/**
 * @brief 翻译 while 循环并补全回边与退出跳转。
 */
void TriAddrGenerator::emitWhile(ASTNode* node) {
  if (node == nullptr || node->children.size() < 2) {
    return;
  }

  const int condition_stmt = code_.stmtCount + 1;
  const std::string condition = emitCondition(node->children[0]);
  const int body_jump = emitJump(OP_IF_GOTO, condition);
  const int exit_jump = emitJump(OP_GOTO);

  // 循环体结束后显式跳回条件入口，形成标准回边。
  patchTarget(body_jump, code_.stmtCount + 1);
  emitBlock(node->children[1]);
  emit(OP_GOTO, std::to_string(condition_stmt));
  patchTarget(exit_jump, code_.stmtCount + 1);
}

/**
 * @brief 翻译 return 语句。
 */
void TriAddrGenerator::emitReturn(ASTNode* node) {
  if (node == nullptr || node->children.empty() || node->children[0] == nullptr) {
    emit(OP_RETURN);
    return;
  }
  emit(OP_RETURN, "", emitExpr(node->children[0]));
}

/**
 * @brief 翻译赋值语句。
 */
void TriAddrGenerator::emitAssignment(ASTNode* node) {
  if (node == nullptr || node->children.size() < 2 || node->children[0] == nullptr) {
    return;
  }
  const std::string lhs = node->children[0]->value;
  const std::string rhs = emitExpr(node->children[1]);
  emit(OP_ASSIGN, lhs, rhs);
}

/**
 * @brief 生成下一个临时变量名。
 */
std::string TriAddrGenerator::newTemp() {
  ++tempCounter_;
  return "t" + std::to_string(tempCounter_);
}

/**
 * @brief 先发出一条目标待定的跳转语句，并返回其下标。
 */
int TriAddrGenerator::emitJump(TriOp op, const std::string& arg1) {
  emit(op, "", arg1);
  return static_cast<int>(code_.stmts.size()) - 1;
}

/**
 * @brief 把跳转语句的目标语句号回填为指定值。
 */
void TriAddrGenerator::patchTarget(int stmt_index, int target_stmt_no) {
  if (stmt_index < 0 || stmt_index >= static_cast<int>(code_.stmts.size())) {
    return;
  }
  code_.stmts[static_cast<std::size_t>(stmt_index)].result = std::to_string(target_stmt_no);
}

/**
 * @brief 向当前代码序列尾部追加一条三地址语句。
 */
void TriAddrGenerator::emit(TriOp op,
                            const std::string& result,
                            const std::string& arg1,
                            const std::string& arg2) {
  code_.addStmt(TriAddrStmt(0, op, arg1, arg2, result));
}

}  // 命名空间 seu_icg
