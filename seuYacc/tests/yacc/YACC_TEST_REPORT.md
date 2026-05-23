# seuYacc 测试报告

## 1. 测试概述与覆盖范围

- 测试对象：`seuYacc` 模块的输入解析、文法处理、LR(1)/LALR(1) 自动机构造、分析表生成、代码生成与运行时行为。
- 执行脚本：[run_yacc_tests.sh](run_yacc_tests.sh)
- 辅助探针：[yacc_probe.cpp](yacc_probe.cpp)
- 本次实际运行结果目录：[results/20260415_220247](results/20260415_220247)
- 汇总结果：[SUMMARY.md](results/20260415_220247/SUMMARY.md)
- 实际执行结果：共 `18` 条测试，`18` 条通过，`0` 条失败。

本套测试覆盖了以下能力面：

- `.y` 三段式输入解析：Definitions / Rules / User subroutines
- `%union`、`%token`、`%type`、`%start`、`%left/%right/%nonassoc`、`%prec`、mid-rule action
- FIRST / FOLLOW 集构造
- LR(1) item 闭包与 lookahead 传播
- canonical LR(1) PDA 构造
- direct LALR(1) 与 `LR(1) -> LALR(1)` 合并结果对比
- ACTION / GOTO 表构造、优先级与结合性消解、reduce/reduce 冲突选择
- 语义动作翻译、用户代码嵌入、生成 parser 的编译与运行
- 词法接口契约：named token 与 quoted char token 混合输入
- 错误输入失败路径、`resources/minic.y` 回归、内建 `--self-test`

当前实现的两个已知边界也被明确纳入测试口径：

- 当前生成 parser 没有真正的错误恢复机制，测试按“稳定返回 `false` / 明确报错”校验，而不是“恢复继续分析”。
- 运行期符号表内部内容未对外暴露，因此运行期作用域仅能通过“不崩溃 + 构造链路可运行”间接验证；显式作用域行为由 `SymbolTableManager` 单独覆盖。

## 2. 详细测试记录

### 01. `parse_sections_basic`

- 名称：基础三段式 Yacc 文件解析
- 目的：验证 Definitions / Rules / User subroutines 三段拆分与基础统计正确。
- 输入：[01_parse_sections_basic.y](test_cases/01_parse_sections_basic.y)
- 测试步骤：
  1. 用 `yacc_probe parse-summary` 解析输入文件。
  2. 输出起始符、token 数、规则数、是否含 verbatim/user code 等摘要。
  3. 与基线文件 [01_parse_sections_basic.txt](expected/01_parse_sections_basic.txt) 比对。
- 预期输出：`start=translation_unit`，`rules=2`，`verbatim=yes`，`user_code=yes`。
- 实际结果：[01_parse_sections_basic.txt](results/20260415_220247/01_parse_sections_basic.txt)，输出与预期完全一致。
- 结论分析：基础三段切分稳定，后续生成链依赖的源文件分段前提成立。

### 02. `parse_union_precedence_midrule`

- 名称：复杂指令解析与 mid-rule action 转换
- 目的：验证 `%union`、typed token / nonterminal、优先级声明和 mid-rule action 合成规则。
- 输入：[02_parse_union_precedence_midrule.y](test_cases/02_parse_union_precedence_midrule.y)
- 测试步骤：
  1. 用 `yacc_probe parse-summary` 解析复杂文法。
  2. 检查 typed token、typed nonterminal、precedence group 与 synthetic midrule rule 数量。
  3. 与 [02_parse_union_precedence_midrule.txt](expected/02_parse_union_precedence_midrule.txt) 比对。
- 预期输出：`typed_tokens=1`，`typed_nonterminals=1`，`precedence_groups=2`，`midrule_rules=1`。
- 实际结果：[02_parse_union_precedence_midrule.txt](results/20260415_220247/02_parse_union_precedence_midrule.txt)，规则总数为 `4`，mid-rule 合成数量为 `1`。
- 结论分析：复杂定义区和 mid-rule action 转换路径工作正常，可支撑后续语义动作生成。

### 03. `first_follow_expr`

