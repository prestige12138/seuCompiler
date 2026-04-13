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

```text
seuLex/
├── include/
│   ├── node.h
│   ├── nfa.h
│   ├── dfa.h
│   ├── lex_parser.h
│   ├── nfa_constructor.h
│   ├── dfa_minimizer.h
│   └── code_generator.h
├── src/
│   ├── lex_parser.cpp
│   ├── nfa_constructor.cpp
│   ├── dfa_minimizer.cpp
│   ├── code_generator.cpp
│   └── main.cpp
├── tests/
│   └── sample_smoke.l
├── CMakeLists.txt
└── README-LEX.md
```

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
  - `int analysis(std::string yytext)`
  - `int next_token()`
  - `std::vector<int> tokenize(const std::string& source)`
  - `int input()`
- 自测会在 `/tmp` 下创建临时目录，不再把生成产物堆在仓库根目录。
- `.l` 规格中的 `%{...%}`、规则动作和用户子程序会原样进入生成的 C++，因此本工具只适用于可信的 Lex 输入文件。
- `{m,n}` 重复次数会做整数溢出检查，并限制在实现上限以内，避免异常大的规则直接拖垮生成过程。
