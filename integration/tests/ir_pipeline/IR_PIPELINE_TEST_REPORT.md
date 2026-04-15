# IR Pipeline Test Report

## 1. 测试概述与覆盖范围

- 测试对象：`Lex -> Yacc -> AST -> TAC -> LLVM IR / Jimple` 全流程。
- 执行脚本：[run_ir_pipeline_test.sh](run_ir_pipeline_test.sh)
- 输入规格：
  - Lex：[demo_ir.l](test_cases/demo_ir.l)
  - Yacc：[demo_ir.y](test_cases/demo_ir.y)
  - 演示程序：[demo_ir.c](test_cases/demo_ir.c)
- 预期输出：
  - [demo_ir.tac](expected/demo_ir.tac)
  - [demo_ir.ll](expected/demo_ir.ll)
  - [demo_ir.jimple](expected/demo_ir.jimple)
- 最新运行结果目录：[results/20260415_113422](results/20260415_113422)
- 最新运行指针：[results/LATEST.txt](results/LATEST.txt)

覆盖能力：

- 函数定义与参数
- 局部变量声明
- 常量赋值与算术表达式
- 函数调用
- `if / else`
- `while`
- `return`
- 三地址码、LLVM IR、Jimple 三份输出的稳定比对

## 2. 详细记录

### demo_full_pipeline_to_llvm_and_jimple

- 名称：完整 IR 导出演示
- 目的：验证小而全的 C 子集程序能稳定走通三模块并导出三种后端文本。
- 输入：`demo_ir.c`，共 34 行，包含 `inc` 和 `main` 两个函数。
- 测试步骤：
  1. 用 `seuYacc` 生成 `demo_ir_parser.cpp` 和 `demo_ir_tokens.h`。
  2. 用 `seuLex --token-header demo_ir_tokens.h` 生成 ABI 模式 lexer。
  3. 编译临时 driver，并读取 `demo_ir.c`。
  4. 执行 `tokenize_for_parser(source)` 和 `yyparse(tokens)`。
  5. 调用 `releaseParseRoot()` 获取 AST。
  6. 调用 `TriAddrGenerator::generate(root)` 生成 TAC。
  7. 调用 `formatLlvmIr(root, code, options)` 生成 LLVM IR。
  8. 调用 `formatJimple(root, code, options)` 生成 Jimple。
  9. 将三份输出分别与 `expected/` 基线做精确比对。
- 预期输出：
  - TAC 含 `call inc(y)`、条件跳转和循环回边
  - LLVM IR 含 `define i32 @inc`、`define i32 @main`、`call i32 @inc`、`br i1`、`ret i32`
  - Jimple 含 `.method public static int inc`、`.method public static int main`、`staticinvoke DemoIrPipeline.inc(y)`、`if ... goto` 和 `return x;`
- 实际结果：
  - [demo_ir.tac](results/20260415_113422/actual/demo_ir.tac)
  - [demo_ir.ll](results/20260415_113422/actual/demo_ir.ll)
  - [demo_ir.jimple](results/20260415_113422/actual/demo_ir.jimple)
- 结论分析：本用例已稳定覆盖声明、赋值、算术、条件、循环、调用和返回，满足课程验收所需的小型全流程演示。

## 3. 总体结论

- 当前仓库已同时保留最小链路样例和完整 IR 导出样例。
- 新增 IR pipeline 在最新一次运行中通过三份输出基线比对，没有发现行为回归。
- `intermediate` 新增的 `TargetIrEmitter` 已能和现有 `seuLex` / `seuYacc` 直接联动，不需要改动现有 Token ABI 或 AST 交接方式。

## 4. 发现的问题与改进建议

- 当前脚本会把 `.l/.y` 中的用户动作生成并编译为本地 C++，因此默认前提仍然是“规格可信”。
- 为降低 CI 风险，脚本已新增 `SEU_TRUSTED_SPECS=1` 显式信任开关；在 CI 环境中未显式开启时会拒绝执行。
- 若后续继续扩展中间代码阶段，建议增加：
  - 更复杂的表达式和比较运算
  - `void` 函数与空返回
  - 数组、指针、结构体等更高阶类型
