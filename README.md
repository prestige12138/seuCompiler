# SEU Compiler 2026

SEU Compiler 2026 是一个按课程实践要求拆分的三阶段编译前端项目，当前仓库包含：

- `seuLex`：Lex 风格词法分析生成器
- `seuYacc`：Yacc 风格 LR(1)/LALR(1) 语法分析生成器
- `intermediate`：AST 与三地址中间代码生成模块
- `integration/tests/pipeline`：最小整链路联通样例

实现语言统一为 `C++17`。

## 当前状态

- `seuLex`：已实现，含 NFA/DFA 可视化、代码生成、自测与测试报告
- `seuYacc`：已实现，含 LR(1)/LALR(1) 自动机、分析表、代码生成、自测与测试报告
- `intermediate`：已实现，含 AST、符号表、三地址码生成、自测与测试报告
- 三模块联通：仓库内已提供一条最小端到端链路

当前仓库默认信任 `.l` / `.y` 规格中的用户动作与用户代码段。生成器会把这些代码原样写入生成产物，测试脚本也会编译并运行这些产物，因此本项目当前适用场景是课程实验、可信输入和本地开发环境，不适用于直接执行不可信文法。

当前最小联通路径是：

`seuLex` 生成 scanner -> `seuYacc` 生成 parser -> `.y` 语义动作构 AST -> `intermediate` 释放 AST 根并生成 IR

## 仓库结构

```text
.
├── CMakeLists.txt
├── README.md
├── AGENTS.md
├── docs/
├── resources/
├── seuLex/
├── seuYacc/
├── intermediate/
└── integration/
```

## 快速开始

### 顶层构建

```bash
cmake -S . -B build
cmake --build build -j
ctest --test-dir build --output-on-failure
```

### 分模块构建

```bash
cmake -S seuLex -B seuLex/build
cmake --build seuLex/build -j
ctest --test-dir seuLex/build --output-on-failure

cmake -S seuYacc -B seuYacc/build
cmake --build seuYacc/build -j
ctest --test-dir seuYacc/build --output-on-failure

cmake -S intermediate -B intermediate/build
cmake --build intermediate/build -j
ctest --test-dir intermediate/build --output-on-failure
```

### 运行最小整链路样例

```bash
./integration/tests/pipeline/run_pipeline_test.sh
```

该脚本会：

1. 生成 `pipeline_tokens.h` 和 `pipeline_parser.cpp`
2. 生成 `pipeline_lexer.cpp`
3. 编译一个桥接 driver
4. 执行 `Lex -> Yacc -> AST -> IR`
5. 校验输出 IR

## 模块入口

### seuLex

- CLI：[seuLex/src/main.cpp](/Users/llawliet/代码/seuCompiler/seuLex/src/main.cpp)
- 生成器门面：[seuLex/include/code_generator.h](/Users/llawliet/代码/seuCompiler/seuLex/include/code_generator.h)
- 模块说明：[seuLex/README-LEX.md](/Users/llawliet/代码/seuCompiler/seuLex/README-LEX.md)
- 模块文档：[seuLex/docs/README.md](/Users/llawliet/代码/seuCompiler/seuLex/docs/README.md)

### seuYacc

- CLI：[seuYacc/src/main.cpp](/Users/llawliet/代码/seuCompiler/seuYacc/src/main.cpp)
- 生成器门面：[seuYacc/include/parse_table.h](/Users/llawliet/代码/seuCompiler/seuYacc/include/parse_table.h)
- 模块说明：[seuYacc/README-YACC.md](/Users/llawliet/代码/seuCompiler/seuYacc/README-YACC.md)
- 模块文档：[seuYacc/docs/README.md](/Users/llawliet/代码/seuCompiler/seuYacc/docs/README.md)

### intermediate

- CLI / 自测：[intermediate/src/main.cpp](/Users/llawliet/代码/seuCompiler/intermediate/src/main.cpp)
- AST 接口：[intermediate/include/ast_builder.h](/Users/llawliet/代码/seuCompiler/intermediate/include/ast_builder.h)
- IR 接口：[intermediate/include/tri_addr_generator.h](/Users/llawliet/代码/seuCompiler/intermediate/include/tri_addr_generator.h)
- 模块说明：[intermediate/README-INTERMEDIATE.md](/Users/llawliet/代码/seuCompiler/intermediate/README-INTERMEDIATE.md)

## 联通约定

### Lex -> Yacc

`seuLex` 生成的 scanner 现在同时导出：

- `int analysis(std::string yytext)`
- `int next_token()`
- `std::vector<int> tokenize(const std::string& source)`
- `std::vector<SeuLexToken> tokenize_detailed(const std::string& source)`

其中 `SeuLexToken` 至少包含：

- `type`
- `lexeme`
- `line`
- `column`

`seuYacc` 生成的 parser 仍保持：

```cpp
bool yyparse(const std::vector<Token>& tokens);
```

当前桥接层需要把 `SeuLexToken` 转成 parser 命名空间里的 `Token`，并按文法需求补全 `YYSTYPE semantic`。
对 `NUMBER`、`IDENTIFIER` 这类可从 `lexeme` 直接恢复语义值的 token，可直接在桥接层回填。
如果词法动作生成了无法从 `lexeme` 反推的语义信息，则需要自定义更强的 Lex→Yacc 语义传递约定。

### Yacc -> Intermediate

当前稳定契约是：

1. `.y` 语义动作里构 AST
2. 在开始符号归约完成时调用 `seu_icg::setParseRoot(...)`
3. 外层驱动调用 `seu_icg::releaseParseRoot()`
4. 用 `seu_icg::TriAddrGenerator` 生成 IR

## 课程资源

参考输入与课程文档位于 [resources/](/Users/llawliet/代码/seuCompiler/resources)：

- [c99.l](/Users/llawliet/代码/seuCompiler/resources/c99.l)
- [minic.l](/Users/llawliet/代码/seuCompiler/resources/minic.l)
- [c99.y](/Users/llawliet/代码/seuCompiler/resources/c99.y)
- [编译原理中期报告.docx](/Users/llawliet/代码/seuCompiler/resources/编译原理中期报告.docx)
- [编译原理课程实践 2026.pptx](/Users/llawliet/代码/seuCompiler/resources/编译原理课程实践%202026.pptx)

## 进一步阅读

- 顶层整合设计：[docs/INTEGRATION.md](/Users/llawliet/代码/seuCompiler/docs/INTEGRATION.md)
- 本轮终审记录：[docs/FINAL_AUDIT.md](/Users/llawliet/代码/seuCompiler/docs/FINAL_AUDIT.md)