- 名称：表达式文法 FIRST / FOLLOW
- 目的：验证典型表达式文法的 FIRST / FOLLOW 集构造。
- 输入：[03_first_follow_expr.y](test_cases/03_first_follow_expr.y)
- 测试步骤：
  1. 加载文法并填充 report 规定全局表。
  2. 调用 `LR1Builder::computeFirstSets` 与 `computeFollowSets`。
  3. 输出 `E`、`T`、`F` 的 FIRST / FOLLOW。
  4. 与 [03_first_follow_expr.txt](expected/03_first_follow_expr.txt) 比对。
- 预期输出：`FIRST(E)={'(',ID}`，`FOLLOW(E)={$,')','+'}` 等。
- 实际结果：[03_first_follow_expr.txt](results/20260415_220247/03_first_follow_expr.txt)，全部集合匹配。
- 结论分析：FIRST / FOLLOW 在非空表达式文法上行为正确。

### 04. `first_follow_nullable_chain`

- 名称：含空产生式链的 FIRST / FOLLOW
- 目的：验证 epsilon 传播与 FOLLOW 逆向传播。
- 输入：[04_first_follow_nullable_chain.y](test_cases/04_first_follow_nullable_chain.y)
- 测试步骤：
  1. 加载含空产生式的链式文法。
  2. 输出 `S`、`A`、`B` 的 FIRST / FOLLOW。
  3. 与 [04_first_follow_nullable_chain.txt](expected/04_first_follow_nullable_chain.txt) 比对。
- 预期输出：`FIRST(S)={'a','b',<epsilon>}`，`FOLLOW(A)={$,'b'}`。
- 实际结果：[04_first_follow_nullable_chain.txt](results/20260415_220247/04_first_follow_nullable_chain.txt)，epsilon 与 FOLLOW 传播均符合预期。
- 结论分析：nullable chain 的集合传播实现是可用的。

### 05. `closure_lookahead`

- 名称：LR(1) 闭包与预测符传播
- 目的：验证初始状态闭包中 lookahead 的传播不是仅扩核心项。
- 输入：[05_closure_lookahead.y](test_cases/05_closure_lookahead.y)
- 测试步骤：
  1. 构造 canonical LR(1) PDA。
  2. 读取状态 `0` 的所有 item。
  3. 与 [05_closure_lookahead.txt](expected/05_closure_lookahead.txt) 比对。
- 预期输出：`A -> . B , 'c'`、`B -> . , 'c'`、`B -> . 'b' , 'c'` 等项均存在。
- 实际结果：[05_closure_lookahead.txt](results/20260415_220247/05_closure_lookahead.txt)，状态 `0` 共 `5` 个 item，lookahead 全部正确。
- 结论分析：closure 的预测符传播正确，不是 LR(0) 级别的“只扩核不扩预测符”。

### 06. `direct_lalr_vs_merge_lalr`

- 名称：direct LALR 与 merged LALR 等价性
- 目的：对比 direct LALR 构造与 canonical LR(1) 合并到 LALR 的结果。
- 输入：[03_first_follow_expr.y](test_cases/03_first_follow_expr.y)
- 测试步骤：
  1. 构造 canonical LR(1) PDA。
  2. 通过 `LALRConverter` 合并得到 merged LALR。
  3. 直接构造 direct LALR。
  4. 比较状态数、冲突数、序列化表内容。
  5. 与 [06_direct_lalr_vs_merge_lalr.txt](expected/06_direct_lalr_vs_merge_lalr.txt) 比对。
- 预期输出：`canonical_states=22`，`merged_lalr_states=12`，`direct_lalr_states=12`，`table_equal=yes`。
- 实际结果：[06_direct_lalr_vs_merge_lalr.txt](results/20260415_220247/06_direct_lalr_vs_merge_lalr.txt)，两条路径状态数一致，分析表等价。
- 结论分析：direct LALR 与 merge 路径在当前表达式文法上保持一致，可作为回归基线。

### 07. `precedence_left_assoc_runtime`

