#include <cstdlib>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "ast_builder.h"
#include "intermediate_code.h"
#include "symbol_table.h"
#include "tri_addr_generator.h"

namespace {

using seu_icg::ASTBuilder;
using seu_icg::ASTNode;
using seu_icg::IntermediateCode;
using seu_icg::SymbolTable;
using seu_icg::TriAddrGenerator;

std::string yesNo(bool value) {
  return value ? "yes" : "no";
}

std::string quoteOrEmpty(const std::string& text) {
  if (text.empty()) {
    return "<empty>";
  }

  std::ostringstream out;
  out << '"';
  for (char ch : text) {
    switch (ch) {
      case '\\':
        out << "\\\\";
        break;
      case '"':
        out << "\\\"";
        break;
      case '\n':
        out << "\\n";
        break;
      case '\t':
        out << "\\t";
        break;
      default:
        out << ch;
        break;
    }
  }
  out << '"';
  return out.str();
}

void serializeAst(const ASTNode* node, int depth, std::ostream& out) {
  out << std::string(static_cast<std::size_t>(depth) * 2U, ' ')
      << seu_icg::astNodeTypeName(node->type) << "[value=" << quoteOrEmpty(node->value)
      << ",type=" << quoteOrEmpty(node->varType)
      << ",children=" << node->children.size() << "]\n";
  for (const ASTNode* child : node->children) {
    if (child != nullptr) {
      serializeAst(child, depth + 1, out);
    }
  }
}

std::string normalizeMultiline(const std::string& text) {
  std::ostringstream out;
  bool previous_was_newline = false;
  for (char ch : text) {
    if (ch == '\n') {
      if (!previous_was_newline) {
        out << " | ";
      }
      previous_was_newline = true;
      continue;
    }
    previous_was_newline = false;
    out << ch;
  }

  std::string normalized = out.str();
  if (normalized.size() >= 3 && normalized.substr(normalized.size() - 3) == " | ") {
    normalized.resize(normalized.size() - 3);
  }
  return normalized;
}

ASTNode* makeArithmeticAssignmentProgram(const ASTBuilder& builder) {
  return builder.makeProgram({
      builder.makeVarDecl("x", "int"),
      builder.makeAssignment(
          builder.makeIdentifier("x", "int"),
          builder.makeBinary(
              seu_icg::NODE_ARITH,
              builder.makeConstant("1", "int"),
              builder.makeBinary(seu_icg::NODE_ARITH,
                                 builder.makeConstant("2", "int"),
                                 builder.makeConstant("3", "int"),
                                 "*",
                                 "int"),
              "+",
              "int")),
  });
}

void commandAstShape() {
  const ASTBuilder builder;
  ASTNode* root = builder.makeProgram({
      builder.makeVarDecl("counter", "int", builder.makeConstant("0", "int")),
      builder.makeAssignment(builder.makeIdentifier("counter", "int"),
                             builder.makeBinary(seu_icg::NODE_ARITH,
                                                builder.makeIdentifier("counter", "int"),
                                                builder.makeConstant("1", "int"),
                                                "+",
                                                "int")),
  });
  builder.setNodeType(root->children[1], "int");
  serializeAst(root, 0, std::cout);
  builder.destroyTree(root);
}

void commandParseRoot() {
  const ASTBuilder builder;
  ASTNode* root = builder.makeProgram({builder.makeReturn(builder.makeConstant("0", "int"))});
  seu_icg::setParseRoot(root);
  ASTNode* observed = seu_icg::getParseRoot();
  ASTNode* released = seu_icg::releaseParseRoot();
  ASTNode* released_again = seu_icg::releaseParseRoot();

  std::cout << "observed_same=" << yesNo(observed == root) << '\n'
            << "released_same=" << yesNo(released == root) << '\n'
            << "after_release_null=" << yesNo(seu_icg::getParseRoot() == nullptr) << '\n'
            << "second_release_null=" << yesNo(released_again == nullptr) << '\n';

  builder.destroyTree(released);
}

void commandSymbolScope() {
  SymbolTable symbols;
  symbols.reset();
  const bool declare_global_x = symbols.declareVariable("x", "int");
  const bool declare_global_g = symbols.declareVariable("g", "double");

  symbols.enterScope();
  const bool declare_inner_x = symbols.declareVariable("x", "char");
  const bool declare_inner_y = symbols.declareVariable("y", "int");
  const bool duplicate_inner_y_rejected = !symbols.declareVariable("y", "short");

  const seu_icg::SymbolEntry* inner_x = symbols.lookup("x");
  const seu_icg::SymbolEntry* global_g = symbols.lookup("g");
  const std::string inner_x_type = inner_x == nullptr ? "<missing>" : inner_x->type;
  const int inner_x_scope = inner_x == nullptr ? -1 : inner_x->scope_level;
  const int global_g_scope = global_g == nullptr ? -1 : global_g->scope_level;

  symbols.exitScope();
  const seu_icg::SymbolEntry* after_exit_x = symbols.lookup("x");
  const seu_icg::SymbolEntry* after_exit_y = symbols.lookup("y");

  std::cout << "declare_global_x=" << yesNo(declare_global_x) << '\n'
            << "declare_global_g=" << yesNo(declare_global_g) << '\n'
            << "declare_inner_x=" << yesNo(declare_inner_x) << '\n'
            << "declare_inner_y=" << yesNo(declare_inner_y) << '\n'
            << "duplicate_inner_y_rejected=" << yesNo(duplicate_inner_y_rejected) << '\n'
            << "inner_x_type=" << inner_x_type << '\n'
            << "inner_x_scope=" << inner_x_scope << '\n'
            << "global_g_scope=" << global_g_scope << '\n'
            << "after_exit_x_type="
            << (after_exit_x == nullptr ? "<missing>" : after_exit_x->type) << '\n'
            << "after_exit_x_scope=" << (after_exit_x == nullptr ? -1 : after_exit_x->scope_level)
            << '\n'
            << "after_exit_y_missing=" << yesNo(after_exit_y == nullptr) << '\n';
}

void commandSymbolOffsets() {
  SymbolTable symbols;
  symbols.reset();
  symbols.declareVariable("g", "int");
  symbols.declareVariable("flag", "char");
  symbols.enterScope();
  symbols.declareParameter("lhs", "double");
  symbols.declareParameter("rhs", "int");
  symbols.declareVariable("tmp", "short");
  symbols.declareVariable("acc", "double");

  const seu_icg::SymbolEntry* global_g = symbols.lookup("g");
  const seu_icg::SymbolEntry* global_flag = symbols.lookup("flag");
  const seu_icg::SymbolEntry* param_lhs = symbols.lookup("lhs");
  const seu_icg::SymbolEntry* param_rhs = symbols.lookup("rhs");
  const seu_icg::SymbolEntry* local_tmp = symbols.lookup("tmp");
  const seu_icg::SymbolEntry* local_acc = symbols.lookup("acc");

  std::cout << "global_g.offset=" << (global_g == nullptr ? -1 : global_g->offset) << '\n'
            << "global_flag.offset=" << (global_flag == nullptr ? -1 : global_flag->offset)
            << '\n'
            << "param_lhs.offset=" << (param_lhs == nullptr ? -1 : param_lhs->offset) << '\n'
            << "param_rhs.offset=" << (param_rhs == nullptr ? -1 : param_rhs->offset) << '\n'
            << "local_tmp.offset=" << (local_tmp == nullptr ? -1 : local_tmp->offset) << '\n'
            << "local_acc.offset=" << (local_acc == nullptr ? -1 : local_acc->offset) << '\n'
            << "scope_level=" << symbols.currentScopeLevel() << '\n';
}

void commandFormatters() {
  seu_icg::TriAddrStmt call_stmt(7, seu_icg::OP_FUNC_CALL, "foo", "x, 1", "t3");
  IntermediateCode code;
  code.addStmt(seu_icg::TriAddrStmt(1, seu_icg::OP_ASSIGN, "1", "", "x"));
  code.addStmt(seu_icg::TriAddrStmt(2, seu_icg::OP_RETURN, "x", ""));

  std::ostringstream dumped;
  seu_icg::dumpIntermediateCode(code, dumped);

  std::cout << "ast_type=" << seu_icg::astNodeTypeName(seu_icg::NODE_WHILE) << '\n'
            << "tri_op=" << seu_icg::triOpName(seu_icg::OP_FUNC_CALL) << '\n'
            << "stmt=" << seu_icg::formatTriAddrStmt(call_stmt) << '\n'
            << "dump=" << normalizeMultiline(dumped.str()) << '\n';
}

void commandIrConstantAssign() {
  const ASTBuilder builder;
  SymbolTable symbols;
  TriAddrGenerator generator(&symbols);

  ASTNode* root = builder.makeProgram({
      builder.makeVarDecl("x", "int", builder.makeConstant("42", "int")),
      builder.makeReturn(builder.makeIdentifier("x", "int")),
  });

  const IntermediateCode code = generator.generate(root);
  std::cout << seu_icg::formatIntermediateCode(code) << '\n';
  builder.destroyTree(root);
}

void commandIrArithmetic() {
  const ASTBuilder builder;
  SymbolTable symbols;
  TriAddrGenerator generator(&symbols);

  ASTNode* root = makeArithmeticAssignmentProgram(builder);
  const IntermediateCode code = generator.generate(root);
  std::cout << seu_icg::formatIntermediateCode(code) << '\n';
  builder.destroyTree(root);
}

void commandIrControlFlow() {
  const ASTBuilder builder;
  SymbolTable symbols;
  TriAddrGenerator generator(&symbols);

  ASTNode* root = builder.makeProgram({
      builder.makeVarDecl("x", "int"),
      builder.makeIf(
          builder.makeBinary(seu_icg::NODE_ARITH,
                             builder.makeIdentifier("x", "int"),
                             builder.makeConstant("0", "int"),
                             ">",
                             "int"),
          builder.makeAssignment(
              builder.makeIdentifier("x", "int"),
              builder.makeCall("foo",
                               {builder.makeIdentifier("x", "int"),
                                builder.makeConstant("1", "int")},
                               "int")),
          builder.makeAssignment(builder.makeIdentifier("x", "int"),
                                 builder.makeConstant("0", "int"))),
      builder.makeWhile(
          builder.makeBinary(seu_icg::NODE_ARITH,
                             builder.makeIdentifier("x", "int"),
                             builder.makeConstant("10", "int"),
                             "<",
                             "int"),
          builder.makeProgram({
              builder.makeAssignment(
                  builder.makeIdentifier("x", "int"),
                  builder.makeBinary(seu_icg::NODE_ARITH,
                                     builder.makeIdentifier("x", "int"),
                                     builder.makeConstant("1", "int"),
                                     "+",
                                     "int")),
          })),
      builder.makeReturn(builder.makeIdentifier("x", "int")),
  });

  const IntermediateCode code = generator.generate(root);
  std::cout << seu_icg::formatIntermediateCode(code) << '\n';
  builder.destroyTree(root);
}

void commandIrFunction() {
  const ASTBuilder builder;
  SymbolTable symbols;
  TriAddrGenerator generator(&symbols);

  ASTNode* function = builder.makeFunction(
      "add",
      "int",
      {builder.makeVarDecl("a", "int"), builder.makeVarDecl("b", "int")},
      builder.makeProgram({
          builder.makeVarDecl("c", "int"),
          builder.makeAssignment(
              builder.makeIdentifier("c", "int"),
              builder.makeBinary(seu_icg::NODE_ARITH,
                                 builder.makeIdentifier("a", "int"),
                                 builder.makeIdentifier("b", "int"),
                                 "+",
                                 "int")),
          builder.makeReturn(builder.makeIdentifier("c", "int")),
      }));

  ASTNode* root = builder.makeProgram({function});
  const IntermediateCode code = generator.generate(root);
  std::cout << seu_icg::formatIntermediateCode(code) << '\n'
            << "function_declared=" << yesNo(symbols.lookup("add") != nullptr) << '\n'
            << "local_c_visible_after_exit=" << yesNo(symbols.lookup("c") != nullptr) << '\n'
            << "param_a_visible_after_exit=" << yesNo(symbols.lookup("a") != nullptr) << '\n';
  builder.destroyTree(root);
}

void commandIrUnsupportedOp() {
  const ASTBuilder builder;
  SymbolTable symbols;
  TriAddrGenerator generator(&symbols);

  ASTNode* root = builder.makeProgram({
      builder.makeAssignment(builder.makeIdentifier("x", "int"),
                             builder.makeBinary(seu_icg::NODE_ARITH,
                                                builder.makeConstant("2", "int"),
                                                builder.makeConstant("3", "int"),
                                                "^",
                                                "int")),
  });

  try {
    static_cast<void>(generator.generate(root));
    std::cout << "error=<none>\n";
  } catch (const std::exception& ex) {
    std::cout << "error=" << ex.what() << '\n';
  }

  builder.destroyTree(root);
}

void commandGenerateEmpty() {
  TriAddrGenerator generator;
  const IntermediateCode null_code = generator.generate(nullptr);

  const ASTBuilder builder;
  ASTNode* empty_program = builder.makeProgram({});
  const IntermediateCode empty_program_code = generator.generate(empty_program);

  std::cout << "null_stmt_count=" << null_code.stmtCount << '\n'
            << "null_text_empty=" << yesNo(seu_icg::formatIntermediateCode(null_code).empty())
            << '\n'
            << "empty_program_stmt_count=" << empty_program_code.stmtCount << '\n'
            << "empty_program_text_empty="
            << yesNo(seu_icg::formatIntermediateCode(empty_program_code).empty()) << '\n';

  builder.destroyTree(empty_program);
}

void commandPerfBatch(const std::string& iterations_text) {
  const int iterations = std::stoi(iterations_text);
  if (iterations <= 0) {
    throw std::runtime_error("iterations must be positive");
  }

  const ASTBuilder builder;
  SymbolTable symbols;
  TriAddrGenerator generator(&symbols);

  int total_stmt_count = 0;
  int final_stmt_count = 0;
  for (int index = 0; index < iterations; ++index) {
    ASTNode* root = makeArithmeticAssignmentProgram(builder);
    const IntermediateCode code = generator.generate(root);
    total_stmt_count += code.stmtCount;
    final_stmt_count = code.stmtCount;
    builder.destroyTree(root);
  }

  std::cout << "iterations=" << iterations << '\n'
            << "final_stmt_count=" << final_stmt_count << '\n'
            << "total_stmt_count=" << total_stmt_count << '\n'
            << "status=ok\n";
}

void printUsage(const char* program) {
  std::cerr
      << "Usage: " << program
      << " <ast-shape|parse-root|symbol-scope|symbol-offsets|formatters|"
         "ir-constant-assign|ir-arithmetic|ir-control-flow|ir-function|"
         "ir-unsupported-op|generate-empty|perf-batch ITERATIONS>\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc < 2) {
      printUsage(argv[0]);
      return 1;
    }

