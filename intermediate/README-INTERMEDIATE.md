# seuIntermediate

`seuIntermediate` 是 SEU Compiler 2026 的中间代码生成模块。它严格复用中期报告里的核心数据结构：

- `ASTNodeType`
- `ASTNode`
- `TriOp`
- `TriAddrStmt`
- `IntermediateCode`

所有接口统一放在 `namespace seu_icg` 下，语言标准为 C++17。

## 目录结构

```text
intermediate/
├── include/
│   ├── ast_builder.h
│   ├── symbol_table.h
│   ├── tri_addr_generator.h
│   ├── intermediate_code.h
│   └── target_ir_emitter.h
├── src/
│   ├── ast_builder.cpp
│   ├── symbol_table.cpp
│   ├── tri_addr_generator.cpp
│   ├── intermediate_code.cpp
│   ├── target_ir_emitter.cpp
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
└── README-INTERMEDIATE.md
```

## 功能范围

当前实现覆盖：

1. 基于语法分析结果构建带语义信息的 AST
2. 全局 / 局部 / 参数三类符号表项管理与作用域切换
3. 三地址语句生成
4. 标准文本格式输出
5. 基于 AST + 三地址码的 LLVM IR / Jimple 文本输出
6. 内建自测

当前第一版重点支持这些 AST/IR 场景：

- 变量声明与带初始化的声明
- 赋值语句
- 二元算术运算 `+ - * / %`
- 函数调用
- `if / else`
- `while`
- `return`
- 函数定义作用域与参数登记
- LLVM IR 非 SSA 内存式 lowering
- Jimple 风格方法/局部变量/标签输出

## 构建

```bash
cd intermediate
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

## 运行

```bash
./build/seuIntermediate --self-test
```

## 与 seuYacc 的集成方式

当前 `seuYacc` 生成的 parser 对外契约是：

```cpp
bool yyparse(const std::vector<Token>& tokens);
```

它不会直接返回 AST，因此最稳的接法是在 `.y` 语义动作中直接构 AST，并在开始符号归约完成时导出根节点。

推荐的 `%union` 写法：

```yacc
%union {
  void* node;
  int ival;
}
```

建议用 `void* node`，而不是直接写 `ASTNode*`。原因是当前 `seuYacc` 会把 `%union` 原样写进生成头文件，但不会自动把 `%{ %}` 中的头文件同步到生成头文件。

典型接法如下：

```yacc
%{
#include "ast_builder.h"
#include "intermediate_code.h"

static seu_icg::ASTBuilder g_ast_builder;
%}

%union {
  void* node;
}

%%
translation_unit
  : function_definition
    {
      seu_icg::setParseRoot(static_cast<seu_icg::ASTNode*>($1.node));
    }
  ;
%%
```

外层驱动流程：

1. `yyparse(tokens)`
2. `ASTNode* root = seu_icg::releaseParseRoot()`
3. `seu_icg::SymbolTable symbols;`
4. `seu_icg::TriAddrGenerator generator(&symbols);`
5. `IntermediateCode code = generator.generate(root);`
6. `dumpIntermediateCode(code, std::cout);`
7. 如需后端文本，可继续调用 `formatLlvmIr(root, code, options)` 或 `formatJimple(root, code, options)`

如果需要完整三模块联通，推荐链路是：

1. 先由 `seuYacc` 生成 `generated_tokens.h`
2. `seuLex` 以 `--token-header generated_tokens.h` 生成 ABI 模式 lexer
3. 直接调用 `tokenize_for_parser(source)`
3. `yyparse(tokens)`
4. `releaseParseRoot()`
5. `TriAddrGenerator::generate(root)`
6. `formatLlvmIr(root, code, options)` / `formatJimple(root, code, options)`

## AST 约定

由于中期报告中的 `ASTNodeType` 没有单独定义块节点或参数列表节点，当前实现约定：

- `NODE_PROGRAM`
  - 可作为程序根节点
  - 也可作为通用语句列表 / 函数体容器
- `NODE_FUNC_DEF`
  - `value` 保存函数名
  - `varType` 保存返回类型
  - 前若干个子节点是参数声明
  - 最后一个子节点是函数体
- `NODE_VAR_DECL`
  - `value` 保存变量名
  - `varType` 保存变量类型
  - 可选第一个子节点是初始化表达式
- `NODE_ARITH`
  - `value` 保存运算符
  - 算术运算和关系条件都通过该节点表达

## 三地址码输出格式

输出格式为稳定的文本三地址表示，例如：

```text
1: t1 = 2 * 3
2: t2 = 1 + t1
3: x = t2
4: if x < 10 goto 6
5: goto 9
6: t3 = call foo(x, 1)
7: x = t3
8: goto 4
9: return x
```

其中：

- 临时变量统一命名为 `t1`, `t2`, ...
- 跳转目标直接使用语句编号
- `OP_FUNC_CALL` 输出为 `result = call func(arglist)`
- 额外提供 `splitBasicBlocks(...)` / `formatBasicBlocks(...)` 作为基本块视图

## LLVM IR / Jimple 输出

当前新增公开接口：

```cpp
seu_icg::TargetIrOptions options;
const std::string llvm_ir = seu_icg::formatLlvmIr(root, code, options);
const std::string jimple = seu_icg::formatJimple(root, code, options);
```

设计要点：

- 不修改报告规定的 `TriAddrStmt` / `IntermediateCode`
- 通过 AST 恢复函数签名、参数和局部变量
- 通过三地址码恢复控制流和临时变量
- LLVM IR 采用非 SSA、`alloca/load/store` 风格
- Jimple 采用方法 + local + label 的稳定文本形式

当前输出边界：

- 支持 `int` / `void` 函数
- 支持标量局部变量、参数、临时变量
- 支持 `+ - * / %`、条件跳转、循环、函数调用和 `return`
- 不涉及 `phi`、指针、数组、结构体和真实后端优化

## 自测内容

内建自测覆盖：

- parse-root 导出与释放
- 作用域符号表查表与遮蔽
- 算术赋值 IR
- `if/else`、`while`、函数调用 IR
- 函数体与参数作用域 IR
- 基本块划分与后继块格式化
- LLVM IR 输出 smoke test
- Jimple 输出 smoke test

## 当前边界

- 当前模块不负责词法分析或 LR 分析表构造
- 当前模块默认 AST 已经由上游语义动作或适配层构建完成
- 当前模块没有扩展报告之外的 IR 操作符，例如专门的 `LABEL`、`PARAM`、`FUNC_BEGIN`
- 条件表达式当前以文本条件直接挂到 `OP_IF_GOTO.arg1`
- `splitBasicBlocks(...)` 返回的块保留原始 `stmtNo`；若只关心块内语句条数，应使用 `stmts.size()`
- `TargetIrEmitter` 会基于 AST 推断每个函数的语句段长度，再切分传入的 `IntermediateCode`；这是因为当前三地址码没有显式 `FUNC_BEGIN/FUNC_END`

## 文档导航

- [文档总览](./docs/README.md)
- [模块架构](./docs/ARCHITECTURE.md)
- [算法说明](./docs/ALGORITHMS.md)
- [API 参考](./docs/API_REFERENCE.md)
- [数据结构说明](./docs/DATA_STRUCTURES.md)
- [依赖图](./docs/DEPENDENCY_GRAPH.md)
- [开发与验证过程](./docs/DEVELOPMENT_PROCESS.md)