- 名称：左结合优先级运行时验证
- 目的：验证 `%left` 消解 shift/reduce 冲突为左结合。
- 输入：[06_precedence_left_assoc.y](test_cases/06_precedence_left_assoc.y)
- 测试步骤：
  1. 调用 `seuYacc` 生成 parser。
  2. 编译临时 driver。
  3. 输入 token 序列 `10 - 3 - 2`。
  4. 读取 `yyparse` 结果与 `get_assoc_value()`。
  5. 与 [07_precedence_left_assoc_runtime.txt](expected/07_precedence_left_assoc_runtime.txt) 比对。
- 预期输出：`parse=true`，`value=5`。
- 实际结果：[07_precedence_left_assoc_runtime.txt](results/20260415_220247/07_precedence_left_assoc_runtime.txt)，解析成功，结果为 `5`。
- 结论分析：左结合冲突消解与生成 parser 的运行时语义一致。

### 08. `precedence_nonassoc_reject`

- 名称：非结合优先级拒绝路径
- 目的：验证 `%nonassoc` 在同优先级二义输入上给出拒绝。
- 输入：[07_precedence_nonassoc.y](test_cases/07_precedence_nonassoc.y)
- 测试步骤：
  1. 生成并编译 parser。
  2. 输入 `NUM < NUM < NUM`。
  3. 观察 `yyparse` 返回值。
  4. 与 [08_precedence_nonassoc_reject.txt](expected/08_precedence_nonassoc_reject.txt) 比对。
- 预期输出：`parse=false`。
- 实际结果：[08_precedence_nonassoc_reject.txt](results/20260415_220247/08_precedence_nonassoc_reject.txt)，解析稳定返回 `false`。
- 结论分析：非结合冲突被正确归约为错误动作，而不是默认 shift。

### 09. `reduce_reduce_resolution`

- 名称：reduce/reduce 冲突选择
- 目的：验证 reduce/reduce 冲突记录与较小 production index 保留策略。
- 输入：[08_reduce_reduce_order.y](test_cases/08_reduce_reduce_order.y)
- 测试步骤：
  1. 构造 LALR 分析表并收集冲突。
  2. 统计冲突类型。
  3. 提取首个冲突的最终解析动作。
  4. 与 [09_reduce_reduce_resolution.txt](expected/09_reduce_reduce_resolution.txt) 比对。
- 预期输出：`reduce_reduce=1`，`resolved_action=r2`。
- 实际结果：[09_reduce_reduce_resolution.txt](results/20260415_220247/09_reduce_reduce_resolution.txt)，冲突发生在 state `4`，最终保留 `r2`。
- 结论分析：reduce/reduce 冲突处理与实现策略一致。

### 10. `symbol_table_scope_shadowing`

- 名称：语义符号表作用域遮蔽
- 目的：验证 `SymbolTableManager` 的 enter / exit scope、遮蔽与恢复。
- 输入：[09_symbol_table_scope.y](test_cases/09_symbol_table_scope.y)
- 测试步骤：
  1. 构造 `SymbolTableManager`。
  2. 在外层作用域声明 `x:int`、`y:float`。
  3. 在内层作用域重新声明 `x:char`。
  4. 依次检查内层、退出内层、退出全部后的查询结果。
  5. 与 [10_symbol_table_scope_shadowing.txt](expected/10_symbol_table_scope_shadowing.txt) 比对。
- 预期输出：`outer_x=int`，`inner_x=char`，`after_exit_x=int`，最终 `missing=yes`。
- 实际结果：[10_symbol_table_scope_shadowing.txt](results/20260415_220247/10_symbol_table_scope_shadowing.txt)，所有查找结果符合预期。
- 结论分析：语义作用域栈行为正确，可作为后续语义分析的稳定依赖。

### 11. `lex_token_contract_accept`

- 名称：Lex 集成 token 契约正例
- 目的：验证 generated parser 对 named token 与 quoted char token 的混合输入契约。
- 输入：[10_generated_parser_contract.y](test_cases/10_generated_parser_contract.y)
- 测试步骤：
  1. 生成并编译 parser。
  2. 手工构造 token 序列 `('(' ID '+' ID ')' '*' ID)`。
  3. 调用 `yyparse`。
  4. 与 [11_lex_token_contract_accept.txt](expected/11_lex_token_contract_accept.txt) 比对。
