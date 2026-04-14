#include "intermediate_code.h"

#include <algorithm>
#include <cctype>
#include <ostream>
#include <sstream>
#include <set>
#include <unordered_map>
#include <utility>

namespace seu_icg {
namespace {

bool parseStatementNumber(const std::string& text, int* value) {
  if (value == nullptr || text.empty()) {
    return false;
  }
  for (char ch : text) {
    if (std::isdigit(static_cast<unsigned char>(ch)) == 0) {
      return false;
    }
  }
  try {
    *value = std::stoi(text);
    return *value > 0;
  } catch (...) {
    return false;
  }
}

std::string formatBlockStatements(const IntermediateCode& block) {
  std::ostringstream out;
  out << '[';
  for (std::size_t index = 0; index < block.stmts.size(); ++index) {
    if (index != 0) {
      out << ',';
    }
    out << block.stmts[index].stmtNo;
  }
  out << ']';
  return out.str();
}

std::string describeSuccessors(const std::vector<IntermediateCode>& blocks,
                               const std::unordered_map<int, std::size_t>& leader_to_block,
                               std::size_t block_index) {
  if (block_index >= blocks.size() || blocks[block_index].stmts.empty()) {
    return "none";
  }

  auto blockNameForLeader = [&](int leader) -> std::string {
    const auto found = leader_to_block.find(leader);
    if (found == leader_to_block.end()) {
      return "invalid(" + std::to_string(leader) + ")";
    }
    return "B" + std::to_string(found->second);
  };

  const TriAddrStmt& last = blocks[block_index].stmts.back();
  const bool has_fallthrough = block_index + 1 < blocks.size();
  int target_stmt = 0;
  const bool has_target = parseStatementNumber(last.result, &target_stmt);

  if (last.op == OP_RETURN) {
    return "none";
  }
  if (last.op == OP_GOTO) {
    return has_target ? blockNameForLeader(target_stmt) : "none";
  }
  if (last.op == OP_IF_GOTO) {
    std::string successors = has_target ? blockNameForLeader(target_stmt) : "?";
    if (has_fallthrough) {
      successors += ", " + std::string("B") + std::to_string(block_index + 2);
    }
    return successors;
  }
  if (has_fallthrough) {
    return "B" + std::to_string(block_index + 2);
  }
  return "none";
}

}  // namespace

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

std::vector<IntermediateCode> splitBasicBlocks(const IntermediateCode& code) {
  std::vector<IntermediateCode> blocks;
  if (code.stmts.empty()) {
    return blocks;
  }

  std::unordered_map<int, std::size_t> stmt_index;
  stmt_index.reserve(code.stmts.size());
  for (std::size_t index = 0; index < code.stmts.size(); ++index) {
    stmt_index.emplace(code.stmts[index].stmtNo, index);
  }

  std::set<int> leaders;
  leaders.insert(code.stmts.front().stmtNo);
  for (std::size_t index = 0; index < code.stmts.size(); ++index) {
    const TriAddrStmt& stmt = code.stmts[index];
    if (stmt.op != OP_GOTO && stmt.op != OP_IF_GOTO && stmt.op != OP_RETURN) {
      continue;
    }

    int target_stmt = 0;
    if ((stmt.op == OP_GOTO || stmt.op == OP_IF_GOTO) &&
        parseStatementNumber(stmt.result, &target_stmt) != 0 &&
        stmt_index.find(target_stmt) != stmt_index.end()) {
      leaders.insert(target_stmt);
    }
    if (index + 1 < code.stmts.size()) {
      leaders.insert(code.stmts[index + 1].stmtNo);
    }
  }

  IntermediateCode current_block;
  for (std::size_t index = 0; index < code.stmts.size(); ++index) {
    const TriAddrStmt& stmt = code.stmts[index];
    if (!current_block.stmts.empty() && leaders.find(stmt.stmtNo) != leaders.end()) {
      blocks.push_back(current_block);
      current_block = IntermediateCode();
    }
    current_block.addStmt(stmt);
  }
  if (!current_block.stmts.empty()) {
    blocks.push_back(current_block);
  }
  return blocks;
}

std::string formatBasicBlocks(const std::vector<IntermediateCode>& blocks) {
  std::unordered_map<int, std::size_t> leader_to_block;
  leader_to_block.reserve(blocks.size());
  for (std::size_t index = 0; index < blocks.size(); ++index) {
    if (!blocks[index].stmts.empty()) {
      leader_to_block.emplace(blocks[index].stmts.front().stmtNo, index + 1);
    }
  }

  std::ostringstream out;
  for (std::size_t index = 0; index < blocks.size(); ++index) {
    const IntermediateCode& block = blocks[index];
    if (block.stmts.empty()) {
      continue;
    }
    if (out.tellp() > 0) {
      out << "\n\n";
    }
    out << "B" << (index + 1) << " [leader=" << block.stmts.front().stmtNo << ", stmts="
        << formatBlockStatements(block) << ", successors="
        << describeSuccessors(blocks, leader_to_block, index) << "]";
    for (const TriAddrStmt& stmt : block.stmts) {
      out << '\n' << formatTriAddrStmt(stmt);
    }
  }
  return out.str();
}

void dumpIntermediateCode(const IntermediateCode& code, std::ostream& out) {
  out << formatIntermediateCode(code);
}

void dumpBasicBlocks(const std::vector<IntermediateCode>& blocks, std::ostream& out) {
  out << formatBasicBlocks(blocks);
}

}  // namespace seu_icg
