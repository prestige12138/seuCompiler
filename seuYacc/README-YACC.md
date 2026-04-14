# seuYacc

`seuYacc` 是 SEU Compiler 2026 中独立的语法分析器生成模块。它负责把 `.y` 文法文件解析为内部文法表示，构造 LR(1) / LALR(1) 自动机与分析表，并输出可直接编译的 C++17 语法分析器代码。

当前实现保持模块化结构，严格把全局文法数据放在 `namespace seu_yacc` 下，并复用中期报告中的核心 Yacc 数据结构命名：

- `operators`
- `produce`
- `ITEM`
- `LRnode`
- `LRPDA`
- `parse_table_item`
- 全局表 `ops`、`terminals`、`nonterminals`、`producers`

## 功能范围

已实现：

- `.y` 文件三段解析
- `%token`、`%type`、`%left`、`%right`、`%nonassoc`、`%start`、`%union` 解析
- 产生式、`%prec`、引号终结符、嵌套动作块解析
- mid-rule action 转换为合成空产生式
- FIRST / FOLLOW 计算
- canonical LR(1) 项目集族构造
- direct LALR(1) lookahead 传播构造
- LR(1) / LALR(1) 分析表生成与冲突处理
- 生成独立的 `parser.cpp` + `tokens.h`
- 生成代码中的 `%union`、typed semantic action、第三段用户代码嵌入
- 内建自测

当前明确不覆盖：

- Bison 扩展的完整指令族
- GLR / IELR
- `%locations`
- `%destructor`
- `%printer`
- 流式 lexer 集成接口

## 目录结构

```text
seuYacc/
├── include/
│   ├── yacc_parser.h
│   ├── lr1_pda.h
│   ├── parse_table.h
│   ├── lalr_converter.h
│   └── symbol_table.h
├── src/
│   ├── yacc_parser.cpp
│   ├── lr1_pda.cpp
│   ├── parse_table.cpp
│   ├── lalr_converter.cpp
│   ├── symbol_table.cpp
│   └── main.cpp
├── docs/
│   ├── README.md
│   ├── ARCHITECTURE.md
│   ├── ALGORITHMS.md
│   ├── API_REFERENCE.md
│   ├── DATA_STRUCTURES.md
│   ├── DEPENDENCY_GRAPH.md
│   └── DEVELOPMENT_PROCESS.md
├── CMakeLists.txt
└── README-YACC.md
```

## 快速开始

构建：

```bash
cd seuYacc
cmake -S . -B build
cmake --build build
```

运行生成器：

```bash
./build/seuYacc ../resources/c99.y generated_parser.cpp generated_tokens.h lalr
```

运行内建自测：

```bash
./build/seuYacc --self-test
ctest --test-dir build --output-on-failure
```

`mode` 支持：

- `lalr`
- `lr1`

默认模式为 `lalr`。

## 生成器输入输出

输入：

- 主输入为一个 `.y` 文法文件
- `resources/c99.y` 是当前默认参考文法
- `resources/minic.l` 是配套词法规范参考，主要用于 token 对齐和整链路联调背景

输出：

- 一个 C++ 源文件，包含：
  - ACTION / GOTO 表
  - `yyparse(const std::vector<Token>&)` 总控程序
  - 语义动作执行逻辑
  - 第三段用户代码
- 一个头文件，包含：
  - `TokenKind`
  - `YYSTYPE`
  - `Token`
  - `yyparse` 声明

生成出的 `.cpp` 现在使用相对路径 include 生成头文件，不再把工作区绝对路径写入产物，便于移动、worktree 和顶层构建。

## 模块职责

- `yacc_parser.*`
  - 负责 `.y` 文法解析
- `symbol_table.*`
  - 负责全局文法表与运行期符号表
- `lr1_pda.*`
  - 负责 FIRST / FOLLOW、canonical LR(1)、direct LALR(1)
- `lalr_converter.*`
  - 负责 canonical LR(1) 到 LALR(1) 的状态合并
- `parse_table.*`
  - 负责分析表生成、代码生成、总控驱动、自测
- `main.cpp`
  - 命令行入口

## 当前实现策略

- 对外默认走 direct LALR(1) 构造，而不是先完整构 canonical LR(1) 再合并。这是为控制 `c99.y` 级别文法的状态膨胀。
- canonical LR(1) 仍保留，供 `lr1` 模式和小文法验证使用。
- `LALRConverter` 保留为显式 LR(1) 到 LALR(1) 合并模块，主要用于算法对照和自测验证。
- 生成器把 `%{...%}` 和第三段用户代码按可信 C/C++ 代码处理，不做沙箱执行。

## 文档导航

- [文档总览](./docs/README.md)
- [模块架构](./docs/ARCHITECTURE.md)
- [算法说明](./docs/ALGORITHMS.md)
- [API 参考](./docs/API_REFERENCE.md)
- [数据结构说明](./docs/DATA_STRUCTURES.md)
- [依赖图](./docs/DEPENDENCY_GRAPH.md)
- [开发与验证过程](./docs/DEVELOPMENT_PROCESS.md)

## 已验证行为

当前自测覆盖：

- 小表达式文法的生成、编译、运行
- 带 `%union`、`%type`、语义动作、第三段用户代码的文法
- `resources/c99.y` 的生成与生成后编译

## 使用约束

- 需要 UNIX / POSIX 环境
- 需要可用的 C++17 编译器
- 生成器内部自测会调用外部编译器，并设置 60 秒超时
- 生成出的 parser 当前接口仍是批量 token 输入，不直接驱动 `lex` 文件
- 生成的 token 头会稳定导出 `SEU_YACC_TOKEN_NAMESPACE`、`SEU_YACC_TOKEN_TYPE`、`SEU_YACC_SEMANTIC_TYPE`
- 与 `seuLex` 联通时，推荐让 `seuLex` 以 `--token-header <generated_tokens.h>` 生成 ABI 模式 lexer，并直接调用 `tokenize_for_parser(...)`