- 预期输出：`parse=true`。
- 实际结果：[11_lex_token_contract_accept.txt](results/20260415_220247/11_lex_token_contract_accept.txt)，解析成功。
- 结论分析：token code 与 quoted char terminal 的集成契约成立。

### 12. `generated_parse_failure`

- 名称：生成 parser 的拒绝路径
- 目的：验证非法 token 序列时 generated parser 稳定失败。
- 输入：[10_generated_parser_contract.y](test_cases/10_generated_parser_contract.y)
- 测试步骤：
  1. 生成并编译 parser。
  2. 构造错误输入 `ID '+' '*' ID`。
  3. 调用 `yyparse`。
  4. 与 [12_generated_parse_failure.txt](expected/12_generated_parse_failure.txt) 比对。
- 预期输出：`parse=false`。
- 实际结果：[12_generated_parse_failure.txt](results/20260415_220247/12_generated_parse_failure.txt)，返回 `false`。
- 结论分析：当前实现没有恢复逻辑，但拒绝行为稳定且可预期。

### 13. `generated_semantic_actions_and_user_code`

- 名称：语义动作翻译与用户代码嵌入
- 目的：验证 `$$`、`$1`、`$<tag>1` 翻译，以及 `%{ %}` / 第三段用户代码嵌入。
- 输入：[11_generated_semantic_actions.y](test_cases/11_generated_semantic_actions.y)
- 测试步骤：
  1. 生成并编译 parser。
  2. 输入 `1 + 2 + 4`。
  3. 调用 `yyparse` 并读取用户函数 `read_semantic_total()`。
  4. 与 [13_generated_semantic_actions_and_user_code.txt](expected/13_generated_semantic_actions_and_user_code.txt) 比对。
- 预期输出：`parse=true`，`semantic_total=7`。
- 实际结果：[13_generated_semantic_actions_and_user_code.txt](results/20260415_220247/13_generated_semantic_actions_and_user_code.txt)，解析成功，语义结果为 `7`。
- 结论分析：语义动作翻译路径、用户代码嵌入路径均已打通。

### 14. `mode_lr1_generation_compile`

- 名称：`lr1` 模式生成与编译
- 目的：验证 `seuYacc <file> ... lr1` 模式能独立完成生成与编译。
- 输入：[12_mode_lr1_generation.y](test_cases/12_mode_lr1_generation.y)
- 测试步骤：
  1. 使用 `lr1` 模式生成 parser。
  2. 将生成的 `parser.cpp` 编译为目标文件。
  3. 与 [14_mode_lr1_generation_compile.txt](expected/14_mode_lr1_generation_compile.txt) 比对。
- 预期输出：`generate_exit=0`，`compile_exit=0`，`mode=lr1`。
- 实际结果：[14_mode_lr1_generation_compile.txt](results/20260415_220247/14_mode_lr1_generation_compile.txt)，全部通过。
- 结论分析：`lr1` 模式可独立工作，不依赖默认 `lalr` 路径。

### 15. `error_missing_delimiters`

- 名称：缺失第二个 `%%` 的输入错误
- 目的：验证 `.y` 文件结构不完整时给出明确错误。
- 输入：[13_error_missing_delimiters.y](test_cases/13_error_missing_delimiters.y)
- 测试步骤：
  1. 直接调用 `seuYacc` 处理非法输入。
  2. 捕获退出码、stdout、stderr。
  3. 与 [15_error_missing_delimiters.txt](expected/15_error_missing_delimiters.txt) 比对。
- 预期输出：`exit_code=1`，stderr 为 `seuYacc error: missing second %% section delimiter`。
- 实际结果：[15_error_missing_delimiters.txt](results/20260415_220247/15_error_missing_delimiters.txt)，与预期完全一致。
- 结论分析：基础结构错误提示明确，便于定位。

### 16. `error_unterminated_action`

- 名称：未闭合 action block 错误
- 目的：验证 grammar section 中未闭合动作块的失败路径。
- 输入：[14_error_unterminated_action.y](test_cases/14_error_unterminated_action.y)
- 测试步骤：
  1. 直接调用 `seuYacc` 处理非法输入。
  2. 捕获退出码、stdout、stderr。
  3. 与 [16_error_unterminated_action.txt](expected/16_error_unterminated_action.txt) 比对。
