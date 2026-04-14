#include "intermediate_code.h"

#include <ostream>
#include <sstream>
#include <utility>

namespace seu_icg {

ASTNode::ASTNode(ASTNodeType t, std::string val)
    : type(t), value(std::move(val)) {}

TriAddrStmt::TriAddrStmt(int no, TriOp o, std::string a1, std::string a2, std::string res)
    : stmtNo(no),
      op(o),
      arg1(std::move(a1)),
      arg2(std::move(a2)),
      result(std::move(res)) {}

TriAddrStmt::TriAddrStmt(int no, TriOp o, std::string a1, std::string res)
    : stmtNo(no),
      op(o),
      arg1(std::move(a1)),
      arg2(""),
      result(std::move(res)) {}

IntermediateCode::IntermediateCode() : stmtCount(0) {}

void IntermediateCode::addStmt(TriAddrStmt stmt) {
  if (stmt.stmtNo <= 0) {
    stmt.stmtNo = stmtCount + 1;
  }
  if (stmt.stmtNo > stmtCount) {
    stmtCount = stmt.stmtNo;
  }
  stmts.push_back(std::move(stmt));
}

const char* astNodeTypeName(ASTNodeType type) {
  switch (type) {
    case NODE_PROGRAM:
      return "NODE_PROGRAM";
    case NODE_FUNC_DEF:
      return "NODE_FUNC_DEF";
    case NODE_VAR_DECL:
      return "NODE_VAR_DECL";
    case NODE_ASSIGN:
      return "NODE_ASSIGN";
    case NODE_ARITH:
      return "NODE_ARITH";
    case NODE_FUNC_CALL:
      return "NODE_FUNC_CALL";
    case NODE_IF:
      return "NODE_IF";
    case NODE_WHILE:
      return "NODE_WHILE";
    case NODE_RETURN:
      return "NODE_RETURN";
    case NODE_ID:
      return "NODE_ID";
    case NODE_CONSTANT:
      return "NODE_CONSTANT";
  }
  return "NODE_UNKNOWN";
}

const char* triOpName(TriOp op) {
  switch (op) {
    case OP_ADD:
      return "OP_ADD";
    case OP_SUB:
      return "OP_SUB";
    case OP_MUL:
      return "OP_MUL";
    case OP_DIV:
      return "OP_DIV";
    case OP_MOD:
      return "OP_MOD";
    case OP_ASSIGN:
      return "OP_ASSIGN";
    case OP_FUNC_CALL:
      return "OP_FUNC_CALL";
    case OP_GOTO:
      return "OP_GOTO";
    case OP_IF_GOTO:
      return "OP_IF_GOTO";
    case OP_RETURN:
      return "OP_RETURN";
  }
  return "OP_UNKNOWN";
}

std::string formatTriAddrStmt(const TriAddrStmt& stmt) {
  std::ostringstream out;
  out << stmt.stmtNo << ": ";
  switch (stmt.op) {
    case OP_ADD:
      out << stmt.result << " = " << stmt.arg1 << " + " << stmt.arg2;
      break;
    case OP_SUB:
      out << stmt.result << " = " << stmt.arg1 << " - " << stmt.arg2;
      break;
    case OP_MUL:
      out << stmt.result << " = " << stmt.arg1 << " * " << stmt.arg2;
      break;
    case OP_DIV:
      out << stmt.result << " = " << stmt.arg1 << " / " << stmt.arg2;
      break;
    case OP_MOD:
      out << stmt.result << " = " << stmt.arg1 << " % " << stmt.arg2;
      break;
    case OP_ASSIGN:
      out << stmt.result << " = " << stmt.arg1;
      break;
    case OP_FUNC_CALL:
      out << stmt.result << " = call " << stmt.arg1;
      if (!stmt.arg2.empty()) {
        out << "(" << stmt.arg2 << ")";
      }
      break;
    case OP_GOTO:
      out << "goto " << stmt.result;
      break;
    case OP_IF_GOTO:
      out << "if " << stmt.arg1 << " goto " << stmt.result;
      break;
    case OP_RETURN:
      if (stmt.arg1.empty()) {
        out << "return";
      } else {
        out << "return " << stmt.arg1;
      }
      break;
  }
  return out.str();
}

std::string formatIntermediateCode(const IntermediateCode& code) {
  std::ostringstream out;
  for (std::size_t index = 0; index < code.stmts.size(); ++index) {
    if (index != 0) {
      out << '\n';
    }
    out << formatTriAddrStmt(code.stmts[index]);
  }
  return out.str();
}

void dumpIntermediateCode(const IntermediateCode& code, std::ostream& out) {
  out << formatIntermediateCode(code);
}

}  // namespace seu_icg
