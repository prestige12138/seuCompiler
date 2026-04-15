# seuIntermediate 测试报告

## 1. 测试概述与覆盖范围

- 测试对象：`seuIntermediate` 中间代码生成模块，覆盖 AST 构造辅助、parse-root 交接、符号表、三地址码生成、格式化输出、CLI 自测和性能烟雾测试。
- 执行脚本：[run_icg_tests.sh](run_icg_tests.sh)
- 辅助探针：[icg_probe.cpp](icg_probe.cpp)
- 用例目录：[test_cases/](test_cases)
- 预期基线目录：[expected/](expected)
- 本次实际运行结果目录：[results/20260415_113413](results/20260415_113413)
- 最新运行指针：[results/LATEST.txt](results/LATEST.txt)
- 汇总结果：[SUMMARY.md](results/20260415_113413/SUMMARY.md)
- 实际执行结果：共 `26` 条测试，`26` 条通过，`0` 条失败。
- 额外验证：执行 `ctest --test-dir build --output-on-failure`，结果为 `1/1` 通过。

本套测试覆盖了以下能力面：

- AST 基础构造、节点类型覆盖、前序结构稳定性
- `setParseRoot` / `getParseRoot` / `releaseParseRoot` 契约
- `SymbolTable` 的作用域遮蔽、同层重复拒绝、全局/参数/局部偏移分配
- `astNodeTypeName`、`triOpName`、`formatTriAddrStmt`、`dumpIntermediateCode`
- 变量声明初始化、赋值、算术运算、函数调用、`if/else`、`while`、`return`
- 函数体生成与函数作用域退出后的局部符号回收
- 错误路径：不支持运算符异常
- 边界路径：空根节点与空程序块
- 基本块 leader 识别、块范围切分、后继块格式化
- LLVM IR 输出与格式稳定性
- Jimple 输出与格式稳定性
- 官方入口 `seuIntermediate --self-test`
- 重复生成 `2000` 次的性能烟雾测试与原始耗时记录

当前实现的两个边界也被明确纳入测试口径：

- 当前模块输入是上游语义动作构造出的 AST，而不是直接读取 `.y` / token 流，因此模块测试仍以“AST/parse-root 契约”为主；三模块端到端联调已由根目录 `pipeline` 与 `ir_pipeline` 集成测试承担。
- 当前性能测试记录原始耗时，但不设置强阈值，避免不同机器或沙箱环境导致误判；性能回归主要通过固定规模下的功能稳定性与时间日志观察。

## 2. 详细测试记录

### 01. `ast_construction_basic`

- 名称：AST 基础构造与类型注入
- 目的：验证 `ASTBuilder` 能正确构造声明、赋值、算术表达式树，并支持后续类型覆盖。
- 输入：[01_ast_construction_basic.md](test_cases/01_ast_construction_basic.md)
- 测试步骤：
  1. 编译 `icg_probe.cpp`。
  2. 运行 `./icg_probe ast-shape`。
  3. 将结果与 [01_ast_construction_basic.txt](expected/01_ast_construction_basic.txt) 做精确比对。
- 预期输出：根节点为 `NODE_PROGRAM`，赋值节点的 `type` 为 `int`，子树结构与手工构造一致。
- 实际结果：[01_ast_construction_basic.txt](results/20260415_002706/01_ast_construction_basic.txt)，与预期完全一致。
- 结论分析：AST 构造辅助接口稳定，可直接作为 Yacc 语义动作的节点工厂。

### 02. `parse_root_handoff`

- 名称：Parse Root 交接
- 目的：验证全局 parse-root 槽位的设置、观察与释放语义。
- 输入：[02_parse_root_handoff.md](test_cases/02_parse_root_handoff.md)
- 测试步骤：
  1. 运行 `./icg_probe parse-root`。
  2. 检查首次观察、首次释放、释放后清空、二次释放四个字段。
  3. 与 [02_parse_root_handoff.txt](expected/02_parse_root_handoff.txt) 比对。