- 预期输出：`exit_code=1`，stderr 为 `seuYacc error: unterminated action block in grammar section`。
- 实际结果：[16_error_unterminated_action.txt](results/20260415_220247/16_error_unterminated_action.txt)，与预期一致。
- 结论分析：action 解析器的错误边界清晰。

### 17. `resource_minic_generation_regression`

- 名称：`resources/minic.y` 回归生成
- 目的：验证当前 `minic-plus` 主规格文法仍可完成生成并编译成目标文件。
- 输入：`resources/minic.y`
- 测试步骤：
  1. 用 `lalr` 模式生成 `minic` parser。
  2. 仅编译生成的 `parser.cpp` 到目标文件。
  3. 与 [17_resource_minic_generation_regression.txt](expected/17_resource_minic_generation_regression.txt) 比对。
- 预期输出：`generate_exit=0`，`compile_exit=0`。
- 实际结果：[17_resource_minic_generation_regression.txt](results/20260415_220247/17_resource_minic_generation_regression.txt)，均成功。
- 结论分析：对当前子集主规格文法的回归能力正常，没有因本轮删减而暴露生成回退。

### 18. `cli_self_test`

- 名称：内建自测命令
- 目的：验证 `seuYacc --self-test` 仍可完整跑通。
- 输入：CLI 命令 `seuYacc --self-test`
- 测试步骤：
  1. 执行内建自测。
  2. 归一化输出，仅保留是否包含 sample / semantic / minic 三条自测完成标志。
  3. 与 [18_cli_self_test.txt](expected/18_cli_self_test.txt) 比对。
- 预期输出：`exit_code=0`，`has_sample=yes`，`has_semantic=yes`，`has_minic=yes`。
- 实际结果：[18_cli_self_test.txt](results/20260415_220247/18_cli_self_test.txt)，全部满足。
- 结论分析：项目自带 smoke test 仍然可用，与外部测试套件互为补充。

## 3. 总体结论

- 本次测试共执行 `18` 项，全部通过。
- 解析层、自动机构造层、分析表层、代码生成层和 CLI 层均有独立覆盖，不是单一 smoke test。
- `resources/minic.y` 回归与 `--self-test` 同时通过，说明当前 `seuYacc` 在 `minic-plus` 子集口径下具备较好的端到端稳定性。

## 4. 发现的问题

- 未发现阻塞交付的功能性缺陷。
- 当前“错误恢复”能力仍然是实现边界，不支持传统 Yacc `error` token 风格的恢复流；现阶段测试只能验证“稳定失败”，不能验证“恢复成功”。
- 运行期符号表内部状态缺少观测接口，因此运行期作用域管理还缺少更细粒度的黑盒断言点。

## 5. 改进建议

- 若后续实现 `error` token、`yyerrok`、`yyclearin` 等恢复语义，应新增专门恢复用例，不应继续沿用“失败即通过”的口径。
- 若希望更深入验证运行期符号表，建议在测试构建模式下暴露只读调试接口，便于断言 `{` / `}` 触发的作用域变化。
- 当前报告已不再保留大文法性能基线；如果后续重新引入性能项，建议单独维护为独立基准报告，而不是混在当前子集回归里。

## 6. 测试覆盖率总结

- 能力覆盖率：`18 / 18` 计划场景已执行并通过。
- 分类覆盖：
  - 输入解析：4 项
  - 文法集合与闭包：3 项
  - LR/LALR 与冲突处理：4 项
  - 代码生成与运行时：5 项
  - 错误处理与回归：2 项
- 代码行覆盖率：当前项目未配置 gcov / llvm-cov / CTest 覆盖率采集，因此无法提供真实行覆盖率百分比；本报告仅给出能力覆盖率与实际运行结果。

## 7. Yacc-A 现场 CLI 演示操作记录

