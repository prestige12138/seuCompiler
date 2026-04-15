# SEU Compiler 2026 答辩 PPT 内容稿

## 第 1 页：项目标题

- 项目名称：SEU Compiler 2026
- 小组成果：`seuLex + seuYacc + intermediate`
- 当前状态：三模块完成，完整链路 `Lex -> Yacc -> AST -> TAC -> LLVM IR / Jimple` 已打通

## 第 2 页：项目目标

- 自主实现 Lex 生成器
- 自主实现 Yacc 生成器
- 基于语法分析结果生成中间代码
- 输出三地址码、LLVM IR 和 Jimple
- 完成课程要求的测试、文档和可运行样例

## 第 3 页：总体架构

- `seuLex`：`.l -> scanner.cpp`
- `seuYacc`：`.y -> parser.cpp + tokens.h`
- `intermediate`：`AST -> TAC -> LLVM IR / Jimple`
- 总链路：`Lex -> Yacc -> AST -> TAC -> LLVM IR / Jimple`

## 第 4 页：Lex 模块

- 三段解析
- 扩展 RE 转换
- NFA / DFA / 最小化
- scanner 代码生成
- dot 可视化

可展示：

- `merged_nfa.dot`
- `dfa.dot`
- `min_dfa.dot`

## 第 5 页：Yacc 模块

- FIRST / FOLLOW
- LR(1) item 闭包与状态构造
- LR(1) 分析表
- LALR(1) 状态合并
- parser 与 token 头生成

## 第 6 页：中间代码模块

- `ASTBuilder`
- `SymbolTable`
- `TriAddrGenerator`
- `TargetIrEmitter`
- 三地址码、基本块、LLVM IR、Jimple 输出

## 第 7 页：关键数据结构复用

- Lex：`node / nfa / dfa`
- Yacc：`produce / ITEM / LRPDA / parse_table_item`
- Intermediate：`ASTNode / TriAddrStmt / IntermediateCode`

说明重点：

- 严格复用中期报告中的核心数据结构命名
- 没有绕成第三方 lex/yacc 工具调用

## 第 8 页：Token ABI 统一

- 早期问题：Lex 输出和 Yacc 输入接口不一致
- 收尾修复：
  - `seuYacc` 生成稳定 ABI 宏
  - `seuLex --token-header` 直接生成 parser token
  - `tokenize_for_parser(...)` 直接喂给 `yyparse(...)`

结果：

- pipeline 不再手工桥接 semantic
- 语义值通过 `yylval` 直传

## 第 9 页：最小整链路演示

演示流程：

1. 运行 `integration/tests/pipeline/run_pipeline_test.sh`
2. 生成 parser、token 头和 lexer
3. 解析输入：

```c
x = answer + 2 * 3;
return x;
```

4. 输出 TAC：

```text
1: t1 = 2 * 3
2: t2 = 42 + t1
3: x = t2
4: return x
```

## 第 10 页：完整 IR 演示

演示样例：

- `demo_ir.l`
- `demo_ir.y`
- `demo_ir.c`

展示点：

- `demo_ir.tac`
- `demo_ir.ll`
- `demo_ir.jimple`

建议重点说明：

- `inc` 和 `main` 两个函数
- `if / while / call / return`
- TAC 到 LLVM IR / Jimple 的对应关系

## 第 11 页：测试与验证

- 顶层 `ctest` 通过
- `seuLex` 模块测试通过
- `seuYacc` 模块测试通过
- `intermediate` 模块测试通过
- `intermediate` 当前专项测试 `26/26`
- 最小链路和完整 IR 链路均有正式测试报告

## 第 12 页：期末提交材料

对照 PPT 第 34 页，当前已归档：

- Lex / Yacc 输入文件
- 三模块源程序
- 生成的 lexer / parser 源程序
- 测试用例与测试报告
- 中间代码样例与 `demo_ir` 后端产物
- 实验报告
- 答辩材料内容稿

建议现场展示：

- [实践PPT核对表.md](/Users/llawliet/代码/seuCompiler/实践PPT核对表.md)
- [docs/FINAL_SUBMISSION.md](/Users/llawliet/代码/seuCompiler/docs/FINAL_SUBMISSION.md)

## 第 13 页：总结

- 项目已经达到课程提交状态
- 三模块代码、文档、测试、报告齐全
- 完整 IR 演示已补齐
- 后续扩展方向明确：
  - 更完整 C 子集语义
  - 真正 SSA/优化级 LLVM
  - 更大规模文法联调