- 预期输出：四个字段均为 `yes`。
- 实际结果：[02_parse_root_handoff.txt](results/20260415_002706/02_parse_root_handoff.txt)，全部满足预期。
- 结论分析：当前 `seuYacc -> seuIntermediate` 的 parse-root 交接契约是可用的。

### 03. `symbol_scope_shadowing`

- 名称：符号表作用域遮蔽
- 目的：验证内外层同名变量遮蔽、当前层重复声明拒绝、退栈后查找恢复。
- 输入：[03_symbol_scope_shadowing.md](test_cases/03_symbol_scope_shadowing.md)
- 测试步骤：
  1. 运行 `./icg_probe symbol-scope`。
  2. 检查全局声明、内层声明、重复拒绝、遮蔽类型、退栈结果。
  3. 与 [03_symbol_scope_shadowing.txt](expected/03_symbol_scope_shadowing.txt) 比对。
- 预期输出：内层 `x` 为 `char`、作用域为 `1`，退出作用域后 `x` 恢复为全局 `int`，`y` 消失。
- 实际结果：[03_symbol_scope_shadowing.txt](results/20260415_002706/03_symbol_scope_shadowing.txt)，字段全部匹配。
- 结论分析：符号表作用域栈行为正确，满足局部变量遮蔽的基本需求。

### 04. `symbol_offsets_global_local_param`

- 名称：全局/局部/参数偏移分配
- 目的：验证偏移按类型宽度累计，并对全局、参数、局部分别维护计数。
- 输入：[04_symbol_offsets_global_local_param.md](test_cases/04_symbol_offsets_global_local_param.md)
- 测试步骤：
  1. 运行 `./icg_probe symbol-offsets`。
  2. 读取全局、参数、局部符号的偏移值。
  3. 与 [04_symbol_offsets_global_local_param.txt](expected/04_symbol_offsets_global_local_param.txt) 比对。
- 预期输出：偏移分别为 `0/4`、`0/8`、`0/2`。
- 实际结果：[04_symbol_offsets_global_local_param.txt](results/20260415_002706/04_symbol_offsets_global_local_param.txt)，与预期一致。
- 结论分析：当前实现虽未做对齐优化，但偏移累计规则稳定且可预测。

### 05. `format_helpers`

- 名称：格式化辅助函数
- 目的：验证枚举名称映射和三地址码文本输出保持稳定。
- 输入：[05_format_helpers.md](test_cases/05_format_helpers.md)
- 测试步骤：
  1. 运行 `./icg_probe formatters`。
  2. 检查 AST 节点名、操作名、单语句格式和整体 IR 转储。
  3. 与 [05_format_helpers.txt](expected/05_format_helpers.txt) 比对。
- 预期输出：`NODE_WHILE`、`OP_FUNC_CALL` 和 `7: t3 = call foo(x, 1)` 等文本固定。
- 实际结果：[05_format_helpers.txt](results/20260415_002706/05_format_helpers.txt)，完全匹配。
- 结论分析：格式化接口可安全用于文档输出、调试打印和教学展示。

### 06. `ir_constant_assignment`

- 名称：常量初始化与返回 IR
- 目的：验证带初始化声明的 IR 生成和 `return` 输出。
- 输入：[06_ir_constant_assignment.md](test_cases/06_ir_constant_assignment.md)
- 测试步骤：
  1. 运行 `./icg_probe ir-constant-assign`。
  2. 检查赋值和返回两条语句。
  3. 与 [06_ir_constant_assignment.txt](expected/06_ir_constant_assignment.txt) 比对。
- 预期输出：`1: x = 42` 与 `2: return x`。
- 实际结果：[06_ir_constant_assignment.txt](results/20260415_002706/06_ir_constant_assignment.txt)，与预期一致。
- 结论分析：变量声明初始化路径可正常落到三地址赋值语句。

### 07. `ir_arithmetic_assignment`