本节记录可现场复现的 CLI 操作。操作 01-14 展示 Yacc-A 重点路径，操作 15-27 补充展示其余 Yacc 测试结果，确保 18 个测试用例在运行层面和结果展示层面均有覆盖。ACTION/GOTO 分析表与 parser 代码生成属于 Yacc-B，在本节中仅作为完整测试套件结果展示。

### 操作 01：进入项目根目录

```bash
cd /Users/llawliet/代码/seuCompiler
```

- 目的：确保后续相对路径均从仓库根目录解析。
- 结果路径：无文件输出。

### 操作 02：确认当前分支

```bash
git branch --show-current
```

- 预期输出：当前用于答辩的分支名，例如 `minic-plus`。
- 结果路径：终端标准输出。

### 操作 03：查看 Yacc-A 相关测试输入清单

```bash
ls seuYacc/tests/yacc/test_cases/0*.y
```

- 预期输出：至少包含 `01_parse_sections_basic.y`、`02_parse_union_precedence_midrule.y`、`03_first_follow_expr.y`、`04_first_follow_nullable_chain.y`、`05_closure_lookahead.y`。
- 结果路径：`seuYacc/tests/yacc/test_cases/`

### 操作 04：展示 `.y` 三段解析输入

```bash
sed -n '1,120p' seuYacc/tests/yacc/test_cases/01_parse_sections_basic.y
```

- 预期输出：显示 definitions、rules、user subroutines 三段结构。
- 结果路径：`seuYacc/tests/yacc/test_cases/01_parse_sections_basic.y`

### 操作 05：展示声明、优先级与 mid-rule action 输入

```bash
sed -n '1,160p' seuYacc/tests/yacc/test_cases/02_parse_union_precedence_midrule.y
```

- 预期输出：显示 `%union`、typed token、typed nonterminal、优先级声明与 mid-rule action。
- 结果路径：`seuYacc/tests/yacc/test_cases/02_parse_union_precedence_midrule.y`

### 操作 06：展示 closure lookahead 输入

```bash
sed -n '1,160p' seuYacc/tests/yacc/test_cases/05_closure_lookahead.y
```

- 预期输出：显示用于验证 LR(1) closure lookahead 传播的文法。
- 结果路径：`seuYacc/tests/yacc/test_cases/05_closure_lookahead.y`

### 操作 07：构建 seuYacc

```bash
cmake -S seuYacc -B seuYacc/build
cmake --build seuYacc/build -j
```

- 预期输出：构建成功，终端出现 `Built target seuYacc`。
- 结果路径：`seuYacc/build/seuYacc`

### 操作 08：运行完整 Yacc 测试

```bash
bash seuYacc/tests/yacc/run_yacc_tests.sh
```

- 预期输出：`Total: 18`、`Passed: 18`、`Failed: 0`，并打印 `Result directory`。
- 结果路径：`seuYacc/tests/yacc/results/<时间戳>/`

### 操作 09：记录本次结果目录

```bash
RESULT_DIR=seuYacc/tests/yacc/results/<时间戳>
```

- 说明：将 `<时间戳>` 替换为操作 08 输出的实际目录名。
- 示例：`RESULT_DIR=seuYacc/tests/yacc/results/20260519_185751`
- 结果路径：`$RESULT_DIR`

### 操作 10：展示测试汇总

```bash
cat "$RESULT_DIR/SUMMARY.md"
```

- 预期输出：18 个测试条目均为 `PASS`。
- 结果路径：`$RESULT_DIR/SUMMARY.md`

### 操作 11：展示三段解析实际结果与预期结果

```bash
cat "$RESULT_DIR/01_parse_sections_basic.txt"
cat seuYacc/tests/yacc/expected/01_parse_sections_basic.txt
```

- 预期输出：实际结果与预期结果一致。
- 实际结果路径：`$RESULT_DIR/01_parse_sections_basic.txt`
- 预期结果路径：`seuYacc/tests/yacc/expected/01_parse_sections_basic.txt`

### 操作 12：展示 `%union`、优先级与 mid-rule action 结果

```bash
cat "$RESULT_DIR/02_parse_union_precedence_midrule.txt"
cat seuYacc/tests/yacc/expected/02_parse_union_precedence_midrule.txt
```

