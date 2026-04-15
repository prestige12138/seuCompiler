/**
 * @file main.cpp
 * @brief Self-test and utility entry point for the intermediate-code module.
 */

#include "ast_builder.h"
#include "intermediate_code.h"
#include "symbol_table.h"
#include "target_ir_emitter.h"
#include "tri_addr_generator.h"

#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using seu_icg::ASTBuilder;
using seu_icg::ASTNode;
using seu_icg::IntermediateCode;
using seu_icg::SymbolTable;
using seu_icg::TriAddrGenerator;

bool expectEqual(const std::string& name,
                 const std::string& actual,
                 const std::string& expected) {
  if (actual == expected) {
    std::cout << "[self-test] " << name << ": ok\n";
    return true;
  }
  std::cerr << "[self-test] " << name << ": failed\n";
  std::cerr << "expected:\n" << expected << "\nactual:\n" << actual << '\n';
  return false;
}

bool expectContainsAll(const std::string& name,
                       const std::string& actual,
                       const std::vector<std::string>& fragments) {
  for (const std::string& fragment : fragments) {
    if (actual.find(fragment) == std::string::npos) {
      std::cerr << "[self-test] " << name << ": failed\n";
      std::cerr << "missing fragment: " << fragment << "\nactual:\n" << actual << '\n';
      return false;
    }
  }
  std::cout << "[self-test] " << name << ": ok\n";
  return true;
}

bool runAstConstructionTest(const ASTBuilder& builder) {
  ASTNode* root = builder.makeAssignment(
      builder.makeIdentifier("a", "int"),
      builder.makeBinary(
          seu_icg::NODE_ARITH,
          builder.makeIdentifier("b", "int"),
          builder.makeBinary(seu_icg::NODE_ARITH,
                             builder.makeConstant("3", "int"),
                             builder.makeIdentifier("c", "int"),
                             "*",
                             "int"),
          "+",
          "int"));

  const bool ok = root != nullptr && root->type == seu_icg::NODE_ASSIGN &&
                  root->children.size() == 2 && root->children[0]->value == "a" &&
                  root->children[1]->type == seu_icg::NODE_ARITH &&
                  root->children[1]->value == "+" &&
                  root->children[1]->children.size() == 2 &&
                  root->children[1]->children[1]->type == seu_icg::NODE_ARITH &&
                  root->children[1]->children[1]->value == "*" &&
                  root->children[1]->children[1]->children.size() == 2 &&
                  root->children[1]->children[1]->children[0]->value == "3" &&
                  root->children[1]->children[1]->children[1]->value == "c";
  if (ok) {
    std::cout << "[self-test] ast_construction_basic: ok\n";
  } else {
    std::cerr << "[self-test] ast_construction_basic: failed\n";
  }
  builder.destroyTree(root);
  return ok;
}

bool runParseRootTest(const ASTBuilder& builder) {
  ASTNode* root = builder.makeProgram({});
  seu_icg::setParseRoot(root);
  ASTNode* observed = seu_icg::getParseRoot();
  ASTNode* released = seu_icg::releaseParseRoot();
  const bool ok = observed == root && released == root && seu_icg::getParseRoot() == nullptr;
  if (ok) {
    std::cout << "[self-test] parse_root_handoff: ok\n";
  } else {
    std::cerr << "[self-test] parse_root_handoff: failed\n";
  }
  builder.destroyTree(released);
  return ok;
}

bool runSymbolTableTest() {
  SymbolTable table;
  table.reset();
  const bool global_ok = table.declareVariable("g", "int");
  table.enterScope();
  const bool param_ok = table.declareParameter("a", "int");
  const bool local_ok = table.declareVariable("x", "double");
  const bool duplicate_rejected = !table.declareVariable("x", "int");
  const auto* inner_x = table.lookup("x");
  const auto* inner_g = table.lookup("g");
  const int inner_x_scope = inner_x == nullptr ? -1 : inner_x->scope_level;
  const int inner_g_scope = inner_g == nullptr ? -1 : inner_g->scope_level;
  table.exitScope();
  const auto* missing_x = table.lookup("x");
  const bool ok = global_ok && param_ok && local_ok && duplicate_rejected &&
                  inner_x != nullptr && inner_x_scope == 1 &&
                  inner_g != nullptr && inner_g_scope == 0 && missing_x == nullptr;
  if (ok) {
    std::cout << "[self-test] symbol_table_scope: ok\n";
  } else {
    std::cerr << "[self-test] symbol_table_scope: failed\n";
    std::cerr << "global_ok=" << global_ok << " param_ok=" << param_ok
              << " local_ok=" << local_ok
              << " duplicate_rejected=" << duplicate_rejected
              << " inner_x_scope=" << inner_x_scope
              << " inner_g_scope=" << inner_g_scope
              << " missing_x_null=" << (missing_x == nullptr) << '\n';
  }
  return ok;
}