- 名称：算术表达式赋值 IR
- 目的：验证临时变量生成顺序和运算树后序展开次序。
- 输入：[07_ir_arithmetic_assignment.md](test_cases/07_ir_arithmetic_assignment.md)
- 测试步骤：
  1. 运行 `./icg_probe ir-arithmetic`。
  2. 检查乘法临时变量、加法临时变量和最终赋值。
  3. 与 [07_ir_arithmetic_assignment.txt](expected/07_ir_arithmetic_assignment.txt) 比对。
- 预期输出：`t1 = 2 * 3`，`t2 = 1 + t1`，`x = t2`。
- 实际结果：[07_ir_arithmetic_assignment.txt](results/20260415_002706/07_ir_arithmetic_assignment.txt)，完全一致。
- 结论分析：当前 `TriAddrGenerator` 的算术表达式展开顺序稳定。

### 08. `ir_function_call_and_control_flow`

- 名称：函数调用与控制流 IR
- 目的：验证 `if/else`、`while`、函数调用和跳转目标回填。
- 输入：[08_ir_function_call_and_control_flow.md](test_cases/08_ir_function_call_and_control_flow.md)
- 测试步骤：
  1. 运行 `./icg_probe ir-control-flow`。
  2. 检查条件跳转、空转移、循环回跳和 `call foo(x, 1)`。
  3. 与 [08_ir_function_call_and_control_flow.txt](expected/08_ir_function_call_and_control_flow.txt) 比对。
- 预期输出：共 `12` 条语句，跳转目标分别为 `3/6/7/9/12`。
- 实际结果：[08_ir_function_call_and_control_flow.txt](results/20260415_002706/08_ir_function_call_and_control_flow.txt)，所有编号与预期一致。
- 结论分析：控制流回填逻辑当前是正确的，没有出现目标行号漂移。

### 09. `ir_function_body`

- 名称：函数体 IR 与作用域回收
- 目的：验证函数参数登记、函数体展开及函数作用域退出后的局部回收。
- 输入：[09_ir_function_body.md](test_cases/09_ir_function_body.md)
- 测试步骤：
  1. 运行 `./icg_probe ir-function`。
  2. 检查三条函数体 IR。
  3. 检查 `function_declared`、`local_c_visible_after_exit`、`param_a_visible_after_exit`。
  4. 与 [09_ir_function_body.txt](expected/09_ir_function_body.txt) 比对。
- 预期输出：函数符号仍可见，局部 `c` 和参数 `a` 在函数退出后不可见。
- 实际结果：[09_ir_function_body.txt](results/20260415_002706/09_ir_function_body.txt)，全部匹配。
- 结论分析：函数级符号表进出栈行为正确，生成器没有泄漏局部符号到外层。

### 10. `error_unsupported_operator`

- 名称：不支持运算符错误路径
- 目的：验证报告外运算符会触发明确异常。
- 输入：[10_error_unsupported_operator.md](test_cases/10_error_unsupported_operator.md)
- 测试步骤：
  1. 运行 `./icg_probe ir-unsupported-op`。
  2. 捕获探针输出的异常文本。
  3. 与 [10_error_unsupported_operator.txt](expected/10_error_unsupported_operator.txt) 比对。
- 预期输出：`error=unsupported arithmetic operator: ^`。
- 实际结果：[10_error_unsupported_operator.txt](results/20260415_002706/10_error_unsupported_operator.txt)，与预期完全一致。
- 结论分析：当前错误信息对定位问题是足够明确的。

### 11. `generate_empty_root`

- 名称：空根节点生成
- 目的：验证空输入不会崩溃，并返回空 IR。
- 输入：[11_generate_empty_root.md](test_cases/11_generate_empty_root.md)
- 测试步骤：
  1. 运行 `./icg_probe generate-empty`。
  2. 分别检查 `generate(nullptr)` 和空 `NODE_PROGRAM` 的输出。
  3. 与 [11_generate_empty_root.txt](expected/11_generate_empty_root.txt) 比对。
- 预期输出：两类输入均为 `stmtCount=0` 且格式化文本为空。
- 实际结果：[11_generate_empty_root.txt](results/20260415_002706/11_generate_empty_root.txt)，与预期一致。
- 结论分析：边界输入处理稳定，不会在空树场景下崩溃。