- 预期输出：`typed_tokens=1`、`typed_nonterminals=1`、`precedence_groups=2`、`midrule_rules=1` 等字段匹配。
- 实际结果路径：`$RESULT_DIR/02_parse_union_precedence_midrule.txt`
- 预期结果路径：`seuYacc/tests/yacc/expected/02_parse_union_precedence_midrule.txt`

### 操作 13：展示 FIRST / FOLLOW 与 nullable chain 结果

```bash
cat "$RESULT_DIR/03_first_follow_expr.txt"
cat "$RESULT_DIR/04_first_follow_nullable_chain.txt"
```

- 预期输出：表达式文法 FIRST/FOLLOW 正确；nullable chain 中 `<epsilon>` 与 FOLLOW 传播结果正确。
- 结果路径：
  - `$RESULT_DIR/03_first_follow_expr.txt`
  - `$RESULT_DIR/04_first_follow_nullable_chain.txt`

### 操作 14：展示 closure lookahead 与 LR(1) 模式结果

```bash
cat "$RESULT_DIR/05_closure_lookahead.txt"
cat "$RESULT_DIR/14_mode_lr1_generation_compile.txt"
```

- 预期输出：
  - `05_closure_lookahead.txt` 中包含带正确 lookahead 的 LR(1) item。
  - `14_mode_lr1_generation_compile.txt` 中包含 `generate_exit=0`、`compile_exit=0`、`mode=lr1`。
- 结果路径：
  - `$RESULT_DIR/05_closure_lookahead.txt`
  - `$RESULT_DIR/14_mode_lr1_generation_compile.txt`

### 操作 15：展示 direct LALR 与 merged LALR 对比结果

```bash
cat "$RESULT_DIR/06_direct_lalr_vs_merge_lalr.txt"
cat seuYacc/tests/yacc/expected/06_direct_lalr_vs_merge_lalr.txt
```

- 预期输出：direct LALR 与 canonical LR(1) 合并后的 LALR 在状态数和表内容上匹配。
- 实际结果路径：`$RESULT_DIR/06_direct_lalr_vs_merge_lalr.txt`
- 预期结果路径：`seuYacc/tests/yacc/expected/06_direct_lalr_vs_merge_lalr.txt`

### 操作 16：展示左结合优先级运行时结果

```bash
cat "$RESULT_DIR/07_precedence_left_assoc_runtime.txt"
cat seuYacc/tests/yacc/expected/07_precedence_left_assoc_runtime.txt
```

- 预期输出：`parse=true`，`value=5`。
- 实际结果路径：`$RESULT_DIR/07_precedence_left_assoc_runtime.txt`
- 预期结果路径：`seuYacc/tests/yacc/expected/07_precedence_left_assoc_runtime.txt`

### 操作 17：展示 nonassoc 拒绝结果

```bash
cat "$RESULT_DIR/08_precedence_nonassoc_reject.txt"
cat seuYacc/tests/yacc/expected/08_precedence_nonassoc_reject.txt
```

- 预期输出：`parse=false`。
- 实际结果路径：`$RESULT_DIR/08_precedence_nonassoc_reject.txt`
- 预期结果路径：`seuYacc/tests/yacc/expected/08_precedence_nonassoc_reject.txt`

### 操作 18：展示 reduce/reduce 冲突记录结果

```bash
cat "$RESULT_DIR/09_reduce_reduce_resolution.txt"
cat seuYacc/tests/yacc/expected/09_reduce_reduce_resolution.txt
```

- 预期输出：包含 reduce/reduce 冲突统计与最终保留动作。
- 实际结果路径：`$RESULT_DIR/09_reduce_reduce_resolution.txt`
- 预期结果路径：`seuYacc/tests/yacc/expected/09_reduce_reduce_resolution.txt`

### 操作 19：展示符号表作用域遮蔽结果

```bash
cat "$RESULT_DIR/10_symbol_table_scope_shadowing.txt"
cat seuYacc/tests/yacc/expected/10_symbol_table_scope_shadowing.txt
```

