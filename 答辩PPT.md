# SEU Compiler 2026 答辩 PPT 内容稿

## 第 1 页：项目标题

- 项目名称：SEU Compiler 2026
- 小组成果：`seuLex + seuYacc + intermediate`
- 当前状态：三模块完成，最小整链路联通

## 第 2 页：项目目标

- 自主实现 Lex 生成器
- 自主实现 Yacc 生成器
- 基于语法分析结果生成中间代码
- 完成课程要求的测试、文档和可运行样例

## 第 3 页：总体架构

- `seuLex`：`.l -> scanner.cpp`
- `seuYacc`：`.y -> parser.cpp + tokens.h`
- `intermediate`：`AST -> TAC -> Basic Blocks`
- 总链路：`Lex -> Yacc -> AST -> IR`

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
- 三地址码格式化输出
- 基本块划分与后继块视图

## 第 7 页：关键数据结构复用

- Lex：`node / nfa / dfa`
- Yacc：`produce / ITEM / LRPDA / parse_table_item`
- Intermediate：`ASTNode / TriAddrStmt / IntermediateCode`

说明重点：

- 严格复用中期报告中的核心数据结构命名
- 没有把课程要求绕开成第三方工具调用

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

4. 输出 IR：

```text
1: t1 = 2 * 3
2: t2 = 42 + t1
3: x = t2
4: return x
```

## 第 10 页：测试与验证

- 顶层 `ctest` 通过
- `seuLex` 模块测试通过
- `seuYacc` 模块测试通过
- `intermediate` 模块测试通过
- `intermediate` 新增基本块专项测试 6 条

## 第 11 页：完成度与边界

已完成：

- Lex 全链路
- Yacc 全链路
- AST / TAC / Basic Blocks
- 最小整链路联通

边界：

- 当前尚未扩展到 LLVM IR / Jimple
- 更完整 C99 语义仍可继续增强
- 生成器默认信任 `.l/.y` 用户代码

## 第 12 页：总结

- 项目已经达到课程提交状态
- 三模块代码、文档、测试、报告齐全
- 后续扩展方向明确：
  - 更大文法联调
  - LLVM IR / Jimple
  - 更完整语义分析