### 12. `cli_self_test`

- 名称：命令行自测回归
- 目的：验证 `seuIntermediate --self-test` 入口仍可完整通过。
- 输入：[12_cli_self_test.md](test_cases/12_cli_self_test.md)
- 测试步骤：
  1. 通过 CMake 构建 `seuIntermediate`。
  2. 运行 `./build/seuIntermediate --self-test`。
  3. 将退出码、stdout、stderr 归一化。
  4. 与 [12_cli_self_test.txt](expected/12_cli_self_test.txt) 比对。
- 预期输出：退出码为 `0`，九条自测全部为 `ok`。
- 实际结果：[12_cli_self_test.txt](results/20260415_113413/12_cli_self_test.txt)，结果完全一致。
- 结论分析：脚本化测试与模块官方自测保持一致，没有出现两套测试口径分叉。

### 13. `perf_batch_generation`

- 名称：批量生成性能烟雾测试
- 目的：验证重复生成 IR 时功能稳定，并记录原始耗时供回归观察。
- 输入：[13_perf_batch_generation.md](test_cases/13_perf_batch_generation.md)
- 测试步骤：
  1. 运行 `./icg_probe perf-batch 2000`。
  2. 将输出与 [13_perf_batch_generation.txt](expected/13_perf_batch_generation.txt) 比对。
  3. 将原始耗时写入 [13_perf_batch_generation.metrics](results/20260415_002706/logs/13_perf_batch_generation.metrics)。
- 预期输出：`iterations=2000`，`final_stmt_count=3`，`total_stmt_count=6000`，`status=ok`。
- 实际结果：[13_perf_batch_generation.txt](results/20260415_113413/13_perf_batch_generation.txt)，功能结果匹配；原始耗时记录为 `elapsed_seconds=0`。
- 结论分析：当前规模下批量生成稳定可复现；后续如需性能门禁，应引入更大规模样本和独立基准环境。

### 14. `basic_block_empty_code`

- 名称：空代码基本块划分
- 目的：验证空 `IntermediateCode` 不会伪造 leader 或基本块。
- 输入：[14_basic_block_empty_code.md](test_cases/14_basic_block_empty_code.md)
- 测试步骤：
  1. 运行 `./icg_probe basic-block-empty`。
  2. 检查 `block_count=0` 和 `leaders=<empty>`。
  3. 与 [14_basic_block_empty_code.txt](expected/14_basic_block_empty_code.txt) 比对。
- 预期输出：无基本块、无 leader。
- 实际结果：[14_basic_block_empty_code.txt](results/20260415_113413/14_basic_block_empty_code.txt)，与预期一致。
- 结论分析：空 IR 输入下的基本块切分行为稳定。

### 15. `basic_block_linear_fallthrough`

- 名称：顺序代码基本块划分
- 目的：验证无跳转的顺序代码不会被错误切块。
- 输入：[15_basic_block_linear_fallthrough.md](test_cases/15_basic_block_linear_fallthrough.md)
- 测试步骤：
  1. 运行 `./icg_probe basic-block-linear`。
  2. 检查仅生成 `1` 个基本块，leader 为 `1`。
  3. 与 [15_basic_block_linear_fallthrough.txt](expected/15_basic_block_linear_fallthrough.txt) 比对。
- 预期输出：单块覆盖 `1-4`，无后继。
- 实际结果：[15_basic_block_linear_fallthrough.txt](results/20260415_113413/15_basic_block_linear_fallthrough.txt)，与预期一致。
- 结论分析：当前切分规则不会对纯顺序代码过度分块。

### 16. `basic_block_conditional_branch`

- 名称：条件分支基本块划分
- 目的：验证条件跳转目标、跳转后继和 `return` 终止的 leader 识别。
- 输入：[16_basic_block_conditional_branch.md](test_cases/16_basic_block_conditional_branch.md)
- 测试步骤：
  1. 运行 `./icg_probe basic-block-conditional`。
  2. 检查 leaders 为 `1,2,3,5`。
  3. 检查 `B1 -> B3, B2` 与 `B2 -> B4`。
  4. 与 [16_basic_block_conditional_branch.txt](expected/16_basic_block_conditional_branch.txt) 比对。