- 预期输出：外层 `x`、内层 `x`、退出作用域后的 `x` 查询结果符合遮蔽与恢复规则。
- 实际结果路径：`$RESULT_DIR/10_symbol_table_scope_shadowing.txt`
- 预期结果路径：`seuYacc/tests/yacc/expected/10_symbol_table_scope_shadowing.txt`

### 操作 20：展示 Lex token 契约正例结果

```bash
cat "$RESULT_DIR/11_lex_token_contract_accept.txt"
cat seuYacc/tests/yacc/expected/11_lex_token_contract_accept.txt
```

- 预期输出：`parse=true`。
- 实际结果路径：`$RESULT_DIR/11_lex_token_contract_accept.txt`
- 预期结果路径：`seuYacc/tests/yacc/expected/11_lex_token_contract_accept.txt`

### 操作 21：展示 generated parser 拒绝路径结果

```bash
cat "$RESULT_DIR/12_generated_parse_failure.txt"
cat seuYacc/tests/yacc/expected/12_generated_parse_failure.txt
```

- 预期输出：`parse=false`。
- 实际结果路径：`$RESULT_DIR/12_generated_parse_failure.txt`
- 预期结果路径：`seuYacc/tests/yacc/expected/12_generated_parse_failure.txt`

### 操作 22：展示语义动作与用户代码结果

```bash
cat "$RESULT_DIR/13_generated_semantic_actions_and_user_code.txt"
cat seuYacc/tests/yacc/expected/13_generated_semantic_actions_and_user_code.txt
```

- 预期输出：`parse=true`，`semantic_total=7`。
- 实际结果路径：`$RESULT_DIR/13_generated_semantic_actions_and_user_code.txt`
- 预期结果路径：`seuYacc/tests/yacc/expected/13_generated_semantic_actions_and_user_code.txt`

### 操作 23：展示缺失分隔符错误结果

```bash
cat "$RESULT_DIR/15_error_missing_delimiters.txt"
cat seuYacc/tests/yacc/expected/15_error_missing_delimiters.txt
```

- 预期输出：`exit_code=1`，stderr 包含缺失第二个 `%%` 的错误信息。
- 实际结果路径：`$RESULT_DIR/15_error_missing_delimiters.txt`
- 预期结果路径：`seuYacc/tests/yacc/expected/15_error_missing_delimiters.txt`

### 操作 24：展示未闭合 action block 错误结果

```bash
cat "$RESULT_DIR/16_error_unterminated_action.txt"
cat seuYacc/tests/yacc/expected/16_error_unterminated_action.txt
```

- 预期输出：`exit_code=1`，stderr 包含未闭合 action block 的错误信息。
- 实际结果路径：`$RESULT_DIR/16_error_unterminated_action.txt`
- 预期结果路径：`seuYacc/tests/yacc/expected/16_error_unterminated_action.txt`

### 操作 25：展示 resources/minic.y 回归生成结果

```bash
cat "$RESULT_DIR/17_resource_minic_generation_regression.txt"
cat seuYacc/tests/yacc/expected/17_resource_minic_generation_regression.txt
```

- 预期输出：`generate_exit=0`，`compile_exit=0`。
- 实际结果路径：`$RESULT_DIR/17_resource_minic_generation_regression.txt`
- 预期结果路径：`seuYacc/tests/yacc/expected/17_resource_minic_generation_regression.txt`

### 操作 26：展示 CLI 内建自测结果

```bash
cat "$RESULT_DIR/18_cli_self_test.txt"
cat seuYacc/tests/yacc/expected/18_cli_self_test.txt
```

- 预期输出：`exit_code=0`，`has_sample=yes`，`has_semantic=yes`，`has_minic=yes`。
- 实际结果路径：`$RESULT_DIR/18_cli_self_test.txt`
- 预期结果路径：`seuYacc/tests/yacc/expected/18_cli_self_test.txt`

### 操作 27：检查 18 个测试结果文件是否齐全

```bash
ls "$RESULT_DIR"/*.txt | wc -l
ls "$RESULT_DIR"/*.txt | sort
```

- 预期输出：第一条命令输出 `18`；第二条命令列出 `01` 到 `18` 的实际结果文件。
- 结果路径：`$RESULT_DIR/*.txt`
