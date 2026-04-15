/**
 * @file tri_addr_generator.cpp
 * @brief AST-to-three-address-code lowering for the current minic-plus subset.
 */

#include "tri_addr_generator.h"

#include <sstream>
#include <stdexcept>
#include <utility>

namespace seu_icg {
namespace {

bool isComparisonOperator(const std::string& op) {
  return op == "<" || op == "<=" || op == ">" || op == ">=" || op == "==" || op == "!=";
}

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

}  // namespace

TriAddrGenerator::TriAddrGenerator(SymbolTable* symbols) : symbols_(symbols) {}

IntermediateCode TriAddrGenerator::generate(ASTNode* root) {
  code_ = IntermediateCode();
  tempCounter_ = 0;
  if (symbols_ != nullptr) {
    symbols_->reset();
  }
  // Generation is single-pass over the AST. Control-flow backpatching is kept
  // local to `emitIf` / `emitWhile`, so callers only see finished TAC.
  emitBlock(root);
  return code_;
}

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

void TriAddrGenerator::emitFunction(ASTNode* node) {
  if (node == nullptr) {
    return;
  }
  if (symbols_ != nullptr) {
    symbols_->declareFunction(node->value, node->varType.empty() ? "int" : node->varType);
    symbols_->enterScope();
  }

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

void TriAddrGenerator::emitIf(ASTNode* node) {
  if (node == nullptr || node->children.size() < 2) {
    return;
  }

  const std::string condition = emitCondition(node->children[0]);
  const int true_jump = emitJump(OP_IF_GOTO, condition);
  const int false_jump = emitJump(OP_GOTO);

  // Backpatch the "then" and optional "else" entry once statement numbers are
  // known, mirroring the textbook translation scheme.
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

void TriAddrGenerator::emitWhile(ASTNode* node) {
  if (node == nullptr || node->children.size() < 2) {
    return;
  }

  const int condition_stmt = code_.stmtCount + 1;
  const std::string condition = emitCondition(node->children[0]);
  const int body_jump = emitJump(OP_IF_GOTO, condition);
  const int exit_jump = emitJump(OP_GOTO);

  // The loop closes with an explicit backward goto to the condition leader.
  patchTarget(body_jump, code_.stmtCount + 1);
  emitBlock(node->children[1]);
  emit(OP_GOTO, std::to_string(condition_stmt));
  patchTarget(exit_jump, code_.stmtCount + 1);
}

void TriAddrGenerator::emitReturn(ASTNode* node) {
  if (node == nullptr || node->children.empty() || node->children[0] == nullptr) {
    emit(OP_RETURN);
    return;
  }
  emit(OP_RETURN, "", emitExpr(node->children[0]));
}

void TriAddrGenerator::emitAssignment(ASTNode* node) {
  if (node == nullptr || node->children.size() < 2 || node->children[0] == nullptr) {
    return;
  }
  const std::string lhs = node->children[0]->value;
  const std::string rhs = emitExpr(node->children[1]);
  emit(OP_ASSIGN, lhs, rhs);
}

std::string TriAddrGenerator::newTemp() {
  ++tempCounter_;
  return "t" + std::to_string(tempCounter_);
}

int TriAddrGenerator::emitJump(TriOp op, const std::string& arg1) {
  emit(op, "", arg1);
  return static_cast<int>(code_.stmts.size()) - 1;
}

void TriAddrGenerator::patchTarget(int stmt_index, int target_stmt_no) {
  if (stmt_index < 0 || stmt_index >= static_cast<int>(code_.stmts.size())) {
    return;
  }
  code_.stmts[static_cast<std::size_t>(stmt_index)].result = std::to_string(target_stmt_no);
}

void TriAddrGenerator::emit(TriOp op,
                            const std::string& result,
                            const std::string& arg1,
                            const std::string& arg2) {
  code_.addStmt(TriAddrStmt(0, op, arg1, arg2, result));
}

}  // namespace seu_icg
