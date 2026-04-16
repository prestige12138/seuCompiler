/**
 * @file target_ir_emitter.cpp
 * @brief 实现从三地址码到 LLVM IR 与 Jimple 的文本级降低。
 */

#include "target_ir_emitter.h"

#include <algorithm>
#include <cctype>
#include <ostream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "tri_addr_generator.h"

namespace seu_icg {
namespace {

/**
 * @brief 表示一个函数级目标 IR 输出单元。
 */
struct FunctionUnit {
  std::string name;
  std::string return_type;
  std::vector<std::string> parameters;
  std::vector<std::string> locals;
  IntermediateCode code;
};

/**
 * @brief 保存解析后的条件表达式三元组。
 */
struct ParsedCondition {
  std::string lhs;
  std::string op;
  std::string rhs;
  bool valid = false;
};

/**
 * @brief 去除字符串首尾空白。
 */
std::string trim(const std::string& text) {
  std::size_t start = 0;
  while (start < text.size() &&
         std::isspace(static_cast<unsigned char>(text[start])) != 0) {
    ++start;
  }

  std::size_t end = text.size();
  while (end > start &&
         std::isspace(static_cast<unsigned char>(text[end - 1])) != 0) {
    --end;
  }
  return text.substr(start, end - start);
}

/**
 * @brief 清理注释文本中的换行，避免生成输出格式被破坏。
 */
std::string sanitizeCommentText(const std::string& text) {
  std::string sanitized;
  sanitized.reserve(text.size());
  for (char ch : text) {
    if (ch == '\n' || ch == '\r') {
      sanitized.push_back(' ');
      continue;
    }
    sanitized.push_back(ch);
  }
  return sanitized;
}

/**
 * @brief 判断一个字符串是否是整数常量。
 */
bool isIntegerLiteral(const std::string& text) {
  if (text.empty()) {
    return false;
  }
  std::size_t index = 0;
  if (text[0] == '-') {
    if (text.size() == 1) {
      return false;
    }
    index = 1;
  }
  for (; index < text.size(); ++index) {
    if (std::isdigit(static_cast<unsigned char>(text[index])) == 0) {
      return false;
    }
  }
  return true;
}

/**
 * @brief 将文本解析为语句编号。
 */
bool parseStatementNumber(const std::string& text, int* value) {
  if (value == nullptr || !isIntegerLiteral(text)) {
    return false;
  }
  try {
    *value = std::stoi(text);
    return *value > 0;
  } catch (...) {
    return false;
  }
}

/**
 * @brief 判断一个字符串是否是合法标识符。
 */
bool isIdentifier(const std::string& text) {
  if (text.empty()) {
    return false;
  }
  if (std::isalpha(static_cast<unsigned char>(text[0])) == 0 && text[0] != '_') {
    return false;
  }
  for (std::size_t index = 1; index < text.size(); ++index) {
    if (std::isalnum(static_cast<unsigned char>(text[index])) == 0 &&
        text[index] != '_') {
      return false;
    }
  }
  return true;
}

/**
 * @brief 断言并返回一个合法标识符，否则抛出上下文相关异常。
 */
std::string requireIdentifier(const std::string& text, const char* context) {
  if (!isIdentifier(text)) {
    throw std::runtime_error(std::string("invalid ") + context + ": " + text);
  }
  return text;
}

/**
 * @brief 将操作数规范化为可直接输出到目标 IR 的文本。
 */
std::string renderOperand(const std::string& value, const char* context) {
  if (isIntegerLiteral(value)) {
    return value;
  }
  return requireIdentifier(value, context);
}

/**
 * @brief 将任意名字清洗为适合目标 IR 使用的标识符。
 */
std::string sanitizeName(const std::string& name) {
  if (name.empty()) {
    return "tmp";
  }
  std::string sanitized;
  sanitized.reserve(name.size() + 1U);
  if (std::isdigit(static_cast<unsigned char>(name[0])) != 0) {
    sanitized.push_back('_');
  }
  for (char ch : name) {
    if (std::isalnum(static_cast<unsigned char>(ch)) != 0 || ch == '_') {
      sanitized.push_back(ch);
    } else {
      sanitized.push_back('_');
    }
  }
  return sanitized;
}

/**
 * @brief 判断某个 AST 结点是否是函数定义。
 */
bool isFunctionNode(const ASTNode* node) {
  return node != nullptr && node->type == NODE_FUNC_DEF;
}

/**
 * @brief 将内部类型名归一化为当前输出阶段支持的简化类型集合。
 */
std::string normalizedType(const std::string& type_name) {
  if (type_name == "void") {
    return "void";
  }
  return "int";
}

/**
 * @brief 若值尚未出现，则将其追加到顺序容器中。
 */
void appendUnique(std::vector<std::string>* values, const std::string& value) {
  if (values == nullptr || value.empty()) {
    return;
  }
  if (std::find(values->begin(), values->end(), value) == values->end()) {
    values->push_back(value);
  }
}

/**
 * @brief 递归收集 AST 中的变量声明名字。
 */
void collectVarDecls(const ASTNode* node, std::vector<std::string>* locals) {
  if (node == nullptr || locals == nullptr) {
    return;
  }
  if (node->type == NODE_VAR_DECL) {
    appendUnique(locals, node->value);
  }
  for (const ASTNode* child : node->children) {
    collectVarDecls(child, locals);
  }
}

/**
 * @brief 将逗号分隔的实参串切分为参数列表。
 */
std::vector<std::string> splitArguments(const std::string& text) {
  std::vector<std::string> args;
  std::string current;
  std::istringstream input(text);
  while (std::getline(input, current, ',')) {
    current = trim(current);
    if (!current.empty()) {
      args.push_back(current);
    }
  }
  return args;
}

/**
 * @brief 解析条件文本，拆出左右操作数和比较运算符。
 */
ParsedCondition parseCondition(const std::string& text) {
  ParsedCondition parsed;
  const std::string normalized = trim(text);
  static const char* kOperators[] = {"<=", ">=", "==", "!=", "<", ">"};
  for (const char* op : kOperators) {
    const std::string needle(op);
    const std::size_t pos = normalized.find(needle);
    if (pos == std::string::npos) {
      continue;
    }
    parsed.lhs = trim(normalized.substr(0, pos));
    parsed.op = needle;
    parsed.rhs = trim(normalized.substr(pos + needle.size()));
    parsed.valid = !parsed.lhs.empty() && !parsed.rhs.empty();
    return parsed;
  }
  parsed.lhs = normalized;
  parsed.op = "!=";
  parsed.rhs = "0";
  parsed.valid = !parsed.lhs.empty();
  return parsed;
}

/**
 * @brief 收集中间代码中被引用的变量名和临时变量名。
 */
void collectReferencedNames(const IntermediateCode& code, std::vector<std::string>* names) {
  if (names == nullptr) {
    return;
  }

  auto addValue = [&](const std::string& value) {
    if (value.empty() || isIntegerLiteral(value)) {
      return;
    }
    appendUnique(names, value);
  };

  for (const TriAddrStmt& stmt : code.stmts) {
    switch (stmt.op) {
      case OP_ADD:
      case OP_SUB:
      case OP_MUL:
      case OP_DIV:
      case OP_MOD:
        addValue(stmt.arg1);
        addValue(stmt.arg2);
        addValue(stmt.result);
        break;
      case OP_ASSIGN:
        addValue(stmt.arg1);
        addValue(stmt.result);
        break;
      case OP_FUNC_CALL:
        addValue(stmt.result);
        for (const std::string& arg : splitArguments(stmt.arg2)) {
          addValue(arg);
        }
        break;
      case OP_IF_GOTO: {
        const ParsedCondition condition = parseCondition(stmt.arg1);
        addValue(condition.lhs);
        addValue(condition.rhs);
        break;
      }
      case OP_RETURN:
        addValue(stmt.arg1);
        break;
      case OP_GOTO:
        break;
    }
  }
}

/**
 * @brief 针对一棵 AST 临时生成一次三地址码。
 */
IntermediateCode generateCodeForAst(const ASTNode* node) {
  TriAddrGenerator generator;
  return generator.generate(const_cast<ASTNode*>(node));
}

/**
 * @brief 截取中间代码序列中的一个连续片段。
 */
IntermediateCode sliceCode(const IntermediateCode& code, std::size_t begin, std::size_t count) {
  IntermediateCode slice;
  const std::size_t end = begin + count;
  for (std::size_t index = begin; index < end && index < code.stmts.size(); ++index) {
    slice.addStmt(code.stmts[index]);
  }
  return slice;
}

/**
 * @brief 构造一个函数级输出单元，补齐形参与局部变量信息。
 */
FunctionUnit buildFunctionUnit(const ASTNode* function_node,
                               const IntermediateCode& function_code,
                               const TargetIrOptions& options) {
  FunctionUnit unit;
  unit.name = function_node != nullptr ? function_node->value : options.entryFunction;
  unit.return_type =
      normalizedType(function_node == nullptr ? "int" : function_node->varType);

  if (function_node != nullptr && function_node->type == NODE_FUNC_DEF) {
    for (std::size_t index = 0; index + 1 < function_node->children.size(); ++index) {
      const ASTNode* parameter = function_node->children[index];
      if (parameter != nullptr && parameter->type == NODE_VAR_DECL) {
        appendUnique(&unit.parameters, parameter->value);
      }
    }
    if (!function_node->children.empty()) {
      collectVarDecls(function_node->children.back(), &unit.locals);
    }
  } else {
    collectVarDecls(function_node, &unit.locals);
  }

  for (const std::string& parameter : unit.parameters) {
    unit.locals.erase(
        std::remove(unit.locals.begin(), unit.locals.end(), parameter),
        unit.locals.end());
  }

  unit.code = function_code;

  // 先根据声明收集一轮局部变量，再把三地址码里新出现的临时变量补进来，
  // 这样目标 IR 输出时不会遗漏存储槽位。
  std::vector<std::string> referenced;
  collectReferencedNames(unit.code, &referenced);
  for (const std::string& name : referenced) {
    if (std::find(unit.parameters.begin(), unit.parameters.end(), name) != unit.parameters.end()) {
      continue;
    }
    appendUnique(&unit.locals, name);
  }

  return unit;
}

/**
 * @brief 按函数粒度把 AST 与三地址码拆成多个输出单元。
 */
std::vector<FunctionUnit> buildFunctionUnits(const ASTNode* root,
                                             const IntermediateCode& code,
                                             const TargetIrOptions& options) {
  std::vector<FunctionUnit> units;
  if (root == nullptr) {
    units.push_back(buildFunctionUnit(nullptr, code, options));
    return units;
  }

  if (root->type == NODE_FUNC_DEF) {
    units.push_back(buildFunctionUnit(root, code, options));
    return units;
  }

  if (root->type == NODE_PROGRAM) {
    bool has_function = false;
    bool has_non_function = false;
    std::size_t offset = 0;
    for (const ASTNode* child : root->children) {
      if (!isFunctionNode(child)) {
        if (child != nullptr) {
          has_non_function = true;
        }
        continue;
      }
      has_function = true;
      const IntermediateCode function_shape = generateCodeForAst(child);
      const std::size_t function_size = function_shape.stmts.size();
      // 当前子集会把每个函数的三地址码连续输出，所以可以再按函数长度
      // 从整体代码流中切回函数级视图。
      if (offset + function_size > code.stmts.size()) {
        throw std::runtime_error("function partition exceeds provided intermediate code");
      }
      units.push_back(buildFunctionUnit(child, sliceCode(code, offset, function_size), options));
      offset += function_size;
    }
    if (has_function) {
      if (has_non_function) {
        throw std::runtime_error("mixed top-level functions and statements are not supported");
      }
      if (offset != code.stmts.size()) {
        throw std::runtime_error("provided intermediate code does not match AST function partition");
      }
      return units;
    }
  }

  units.push_back(buildFunctionUnit(root, code, options));
  return units;
}

/**
 * @brief 将内部类型名映射到 LLVM IR 类型名。
 */
std::string llvmTypeFor(const std::string& type_name) {
  return normalizedType(type_name) == "void" ? "void" : "i32";
}

/**
 * @brief 将比较运算符映射为 LLVM IR 比较指令名。
 */
std::string llvmCmpOp(const std::string& op) {
  if (op == "<") {
    return "icmp slt";
  }
  if (op == "<=") {
    return "icmp sle";
  }
  if (op == ">") {
    return "icmp sgt";
  }
  if (op == ">=") {
    return "icmp sge";
  }
  if (op == "==") {
    return "icmp eq";
  }
  return "icmp ne";
}

/**
 * @brief 将三地址算术操作映射到 LLVM IR 指令。
 */
std::string llvmBinaryOp(TriOp op) {
  switch (op) {
    case OP_ADD:
      return "add nsw";
    case OP_SUB:
      return "sub nsw";
    case OP_MUL:
      return "mul nsw";
    case OP_DIV:
      return "sdiv";
    case OP_MOD:
      return "srem";
    default:
      return "";
  }
}

/**
 * @brief 将内部类型名映射到 Jimple 方法签名类型。
 */
std::string jimpleSignatureType(const std::string& type_name) {
  return normalizedType(type_name) == "void" ? "void" : "int";
}

/**
 * @brief 将三地址算术操作映射到 Jimple 运算符。
 */
std::string jimpleBinaryOp(TriOp op) {
  switch (op) {
    case OP_ADD:
      return "+";
    case OP_SUB:
      return "-";
    case OP_MUL:
      return "*";
    case OP_DIV:
      return "/";
    case OP_MOD:
      return "%";
    default:
      return "";
  }
}

/**
 * @brief 将函数级三地址码划分为基本块。
 */
std::vector<IntermediateCode> partitionBlocks(const FunctionUnit& unit) {
  std::vector<IntermediateCode> blocks = splitBasicBlocks(unit.code);
  if (!blocks.empty()) {
    return blocks;
  }
  if (!unit.code.stmts.empty()) {
    return {unit.code};
  }
  return {};
}

/**
 * @brief 生成参数与局部变量的统一存储顺序。
 */
std::vector<std::string> collectStorageOrder(const FunctionUnit& unit) {
  std::vector<std::string> names;
  for (const std::string& parameter : unit.parameters) {
    appendUnique(&names, parameter);
  }
  for (const std::string& local : unit.locals) {
    appendUnique(&names, local);
  }
  return names;
}

/**
 * @brief 为 LLVM IR 中需要落栈的变量建立槽位表。
 */
std::unordered_map<std::string, std::string> buildLlvmSlots(
    const std::vector<std::string>& names) {
  std::unordered_map<std::string, std::string> slots;
  slots.reserve(names.size());
  for (const std::string& name : names) {
    const std::string identifier = requireIdentifier(name, "storage name");
    slots.emplace(identifier, "%" + sanitizeName(identifier) + ".addr");
  }
  return slots;
}

/**
 * @brief 生成一个函数的 LLVM IR 文本。
 */
std::string emitLlvmFunction(const FunctionUnit& unit) {
  const std::string function_name = requireIdentifier(unit.name, "function name");
  const std::vector<std::string> names = collectStorageOrder(unit);
  const std::unordered_map<std::string, std::string> slots = buildLlvmSlots(names);
  const std::vector<IntermediateCode> blocks = partitionBlocks(unit);

  std::unordered_map<int, std::string> block_labels;
  block_labels.reserve(blocks.size());
  // 先把基本块入口语句号映射成稳定标签，后续所有跳转都统一引用这里，
  // 避免在输出阶段反复扫描目标块位置。
  for (const IntermediateCode& block : blocks) {
    if (!block.stmts.empty()) {
      block_labels.emplace(block.stmts.front().stmtNo,
                           "bb_" + std::to_string(block.stmts.front().stmtNo));
    }
  }

  int next_register = 1;
  auto newRegister = [&]() {
    return "%r" + std::to_string(next_register++);
  };

  auto loadValue = [&](const std::string& value, std::ostringstream* out) -> std::string {
    const std::string operand = renderOperand(value, "operand");
    if (isIntegerLiteral(operand)) {
      return operand;
    }
    const auto found = slots.find(operand);
    if (found == slots.end()) {
      throw std::runtime_error("unknown operand: " + operand);
    }
    const std::string reg = newRegister();
    *out << "  " << reg << " = load i32, i32* " << found->second << '\n';
    return reg;
  };

  auto storeValue = [&](const std::string& name,
                        const std::string& value,
                        std::ostringstream* out) {
    const std::string identifier = requireIdentifier(name, "result name");
    const auto found = slots.find(identifier);
    if (found == slots.end()) {
      throw std::runtime_error("unknown storage: " + identifier);
    }
    *out << "  store i32 " << value << ", i32* " << found->second << '\n';
  };

  std::ostringstream out;
  bool needs_invalid_block = false;
  bool needs_exit_block = false;
  out << "define " << llvmTypeFor(unit.return_type) << " @" << function_name << '(';
  for (std::size_t index = 0; index < unit.parameters.size(); ++index) {
    if (index != 0) {
      out << ", ";
    }
    const std::string parameter = requireIdentifier(unit.parameters[index], "parameter name");
    out << "i32 %" << sanitizeName(parameter) << ".in";
  }
  out << ") {\n";
  out << "entry:\n";

  // LLVM 中间表示使用显式栈槽保存所有局部值，这样三地址码里的“变量名”
  // 可以直接对应到 `alloca` 出来的可写地址。
  for (const std::string& name : names) {
    out << "  " << slots.at(name) << " = alloca i32\n";
  }
  for (const std::string& parameter : unit.parameters) {
    const std::string identifier = requireIdentifier(parameter, "parameter name");
    out << "  store i32 %" << sanitizeName(identifier) << ".in, i32* " << slots.at(identifier)
        << '\n';
  }

  if (blocks.empty()) {
    if (normalizedType(unit.return_type) == "void") {
      out << "  ret void\n";
    } else {
      out << "  ret i32 0\n";
    }
    out << "}\n";
    return out.str();
  }

  out << "  br label %" << block_labels.at(blocks.front().stmts.front().stmtNo) << '\n';
  for (std::size_t block_index = 0; block_index < blocks.size(); ++block_index) {
    const IntermediateCode& block = blocks[block_index];
    const int leader = block.stmts.front().stmtNo;
    out << '\n' << block_labels.at(leader) << ":\n";

    bool terminated = false;
    // 每个基本块按三地址语句原顺序线性降低；
    // 一旦遇到跳转或返回，该块就被视为已经终结。
    for (const TriAddrStmt& stmt : block.stmts) {
      switch (stmt.op) {
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_MOD: {
          const std::string lhs = loadValue(stmt.arg1, &out);
          const std::string rhs = loadValue(stmt.arg2, &out);
          const std::string value_reg = newRegister();
          out << "  " << value_reg << " = " << llvmBinaryOp(stmt.op) << " i32 " << lhs
              << ", " << rhs << '\n';
          storeValue(stmt.result, value_reg, &out);
          break;
        }
        case OP_ASSIGN: {
          const std::string value = loadValue(stmt.arg1, &out);
          storeValue(stmt.result, value, &out);
          break;
        }
        case OP_FUNC_CALL: {
          const std::vector<std::string> args = splitArguments(stmt.arg2);
          std::vector<std::string> lowered_args;
          lowered_args.reserve(args.size());
          for (const std::string& arg : args) {
            lowered_args.push_back(loadValue(arg, &out));
          }
          const std::string callee = requireIdentifier(stmt.arg1, "callee name");
          const std::string call_reg = newRegister();
          out << "  " << call_reg << " = call i32 @" << callee << '(';
          for (std::size_t index = 0; index < lowered_args.size(); ++index) {
            if (index != 0) {
              out << ", ";
            }
            out << "i32 " << lowered_args[index];
          }
          out << ")\n";
          storeValue(stmt.result, call_reg, &out);
          break;
        }
        case OP_IF_GOTO: {
          const ParsedCondition condition = parseCondition(stmt.arg1);
          const std::string lhs = loadValue(condition.lhs, &out);
          const std::string rhs = loadValue(condition.rhs, &out);
          const std::string cond_reg = newRegister();
          out << "  " << cond_reg << " = " << llvmCmpOp(condition.op) << " i32 " << lhs
              << ", " << rhs << '\n';
          int target_stmt = 0;
          const auto target =
              parseStatementNumber(stmt.result, &target_stmt) ? block_labels.find(target_stmt)
                                                              : block_labels.end();
          const std::string true_label =
              target == block_labels.end() ? "bb_invalid" : target->second;
          if (target == block_labels.end()) {
            needs_invalid_block = true;
          }
          std::string false_label = "bb_exit";
          if (block_index + 1 < blocks.size()) {
            false_label = block_labels.at(blocks[block_index + 1].stmts.front().stmtNo);
          } else {
            needs_exit_block = true;
          }
          // 三地址条件跳转只显式保存真分支目标，假分支在原始代码中表现为顺序落空，
          // 到 LLVM IR 这里需要把这条假边显式补成第二个跳转目标。
          out << "  br i1 " << cond_reg << ", label %" << true_label << ", label %"
              << false_label << '\n';
          terminated = true;
          break;
        }
        case OP_GOTO: {
          int target_stmt = 0;
          const auto target =
              parseStatementNumber(stmt.result, &target_stmt) ? block_labels.find(target_stmt)
                                                              : block_labels.end();
          const std::string label =
              target == block_labels.end() ? "bb_invalid" : target->second;
          if (target == block_labels.end()) {
            needs_invalid_block = true;
          }
          out << "  br label %" << label << '\n';
          terminated = true;
          break;
        }
        case OP_RETURN: {
          if (stmt.arg1.empty() || normalizedType(unit.return_type) == "void") {
            out << "  ret void\n";
          } else {
            const std::string value = loadValue(stmt.arg1, &out);
            out << "  ret i32 " << value << '\n';
          }
          terminated = true;
          break;
        }
      }
    }

    if (!terminated) {
      if (block_index + 1 < blocks.size()) {
        out << "  br label %" << block_labels.at(blocks[block_index + 1].stmts.front().stmtNo)
            << '\n';
      } else if (normalizedType(unit.return_type) == "void") {
        out << "  ret void\n";
      } else {
        out << "  ret i32 0\n";
      }
    }
  }

  if (needs_invalid_block) {
    out << "\nbb_invalid:\n";
    if (normalizedType(unit.return_type) == "void") {
      out << "  ret void\n";
    } else {
      out << "  ret i32 0\n";
    }
  }
  if (needs_exit_block) {
    out << "\nbb_exit:\n";
    if (normalizedType(unit.return_type) == "void") {
      out << "  ret void\n";
    } else {
      out << "  ret i32 0\n";
    }
  }

  out << "}\n";
  return out.str();
}

/**
 * @brief 生成一个函数的 Jimple 文本。
 */
std::string emitJimpleFunction(const FunctionUnit& unit, const TargetIrOptions& options) {
  const std::string function_name = requireIdentifier(unit.name, "function name");
  const std::string class_name = requireIdentifier(options.className, "class name");
  const std::vector<std::string> names = collectStorageOrder(unit);
  const std::vector<IntermediateCode> blocks = partitionBlocks(unit);

  std::unordered_map<int, std::string> block_labels;
  block_labels.reserve(blocks.size());
  // Jimple 中间表示同样基于基本块语句号建标签，但这里直接输出成文本标签，
  // 不再需要像 LLVM IR 那样显式维护 SSA 寄存器。
  for (const IntermediateCode& block : blocks) {
    if (!block.stmts.empty()) {
      block_labels.emplace(block.stmts.front().stmtNo,
                           "label_" + std::to_string(block.stmts.front().stmtNo));
    }
  }

  std::ostringstream out;
  bool needs_invalid_label = false;
  bool needs_exit_label = false;
  out << ".method public static " << jimpleSignatureType(unit.return_type) << ' '
      << function_name << '(';
  for (std::size_t index = 0; index < unit.parameters.size(); ++index) {
    if (index != 0) {
      out << ", ";
    }
    out << "int " << requireIdentifier(unit.parameters[index], "parameter name");
  }
  out << ")\n{\n";

  // 形参已在方法签名里出现，因此这里只声明真正需要单独存储的局部变量。
  for (const std::string& local : names) {
    if (std::find(unit.parameters.begin(), unit.parameters.end(), local) != unit.parameters.end()) {
      continue;
    }
    out << "  int " << requireIdentifier(local, "local name") << ";\n";
  }

  if (blocks.empty()) {
    if (jimpleSignatureType(unit.return_type) == "void") {
      out << "  return;\n";
    } else {
      out << "  return 0;\n";
    }
    out << "}\n";
    return out.str();
  }

  for (std::size_t block_index = 0; block_index < blocks.size(); ++block_index) {
    const IntermediateCode& block = blocks[block_index];
    const int leader = block.stmts.front().stmtNo;
    out << block_labels.at(leader) << ":\n";

    bool terminated = false;
    for (const TriAddrStmt& stmt : block.stmts) {
      switch (stmt.op) {
        case OP_ADD:
        case OP_SUB:
        case OP_MUL:
        case OP_DIV:
        case OP_MOD:
          out << "  " << requireIdentifier(stmt.result, "result name") << " = "
              << renderOperand(stmt.arg1, "operand") << ' ' << jimpleBinaryOp(stmt.op) << ' '
              << renderOperand(stmt.arg2, "operand") << ";\n";
          break;
        case OP_ASSIGN:
          out << "  " << requireIdentifier(stmt.result, "result name") << " = "
              << renderOperand(stmt.arg1, "operand") << ";\n";
          break;
        case OP_FUNC_CALL:
          out << "  " << requireIdentifier(stmt.result, "result name") << " = staticinvoke "
              << class_name << '.' << requireIdentifier(stmt.arg1, "callee name") << '(';
          {
            const std::vector<std::string> args = splitArguments(stmt.arg2);
            for (std::size_t index = 0; index < args.size(); ++index) {
              if (index != 0) {
                out << ", ";
              }
              out << renderOperand(args[index], "operand");
            }
          }
          out << ");\n";
          break;
        case OP_IF_GOTO: {
          int target_stmt = 0;
          const auto target =
              parseStatementNumber(stmt.result, &target_stmt) ? block_labels.find(target_stmt)
                                                              : block_labels.end();
          const std::string label =
              target == block_labels.end() ? "label_invalid" : target->second;
          if (target == block_labels.end()) {
            needs_invalid_label = true;
          }
          std::string false_label = "label_exit";
          if (block_index + 1 < blocks.size()) {
            false_label = block_labels.at(blocks[block_index + 1].stmts.front().stmtNo);
          } else {
            needs_exit_label = true;
          }
          // 这里的 Jimple 文本表示没有隐式落空边，因此需要显式补一条
          // 指向下一个基本块的 `goto` 来表示假分支。
          {
            const ParsedCondition condition = parseCondition(stmt.arg1);
            out << "  if " << renderOperand(condition.lhs, "condition operand") << ' '
                << condition.op << ' ' << renderOperand(condition.rhs, "condition operand")
                << " goto " << label << ";\n";
          }
          out << "  goto " << false_label << ";\n";
          terminated = true;
          break;
        }
        case OP_GOTO: {
          int target_stmt = 0;
          const auto target =
              parseStatementNumber(stmt.result, &target_stmt) ? block_labels.find(target_stmt)
                                                              : block_labels.end();
          const std::string label =
              target == block_labels.end() ? "label_invalid" : target->second;
          if (target == block_labels.end()) {
            needs_invalid_label = true;
          }
          out << "  goto " << label << ";\n";
          terminated = true;
          break;
        }
        case OP_RETURN:
          if (stmt.arg1.empty() || jimpleSignatureType(unit.return_type) == "void") {
            out << "  return;\n";
          } else {
            out << "  return " << renderOperand(stmt.arg1, "return operand") << ";\n";
          }
          terminated = true;
          break;
      }
    }

    if (!terminated && block_index + 1 < blocks.size()) {
      out << "  goto " << block_labels.at(blocks[block_index + 1].stmts.front().stmtNo)
          << ";\n";
    } else if (!terminated) {
      if (jimpleSignatureType(unit.return_type) == "void") {
        out << "  return;\n";
      } else {
        out << "  return 0;\n";
      }
    }
  }

  if (needs_invalid_label) {
    out << "label_invalid:\n";
    if (jimpleSignatureType(unit.return_type) == "void") {
      out << "  return;\n";
    } else {
      out << "  return 0;\n";
    }
  }
  if (needs_exit_label) {
    out << "label_exit:\n";
    if (jimpleSignatureType(unit.return_type) == "void") {
      out << "  return;\n";
    } else {
      out << "  return 0;\n";
    }
  }

  out << "}\n";
  return out.str();
}

}  // 匿名命名空间

/**
 * @brief 生成完整的 LLVM IR 文本输出。
 */
std::string formatLlvmIr(const ASTNode* root,
                         const IntermediateCode& code,
                         const TargetIrOptions& options) {
  // 函数内容逐个独立输出，但模块级注释只写一次，
  // 这样生成结果更稳定，也更容易做文本比对。
  const std::vector<FunctionUnit> units = buildFunctionUnits(root, code, options);
  std::ostringstream out;
  if (options.emitComments) {
    out << "; module: " << sanitizeCommentText(options.moduleName) << '\n';
  }
  for (std::size_t index = 0; index < units.size(); ++index) {
    if (index != 0) {
      out << '\n';
    }
    out << emitLlvmFunction(units[index]);
  }
  return out.str();
}

/**
 * @brief 生成完整的 Jimple 文本输出。
 */
std::string formatJimple(const ASTNode* root,
                         const IntermediateCode& code,
                         const TargetIrOptions& options) {
  const std::vector<FunctionUnit> units = buildFunctionUnits(root, code, options);
  std::ostringstream out;
  if (options.emitComments) {
    out << ".class public final " << requireIdentifier(options.className, "class name") << '\n';
    out << ".super java.lang.Object\n\n";
  }
  for (std::size_t index = 0; index < units.size(); ++index) {
    if (index != 0) {
      out << '\n';
    }
    out << emitJimpleFunction(units[index], options);
  }
  return out.str();
}

/**
 * @brief 将 LLVM IR 文本写入给定输出流。
 */
void dumpLlvmIr(const ASTNode* root,
                const IntermediateCode& code,
                std::ostream& out,
                const TargetIrOptions& options) {
  out << formatLlvmIr(root, code, options);
}

/**
 * @brief 将 Jimple 文本写入给定输出流。
 */
void dumpJimple(const ASTNode* root,
                const IntermediateCode& code,
                std::ostream& out,
                const TargetIrOptions& options) {
  out << formatJimple(root, code, options);
}

}  // 命名空间 seu_icg
