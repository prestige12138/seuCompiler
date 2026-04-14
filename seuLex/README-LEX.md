# seuLex

模块化的 `seuLex` 词法分析器生成器实现，保持中期报告要求的数据结构名不变：

- `node`
- `nfa`
- `dfa`
- `idreTable`
- `nfaTable`
- `dfaterminals`
- `TerStateActionTable`
- `mindfareturn`
- `char_set`
- `mulit`
- `myMul`

这些名字统一位于 `namespace seu_lex` 中，避免污染全局命名空间。

## 目录结构

源码与文档：

```text
seuLex/
├── include/
│   ├── node.h
│   ├── nfa.h
│   ├── dfa.h
│   ├── lex_parser.h
│   ├── regex_expander.h
│   ├── nfa_constructor.h
│   ├── dfa_builder.h
│   ├── dfa_minimizer.h
│   └── code_generator.h
├── src/
│   ├── internal/
│   │   └── lex_state.h
│   ├── node.cpp
│   ├── lex_state.cpp
│   ├── lex_parser.cpp
│   ├── regex_expander.cpp
│   ├── nfa_constructor.cpp
│   ├── dfa_builder.cpp
│   ├── dfa_minimizer.cpp
│   ├── code_generator.cpp
│   └── main.cpp
├── docs/
│   ├── README.md
│   ├── ARCHITECTURE.md
│   ├── ALGORITHMS.md
│   ├── API_REFERENCE.md
│   ├── DATA_STRUCTURES.md
│   ├── DEPENDENCY_GRAPH.md
│   └── DEVELOPMENT_PROCESS.md
├── tests/
│   └── lex/
│       ├── test_cases/
│       ├── expected/
│       ├── results/
│       ├── run_lex_tests.sh
│       └── LEX_TEST_REPORT.md
├── CMakeLists.txt
└── README-LEX.md
```

运行或构建后还可能出现这些产物：

- `build/`
- `dot/`
- `generated_lexer.cpp`

它们不是源码模块的一部分。

## 功能

1. Lex 输入文件三段解析
2. 扩展 RE 转普通 RE
3. 中缀转后缀并构造单 RE 的 NFA
4. 多个 NFA 合并
5. NFA 确定化
6. DFA 最小化
7. 根据最小 DFA 生成完整词法分析器代码
8. 自测与 dot 可视化

## 构建

```bash
cd seuLex
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## 运行

```bash
./build/seuLex ../resources/minic.l generated_minic.cpp dot
```

或者运行自测：

```bash
./build/seuLex --self-test
```

## 说明

- 当前实现依赖 UNIX/POSIX 环境。
- 生成器输出包含：
  - `void begin_lexing(const std::string& source)`
  - `int analysis(std::string yytext)`
  - `int next_token()`
  - `std::vector<int> tokenize(const std::string& source)`
  - `std::vector<SeuLexToken> tokenize_detailed(const std::string& source)`
  - `std::vector<ParserToken> tokenize_for_parser(const std::string& source)`，当生成时传入 `--token-header`
  - `int input()`
- `SeuLexToken` 是面向生成 scanner 的稳定详细 token 结构，至少包含：
  - `type`
  - `lexeme`
  - `line`
  - `column`
- ABI 模式下，generated lexer 会直接包含 `seuYacc` 生成的 token 头，并把 `.l` 动作里的 `yylval` 写入 parser `Token.semantic`
- 若 `.l` 需要把指针语义绑定到最终 token 存储，可定义 `SEU_LEX_FINALIZE_PARSER_TOKEN(token_ref, token_view_ref)`
- `token_view_ref` 只保证提供稳定的 `type/lexeme/line/column` 视图；批量模式下它可能是最终 parser token 本身
- 若要逐 token 拉取输入，先调用 `begin_lexing(source)`，再调用 `next_token()` 或 `lex_one_parser_token(...)`
- 生成出的 scanner 现在还导出经典运行时符号：
  - `char yytext[]`
  - `int yylineno`
  - `int column`
- `yytext` 当前实现容量为 1 MiB；若单个匹配词素超出上限，scanner 会抛出运行时异常，而不是静默截断
- 自测会在 `/tmp` 下创建临时目录，不再把生成产物堆在仓库根目录。
- `.l` 规格中的 `%{...%}`、规则动作和用户子程序会原样进入生成的 C++，因此本工具只适用于可信的 Lex 输入文件。
- `{m,n}` 重复次数会做整数溢出检查，并限制在实现上限以内，避免异常大的规则直接拖垮生成过程。

## 文档导航

- [文档总览](./docs/README.md)
- [模块架构](./docs/ARCHITECTURE.md)
- [算法说明](./docs/ALGORITHMS.md)
- [API 参考](./docs/API_REFERENCE.md)
- [数据结构说明](./docs/DATA_STRUCTURES.md)
- [依赖图](./docs/DEPENDENCY_GRAPH.md)
- [开发与验证过程](./docs/DEVELOPMENT_PROCESS.md)
