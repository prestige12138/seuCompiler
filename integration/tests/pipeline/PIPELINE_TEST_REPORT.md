# Pipeline Test Report

该测试用于验证最小整链路：

`seuLex` 生成 scanner -> `seuYacc` 生成 parser -> `intermediate` 释放 AST 根并生成三地址码。

当前脚本：[run_pipeline_test.sh](run_pipeline_test.sh)

当前用例：

- Lex 规格：[pipeline_expr.l](test_cases/pipeline_expr.l)
- Yacc 规格：[pipeline_expr.y](test_cases/pipeline_expr.y)
- 预期 IR：[pipeline_expr.ir](expected/pipeline_expr.ir)
- 完整 IR 导出演示另见：[../ir_pipeline/IR_PIPELINE_TEST_REPORT.md](../ir_pipeline/IR_PIPELINE_TEST_REPORT.md)

当前验证目标：

1. 生成的 parser 不写入仓库绝对 include 路径。
2. 生成的 lexer 提供可直接桥接到 parser `Token` 的详细 token 输出接口。
3. `.y` 语义动作能够构 AST 并通过 `releaseParseRoot()` 交给 `intermediate`。
4. 最终 IR 与预期完全一致。

最新通过结果目录：

- [results/20260416_005216](/Users/llawliet/代码/seuCompiler/integration/tests/pipeline/results/20260416_005216)
- 当前指针：[results/LATEST.txt](results/LATEST.txt)
- 最新输出：[pipeline_expr.ir](/Users/llawliet/代码/seuCompiler/integration/tests/pipeline/results/20260416_005216/pipeline_expr.ir)
