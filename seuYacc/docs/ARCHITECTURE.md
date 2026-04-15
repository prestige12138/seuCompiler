# seuYacc 架构说明

## 1. 总体目标

`seuYacc` 的目标不是解释执行 `.y` 文件，而是把 `.y` 文件编译成一个新的、独立的 C++17 语法分析器。

因此它本质上是一个“生成器”，而不是“运行时解释器”。

完整流水线如下：

1. 解析 `.y` 文件三段内容
2. 建立内部文法、优先级、类型信息
3. 构造 LR 自动机
4. 生成 ACTION / GOTO 表
5. 输出 parser 源码和头文件
6. 由外部 C++ 编译器编译生成后的 parser

## 2. 模块划分

### `yacc_parser.*`

职责：

- 读取 `.y` 文件
- 切分 Definitions / Rules / User Subroutines 三段
- 提取 `%{...%}`、`%union`、`%token`、`%type`、`%left`、`%right`、`%nonassoc`、`%start`
- 解析产生式
- 解析 `%prec`
- 解析终结符、非终结符、动作块
- 将 mid-rule action 转换为合成空产生式

输入：

- `.y` 文件路径

输出：

- `YaccSpecification`

### `symbol_table.*`

职责：

- 维护报告要求的全局文法表
- 维护 token / nonterminal 的编号
- 维护优先级与结合性映射
- 维护生成 parser 运行期符号表

输入：

- `YaccSpecification` 中的 token、规则、类型和优先级信息

输出：

- 全局表 `ops`、`terminals`、`nonterminals`、`producers`
- 运行期查询接口

### `lr1_pda.*`

职责：

- 计算 FIRST 集
- 计算 FOLLOW 集
- 计算 LR(1) closure / goto
- 构造 canonical LR(1) 自动机
- 构造 direct LALR(1) 自动机

特点：

- `buildCanonicalPDA()` 保留规范 LR(1) 版本
- `buildLALRPDA()` 直接在 LR(0) core 上传播 lookahead，避免较大文法上的状态爆炸

### `lalr_converter.*`

职责：

- 把 canonical LR(1) 状态按 LR(0) core 合并为 LALR(1)

当前定位：

- 算法对照模块
- 主要用于自测验证 direct-LALR 构造是否与“canonical 再合并”的结果一致

### `parse_table.*`

职责：

- 构建 ACTION / GOTO 表
- 处理 shift/reduce、reduce/reduce 冲突
- 生成 C++ parser 代码
- 提供 `SeuYaccDriver`
- 提供内建自测

这是模块中最“集成”的一层，负责把前面的数据结构变成最终可交付产物。

### `main.cpp`

职责：

- 提供命令行入口
- 调用 `SeuYaccDriver`

## 3. 主调用链

默认 `lalr` 模式的调用链：

1. `main`
2. `SeuYaccDriver::generate`
3. `YaccParser::parseYaccFile`
4. `SymbolTableManager` 填充全局表
5. `LR1Builder::buildLALRPDA`
6. `ParseTableBuilder::buildLALRTable`
7. `ParserCodeGenerator::emitParser`

显式 `lr1` 模式的调用链：

1. `main`
2. `SeuYaccDriver::generate`
3. `YaccParser::parseYaccFile`
4. `SymbolTableManager` 填充全局表
5. `LR1Builder::buildCanonicalPDA`
6. `ParseTableBuilder::buildLR1Table`
7. `ParserCodeGenerator::emitParser`

## 4. 设计决策

### 默认不先构 canonical LR(1)

原因：

- 历史上完整规格文法规模较大
- 规范 LR(1) 状态数膨胀明显
- 直接走 LR(0) core + lookahead 传播更适合当前工程目标

### 仍保留 canonical LR(1)

原因：

- 用户要求完整 LR(1) 功能
- 对小文法更便于审查
- 可作为对 direct-LALR 的算法参照

### 生成器而不是运行时解释器

原因：

- 课程项目最终目标是“生成编译器前端组件”
- 生成后的 parser 应可脱离 `seuYacc` 独立编译与使用

## 5. 生成代码的结构

生成出的 `.cpp` 大体包含：

1. 头文件 include
2. `%{...%}` 段代码
3. 第三段用户代码
4. `namespace <generated>`
5. 运行期辅助结构
6. 产生式元数据
7. ACTION / GOTO 表
8. token 到符号名映射
9. 语义动作执行函数
10. `yyparse(const std::vector<Token>&)`

生成出的 `.cpp` 现在对生成头使用相对路径 include，避免产物绑定到某个绝对工作区。

生成出的 `.h` 包含：

1. `YYSTYPE`
2. `TokenKind`
3. `Token`
4. `yyparse` 声明

## 6. 运行期接口约定

生成 parser 不直接读取文本流，而是接收：

```cpp
bool yyparse(const std::vector<Token>& tokens);
```

其中 `Token` 至少包含：

- `type`
- `lexeme`
- `line`
- `column`
- `semantic`

这意味着 `seuLex` 的职责是把输入源程序先转成 token 序列，`seuYacc` 生成的 parser 再消费这批 token。

当前推荐桥接方式是使用 `seuLex` 生成 scanner 的 `tokenize_detailed(...)` 结果，再映射到 parser 命名空间下的 `Token`。

## 7. 自测架构

内建自测覆盖三层：

- 小表达式文法
  - 校验基础生成链和解析执行
- 语义动作文法
  - 校验 `%union`、`%type`、typed action、第三段用户代码
- `minic.y`
  - 校验真实规模文法的生成和生成后编译

这三层组合之后，能覆盖当前最容易出错的生成路径。