bool runArithmeticAssignmentTest(const ASTBuilder& builder) {
  SymbolTable symbols;
  TriAddrGenerator generator(&symbols);

  ASTNode* root = builder.makeProgram({
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

  const IntermediateCode code = generator.generate(root);
  const bool ok = expectEqual("arithmetic_assignment_ir",
                              seu_icg::formatIntermediateCode(code),
                              "1: t1 = 2 * 3\n2: t2 = 1 + t1\n3: x = t2");
  builder.destroyTree(root);
  return ok;
}

bool runControlFlowAndCallTest(const ASTBuilder& builder) {
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
  const bool ok = expectEqual(
      "control_flow_and_call_ir",
      seu_icg::formatIntermediateCode(code),
      "1: if x > 0 goto 3\n"
      "2: goto 6\n"
      "3: t1 = call foo(x, 1)\n"
      "4: x = t1\n"
      "5: goto 7\n"
      "6: x = 0\n"
      "7: if x < 10 goto 9\n"
      "8: goto 12\n"
      "9: t2 = x + 1\n"
      "10: x = t2\n"
      "11: goto 7\n"
      "12: return x");
  builder.destroyTree(root);
  return ok;
}

bool runFunctionBodyTest(const ASTBuilder& builder) {
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
  const bool ok =
      expectEqual("function_body_ir",
                  seu_icg::formatIntermediateCode(code),
                  "1: t1 = a + b\n2: c = t1\n3: return c");
  builder.destroyTree(root);
  return ok;
}

bool runBasicBlockTest() {
  IntermediateCode code;
  code.addStmt(seu_icg::TriAddrStmt(1, seu_icg::OP_IF_GOTO, "x > 0", "3"));
  code.addStmt(seu_icg::TriAddrStmt(2, seu_icg::OP_GOTO, "", "6"));
  code.addStmt(seu_icg::TriAddrStmt(3, seu_icg::OP_FUNC_CALL, "foo", "x, 1", "t1"));
  code.addStmt(seu_icg::TriAddrStmt(4, seu_icg::OP_ASSIGN, "t1", "", "x"));
  code.addStmt(seu_icg::TriAddrStmt(5, seu_icg::OP_GOTO, "", "7"));
  code.addStmt(seu_icg::TriAddrStmt(6, seu_icg::OP_ASSIGN, "0", "", "x"));
  code.addStmt(seu_icg::TriAddrStmt(7, seu_icg::OP_IF_GOTO, "x < 10", "9"));
  code.addStmt(seu_icg::TriAddrStmt(8, seu_icg::OP_GOTO, "", "12"));
  code.addStmt(seu_icg::TriAddrStmt(9, seu_icg::OP_ADD, "x", "1", "t2"));
  code.addStmt(seu_icg::TriAddrStmt(10, seu_icg::OP_ASSIGN, "t2", "", "x"));
  code.addStmt(seu_icg::TriAddrStmt(11, seu_icg::OP_GOTO, "", "7"));
  code.addStmt(seu_icg::TriAddrStmt(12, seu_icg::OP_RETURN, "x", ""));

  const std::vector<IntermediateCode> blocks = seu_icg::splitBasicBlocks(code);
  const bool ok = expectEqual(
      "basic_block_partition",
      seu_icg::formatBasicBlocks(blocks),
      "B1 [leader=1, stmts=[1], successors=B3, B2]\n"
      "1: if x > 0 goto 3\n\n"
      "B2 [leader=2, stmts=[2], successors=B4]\n"
      "2: goto 6\n\n"
      "B3 [leader=3, stmts=[3,4,5], successors=B5]\n"
      "3: t1 = call foo(x, 1)\n"
      "4: x = t1\n"
      "5: goto 7\n\n"
      "B4 [leader=6, stmts=[6], successors=B5]\n"
      "6: x = 0\n\n"
      "B5 [leader=7, stmts=[7], successors=B7, B6]\n"
      "7: if x < 10 goto 9\n\n"
      "B6 [leader=8, stmts=[8], successors=B8]\n"
      "8: goto 12\n\n"
      "B7 [leader=9, stmts=[9,10,11], successors=B5]\n"
      "9: t2 = x + 1\n"
      "10: x = t2\n"
      "11: goto 7\n\n"
      "B8 [leader=12, stmts=[12], successors=none]\n"
      "12: return x");
  return ok;
}

bool runLlvmEmitterTest(const ASTBuilder& builder) {
  ASTNode* function = builder.makeFunction(
      "main",
      "int",
      {},
      builder.makeProgram({
          builder.makeVarDecl("a", "int"),
          builder.makeVarDecl("b", "int"),
          builder.makeReturn(builder.makeIdentifier("a", "int")),
      }));

  IntermediateCode code;
  code.addStmt(seu_icg::TriAddrStmt(1, seu_icg::OP_ASSIGN, "1", "", "a"));
  code.addStmt(seu_icg::TriAddrStmt(2, seu_icg::OP_ASSIGN, "2", "", "b"));
  code.addStmt(seu_icg::TriAddrStmt(3, seu_icg::OP_ADD, "a", "b", "t1"));
  code.addStmt(seu_icg::TriAddrStmt(4, seu_icg::OP_RETURN, "t1", ""));

  seu_icg::TargetIrOptions options;
  options.emitComments = false;
  const bool ok = expectContainsAll(
      "llvm_ir_emitter",
      seu_icg::formatLlvmIr(function, code, options),
      {"define i32 @main()", "alloca i32", "add nsw i32", "ret i32"});
  builder.destroyTree(function);
  return ok;
}

bool runJimpleEmitterTest(const ASTBuilder& builder) {
  ASTNode* function = builder.makeFunction(
      "main",
      "int",
      {},
      builder.makeProgram({
          builder.makeVarDecl("x", "int"),
          builder.makeReturn(builder.makeIdentifier("x", "int")),
      }));

  IntermediateCode code;
  code.addStmt(seu_icg::TriAddrStmt(1, seu_icg::OP_FUNC_CALL, "foo", "x, 1", "t1"));
  code.addStmt(seu_icg::TriAddrStmt(2, seu_icg::OP_ASSIGN, "t1", "", "x"));
  code.addStmt(seu_icg::TriAddrStmt(3, seu_icg::OP_RETURN, "x", ""));

  seu_icg::TargetIrOptions options;
  options.emitComments = true;
  options.className = "SeuDemo";
  const bool ok = expectContainsAll(
      "jimple_ir_emitter",
      seu_icg::formatJimple(function, code, options),
      {".class public final SeuDemo", ".method public static int main()",
       "staticinvoke SeuDemo.foo(x, 1);", "return x;"});
  builder.destroyTree(function);
  return ok;
}

bool runSelfTests() {
  const ASTBuilder builder;
  return runAstConstructionTest(builder) && runParseRootTest(builder) &&
         runSymbolTableTest() &&
         runArithmeticAssignmentTest(builder) &&
         runControlFlowAndCallTest(builder) && runFunctionBodyTest(builder) &&
         runBasicBlockTest() && runLlvmEmitterTest(builder) &&
         runJimpleEmitterTest(builder);
}

void printUsage(const char* program) {
  std::cout << "Usage: " << program << " --self-test\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc == 2 && std::string(argv[1]) == "--self-test") {
      return runSelfTests() ? 0 : 1;
    }
    printUsage(argv[0]);
    return 0;
  } catch (const std::exception& ex) {
    std::cerr << "seuIntermediate error: " << ex.what() << '\n';
    return 1;
  }
}