- 预期输出：共 `4` 个块，条件与无条件后继都正确。
- 实际结果：[16_basic_block_conditional_branch.txt](results/20260415_113413/16_basic_block_conditional_branch.txt)，与预期一致。
- 结论分析：基本块划分已经正确覆盖典型分支结构。

### 17. `basic_block_mixed_control_flow`

- 名称：混合控制流基本块划分
- 目的：验证循环回边、函数调用块和退出块的综合切分。
- 输入：[17_basic_block_mixed_control_flow.md](test_cases/17_basic_block_mixed_control_flow.md)
- 测试步骤：
  1. 运行 `./icg_probe basic-block-mixed`。
  2. 检查 leaders 为 `1,2,3,6,7,8,9,12`。
  3. 检查回边 `B7 -> B5` 和退出块 `B8`。
  4. 与 [17_basic_block_mixed_control_flow.txt](expected/17_basic_block_mixed_control_flow.txt) 比对。
- 预期输出：共 `8` 个块，后继块关系与控制流一致。
- 实际结果：[17_basic_block_mixed_control_flow.txt](results/20260415_113413/17_basic_block_mixed_control_flow.txt)，与预期一致。
- 结论分析：当前实现已经能对复杂控制流 IR 形成稳定基本块视图。

### 18. `basic_block_sparse_stmt_numbers`

- 名称：稀疏语句号基本块划分
- 目的：验证 `stmtNo` 非连续时的块成员显示与切分规则。
- 输入：[18_basic_block_sparse_stmt_numbers.md](test_cases/18_basic_block_sparse_stmt_numbers.md)
- 测试步骤：
  1. 运行 `./icg_probe basic-block-sparse`。
  2. 检查 leaders 为 `10`。
  3. 检查块成员显示为 `stmts=[10,20,30]`。
  4. 与 [18_basic_block_sparse_stmt_numbers.txt](expected/18_basic_block_sparse_stmt_numbers.txt) 比对。
- 预期输出：单块包含 `10,20,30` 三条语句，不伪装为连续区间。
- 实际结果：[18_basic_block_sparse_stmt_numbers.txt](results/20260415_113413/18_basic_block_sparse_stmt_numbers.txt)，与预期一致。
- 结论分析：当前基本块格式化对稀疏语句号是稳定的。

### 19. `basic_block_invalid_target`

- 名称：非法跳转目标处理
- 目的：验证跳转目标不存在时不会崩溃，并给出固定后继表示。
- 输入：[19_basic_block_invalid_target.md](test_cases/19_basic_block_invalid_target.md)
- 测试步骤：
  1. 运行 `./icg_probe basic-block-invalid-target`。
  2. 检查 leaders 为 `10,20`。
  3. 检查首块后继显示为 `invalid(99)`。
  4. 与 [19_basic_block_invalid_target.txt](expected/19_basic_block_invalid_target.txt) 比对。
- 预期输出：非法目标不生成 leader，但会被稳定标记为 `invalid(99)`。
- 实际结果：[19_basic_block_invalid_target.txt](results/20260415_113413/19_basic_block_invalid_target.txt)，与预期一致。
- 结论分析：非法目标输入当前不会导致崩溃，且格式化契约明确。

### 20. `llvm_linear_arith_and_return`

- 名称：LLVM 线性算术与返回
- 目的：验证 `assign/add/return` 到 LLVM IR 的最小映射稳定。
- 输入：[20_llvm_linear_arith_and_return.md](test_cases/20_llvm_linear_arith_and_return.md)
- 测试步骤：
  1. 运行 `./icg_probe llvm-linear`。
  2. 检查 `define`、`alloca`、`add nsw` 与 `ret i32`。
  3. 与 [20_llvm_linear_arith_and_return.txt](expected/20_llvm_linear_arith_and_return.txt) 比对。
