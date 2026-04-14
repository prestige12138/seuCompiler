# seuYacc

模块化的 `seuYacc` 语法分析器生成器实现，当前代码目标是：

- 解析 `.y` 输入文件
- 构造规范 LR(1) 项目集族
- 生成 LR(1) / LALR(1) 分析表
- 生成独立的 C++ 语法分析器代码
- 提供基础符号表管理与内建自测

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
├── CMakeLists.txt
└── README-YACC.md
```

## 构建

```bash
cd seuYacc
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## 运行

```bash
./build/seuYacc ../resources/c99.y generated_parser.cpp generated_tokens.h lalr
```

或者运行自测：

```bash
./build/seuYacc --self-test
```

## 说明

- 当前实现依赖 UNIX/POSIX 环境。
- 全局文法表位于 `namespace seu_yacc`。
- 生成器会原样嵌入 `%{...%}` 和用户代码段，因此 `.y` 输入文件按可信代码处理。
- 默认输出 LALR(1) 分析器，也支持显式选择 `lr1` 模式。