    const std::string command = argv[1];
    if (command == "ast-shape") {
      commandAstShape();
      return 0;
    }
    if (command == "parse-root") {
      commandParseRoot();
      return 0;
    }
    if (command == "symbol-scope") {
      commandSymbolScope();
      return 0;
    }
    if (command == "symbol-offsets") {
      commandSymbolOffsets();
      return 0;
    }
    if (command == "formatters") {
      commandFormatters();
      return 0;
    }
    if (command == "ir-constant-assign") {
      commandIrConstantAssign();
      return 0;
    }
    if (command == "ir-arithmetic") {
      commandIrArithmetic();
      return 0;
    }
    if (command == "ir-control-flow") {
      commandIrControlFlow();
      return 0;
    }
    if (command == "ir-function") {
      commandIrFunction();
      return 0;
    }
    if (command == "ir-unsupported-op") {
      commandIrUnsupportedOp();
      return 0;
    }
    if (command == "generate-empty") {
      commandGenerateEmpty();
      return 0;
    }
    if (command == "perf-batch") {
      if (argc != 3) {
        throw std::runtime_error("perf-batch requires one iteration count");
      }
      commandPerfBatch(argv[2]);
      return 0;
    }

    throw std::runtime_error("unknown command: " + command);
  } catch (const std::exception& ex) {
    std::cerr << "icg_probe error: " << ex.what() << '\n';
    return 1;
  }
}
