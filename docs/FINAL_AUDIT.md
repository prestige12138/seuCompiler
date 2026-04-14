# 终审摘要

## 已修复

- 修复 `seuYacc` 生成代码写入绝对 include 路径的问题
- 为 `seuLex` 增加 `SeuLexToken`、`tokenize_detailed(...)` 与 ABI 模式 `tokenize_for_parser(...)`
- 为 `seuLex` 增加 `yytext`、`yylineno`、`column` 运行时契约
- 为 `seuYacc` 生成 token 头补稳定 ABI 导出宏
- 增加最小整链路测试：`Lex -> Yacc -> AST -> IR`
- 为 `intermediate` 增加基本块划分与格式化输出
- 增加顶层 `CMakeLists.txt`
- 同步根 README 与三模块 README
- 为 `intermediate` 补齐 `docs/` 文档骨架

## 仍明确保留的边界

- parser 仍以 `bool yyparse(const std::vector<Token>&)` 为主接口，没有升级为返回 AST 的 `ParseResult`
- `seuIntermediate` 仍通过 `setParseRoot/releaseParseRoot` 交接 AST
- 课程级大文法 `c99` 当前已有生成与编译回归，但整链路联通优先使用教学小文法样例
- 三模块当前默认信任 `.l` / `.y` 中的用户动作与嵌入代码；生成器会原样落盘，测试会继续编译并执行这些产物，因此当前结论仅适用于可信输入场景
- 当前没有继续扩展到 LLVM IR / Jimple；中间代码仍以三地址码和基本块视图为主

## 验收重点

- 三模块可独立构建与测试
- 顶层可统一构建与测试
- Lex -> Yacc 已统一到稳定 token ABI
- 仓库内存在正式整链路样例，不再只停留在文档级“可拼接”

## 本轮验证结果

- 顶层 `cmake -S . -B build && ctest --test-dir build --output-on-failure` 通过
- `seuLex/tests/lex/run_lex_tests.sh` 通过
- `seuYacc/tests/yacc/run_yacc_tests.sh` 通过
- `intermediate/tests/icg/run_icg_tests.sh` 通过
- 已确认最小整链路样例 `Lex -> Yacc -> AST -> IR` 可稳定运行