- 预期输出：产生一段 `main` 函数 LLVM IR，包含 `a/b/t1` 三个存储槽和 `add nsw i32`。
- 实际结果：[20_llvm_linear_arith_and_return.txt](results/20260415_113413/20_llvm_linear_arith_and_return.txt)，与预期一致。
- 结论分析：当前非 SSA 内存式 lowering 已覆盖最基础的算术和返回路径。

### 21. `llvm_branch_and_loop_shape`

- 名称：LLVM 分支与循环形状
- 目的：验证条件跳转、循环回边和 block label 生成。
- 输入：[21_llvm_branch_and_loop_shape.md](test_cases/21_llvm_branch_and_loop_shape.md)
- 测试步骤：
  1. 运行 `./icg_probe llvm-branch-loop`。
  2. 检查 `bb_1/bb_3/bb_6` 等标签。
  3. 检查 `icmp slt` 和 `br i1`。
  4. 与 [21_llvm_branch_and_loop_shape.txt](expected/21_llvm_branch_and_loop_shape.txt) 比对。
- 预期输出：出现条件块、退出块和回跳块，控制流结构稳定。
- 实际结果：[21_llvm_branch_and_loop_shape.txt](results/20260415_113413/21_llvm_branch_and_loop_shape.txt)，与预期一致。
- 结论分析：当前 lowering 已能稳定表达 `if/while` 类控制流骨架。

### 22. `jimple_call_and_assign`

- 名称：Jimple 调用与赋值
- 目的：验证函数调用结果落到临时变量再赋值的 Jimple 输出。
- 输入：[22_jimple_call_and_assign.md](test_cases/22_jimple_call_and_assign.md)
- 测试步骤：
  1. 运行 `./icg_probe jimple-call-assign`。
  2. 检查 `.class`、`.method`、`staticinvoke` 和 `return`。
  3. 与 [22_jimple_call_and_assign.txt](expected/22_jimple_call_and_assign.txt) 比对。
- 预期输出：输出 `SeuDemo.main` 方法，声明 `a/x/t1`，并含 `t1 = staticinvoke SeuDemo.foo(a, 1);`。
- 实际结果：[22_jimple_call_and_assign.txt](results/20260415_113413/22_jimple_call_and_assign.txt)，与预期一致。
- 结论分析：Jimple 风格输出已覆盖调用、临时变量和返回值路径。

### 23. `llvm_output_stable_format`

- 名称：LLVM 输出格式稳定性
- 目的：锁定参数签名、entry 块和寄存器编号的输出形式。
- 输入：[23_llvm_output_stable_format.md](test_cases/23_llvm_output_stable_format.md)
- 测试步骤：
  1. 运行 `./icg_probe llvm-stable`。
  2. 检查 `define i32 @add(i32 %lhs.in, i32 %rhs.in)` 以及 `sum/t1` 的槽位顺序。
  3. 与 [23_llvm_output_stable_format.txt](expected/23_llvm_output_stable_format.txt) 比对。
- 预期输出：函数签名、alloca 顺序和寄存器编号保持稳定。
- 实际结果：[23_llvm_output_stable_format.txt](results/20260415_113413/23_llvm_output_stable_format.txt)，与预期一致。
- 结论分析：LLVM 文本已具备适合作为课程验收材料的稳定性。

### 24. `jimple_output_stable_format`

- 名称：Jimple 输出格式稳定性
- 目的：锁定 Jimple 类头、方法头、标签名和显式跳转形式。
- 输入：[24_jimple_output_stable_format.md](test_cases/24_jimple_output_stable_format.md)
- 测试步骤：
  1. 运行 `./icg_probe jimple-stable`。
  2. 检查 `.class public final SeuDemo`、`label_1/label_3/label_4` 与 `goto`。
  3. 与 [24_jimple_output_stable_format.txt](expected/24_jimple_output_stable_format.txt) 比对。
- 预期输出：条件块、跳转块和返回块文本顺序固定。
- 实际结果：[24_jimple_output_stable_format.txt](results/20260415_113413/24_jimple_output_stable_format.txt)，与预期一致。
- 结论分析：Jimple 文本格式当前稳定，便于文档引用和回归比对。

### 25. `llvm_invalid_target_fallback`

- 名称：LLVM 非法目标兜底块
- 目的：验证 emitter 在非法跳转目标和条件末尾 false-path 场景下会生成可落地的兜底 block。
- 输入：[25_llvm_invalid_target_fallback.md](test_cases/25_llvm_invalid_target_fallback.md)
- 测试步骤：
  1. 运行 `./icg_probe llvm-invalid-target`。
  2. 检查 `bb_invalid` 和 `bb_exit` 都被真实定义。
  3. 与 [25_llvm_invalid_target_fallback.txt](expected/25_llvm_invalid_target_fallback.txt) 比对。
- 预期输出：`br i1` 指向 `bb_invalid` / `bb_exit`，并在函数尾部补出两个兜底块。
- 实际结果：[25_llvm_invalid_target_fallback.txt](results/20260415_113413/25_llvm_invalid_target_fallback.txt)，与预期一致。
- 结论分析：LLVM emitter 不再生成未定义标签，失效路径已有稳定回落。

### 26. `jimple_invalid_target_fallback`

- 名称：Jimple 非法目标兜底块
- 目的：验证 Jimple emitter 对非法目标和条件 false-path 也会补全显式兜底标签。
- 输入：[26_jimple_invalid_target_fallback.md](test_cases/26_jimple_invalid_target_fallback.md)
- 测试步骤：
  1. 运行 `./icg_probe jimple-invalid-target`。
  2. 检查 `label_invalid` 和 `label_exit` 被显式定义。
  3. 与 [26_jimple_invalid_target_fallback.txt](expected/26_jimple_invalid_target_fallback.txt) 比对。
- 预期输出：条件语句后显式 `goto label_exit;`，函数尾部补出两个兜底标签。
- 实际结果：[26_jimple_invalid_target_fallback.txt](results/20260415_113413/26_jimple_invalid_target_fallback.txt)，与预期一致。
- 结论分析：Jimple emitter 的异常控制流输出已闭合，不再悬空。

## 3. 总体结论

- 本次为 `seuIntermediate` 新增并验证了 LLVM IR / Jimple 输出能力，`26/26` 用例全部通过。
- 额外执行的 `ctest` 官方入口测试也通过，说明脚本化测试与现有 CMake 自测一致。
- 目前未发现生产模块在既有功能范围内的行为性缺陷。

## 4. 发现的问题与结论分析

- 未发现 `intermediate/` 生产代码中的功能性失败用例。
- 当前 LLVM IR / Jimple 输出仍是课程演示级稳定文本，不包含 SSA、`phi`、指针和结构体 lowering。
- 性能测试当前属于烟雾级，不是严格基准测试；它的价值在于快速发现“明显退化或崩溃”，而不是精确性能评估。

## 5. 改进建议

- 若后续扩展 AST 支持一元运算、逻辑运算、短路求值、数组或函数参数压栈规则，应同步扩充 `icg_probe` 场景和 `expected/` 基线。
- 若后续要把 LLVM IR / Jimple 交给真实工具链，建议增加 `llvm-as/opt` 或 Soot 级别的外部校验。
- 若项目后续需要性能门禁，建议单独引入固定规模 AST 基准程序和更高精度计时。

## 6. 测试覆盖率总结

- AST 构造辅助：已覆盖
- Parse-root 交接：已覆盖
- 符号表作用域与偏移：已覆盖
- 三地址码格式化：已覆盖
- 基本块 leader 识别与后继块格式化：已覆盖
- 声明初始化、赋值、算术表达式：已覆盖
- 函数调用、条件分支、循环、返回：已覆盖
- 函数定义与作用域退出：已覆盖
- 错误路径与空输入边界：已覆盖
- CLI 自测入口：已覆盖
- 性能烟雾测试：已覆盖
- LLVM IR 输出：已覆盖
- Jimple 输出：已覆盖
- LLVM / Jimple 失效路径兜底块：已覆盖
