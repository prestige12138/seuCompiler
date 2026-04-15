# MINIC-PLUS 规范

本文件定义 `minic-plus` 分支当前采用的收缩版子集边界。

这一版不再沿用最初那份“比 `demo_ir` 更大一档”的草案，而是明确收紧到当前仓库已经稳定跑通的最小自洽子集。原因很直接：这次目标是“最小化删改，不新增功能”，因此只能以现有实现真实支持的能力为准，不能继续保留 `void`、全局变量、`for`、`break/continue`、`&&/||` 这类尚未在现有主链路中稳定闭环的特性声明。

## 1. 设计目标

`MiniC-Plus` 现在的定位是：

- 明显小于 `resources/c99.l` / `resources/c99.y`
- 与当前 `Lex -> Yacc -> AST -> IR` 可执行链路保持一致
- 保留课程演示最核心的结构化语法能力
- 尽量复用现有实现，不对 `seuLex`、`seuYacc`、`intermediate` 做结构性改造

当前可执行基线与 [integration/tests/ir_pipeline/test_cases/demo_ir.l](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/test_cases/demo_ir.l) 和 [integration/tests/ir_pipeline/test_cases/demo_ir.y](/Users/llawliet/代码/seuCompiler/integration/tests/ir_pipeline/test_cases/demo_ir.y) 保持同一能力级别。

## 2. 当前阶段状态

当前分支只做了“边界收紧”，没有扩展实现：

- [resources/minic.l](/Users/llawliet/代码/seuCompiler/resources/minic.l)
- [resources/minic.y](/Users/llawliet/代码/seuCompiler/resources/minic.y)

约束：

- 不修改 `seuLex/` 代码结构
- 不修改 `seuYacc/` 代码结构
- 不修改 `intermediate/` 代码结构
- 不新增 AST 结点、IR 操作或特判逻辑

## 3. 支持的语言特性

### 3.1 类型

- 仅支持 `int`

约束：

- 变量类型仅允许 `int`
- 函数返回类型仅允许 `int`

### 3.2 顶层结构

支持：

- 多个函数定义

不支持：

- 全局变量声明
- 函数原型声明
- `typedef`

### 3.3 函数

支持：

- `int f(int a, int b) { ... }`
- `int main() { ... }`
- 空参数表
- 逗号分隔参数列表

约束：

- 参数类型仅允许 `int`
- 不支持 `void` 参数表记法
- 不支持数组参数、指针参数、函数指针参数

### 3.4 声明

支持：

- `int x;`

约束：

- 仅支持局部简单声明
- 块内仍采用“声明在前，语句在后”的简化约束

不支持：

- 声明初始化
- 逗号声明符
- 全局声明
- `const` / `static` / `extern` / `register` / `volatile`
- 指针、数组、结构体、联合体、枚举

### 3.5 语句

支持：

- 赋值语句：`x = expr;`
- 复合语句：`{ ... }`
- `if (expr) stmt`
- `if (expr) stmt else stmt`
- `while (expr) stmt`
- `return expr;`

不支持：

- 空语句
- `return;`
- `for`
- `break`
- `continue`
- `switch/case/default`
- `do-while`
- `goto`

### 3.6 表达式

支持：

- 标识符
- 十进制整数常量
- 括号表达式
- 函数调用表达式
- 算术运算：`+ - * / %`
- 比较运算：`< > <= >= == !=`

不支持：

- 一元 `!`
- 一元负号
- `&& ||`
- 赋值表达式
- `++ --`
- 复合赋值
- 逗号表达式
- cast
- `sizeof`
- 位运算、移位、条件运算符

### 3.7 词法

词法资源只覆盖以下内容：

- 关键字：`int if else while return`
- 标识符
- 十进制整数常量
- 注释：`/* ... */`、`// ...`
- 空白
- 运算符与界符：
  - `= + - * / % < > <= >= == !=`
  - `(` `)` `{` `}` `,` `;`

## 4. 明确不支持的特性

以下内容全部排除在当前 `MiniC-Plus` 边界之外：

- `void`
- 全局变量
- `for`
- `break`
- `continue`
- `&& || !`
- 指针
- 数组
- 函数指针
- `typedef`
- `struct`
- `union`
- `enum`
- 成员访问 `.` / `->`
- 下标 `[]`
- 字符串、字符、浮点常量
- 复合赋值
- 位运算与移位
- 条件运算符 `?:`
- `sizeof`
- cast

## 5. 与 main 分支相比的复杂度变化

相较当前 `c99` 资源，当前 `MiniC-Plus` 明显更小：

- 删除了几乎全部类型系统分支
- 删除了复杂声明器
- 删除了聚合类型
- 删除了非结构化控制流
- 删除了逻辑表达式和大部分一元运算

它现在不再追求“略大于 `demo_ir`”，而是直接与当前稳定可执行链路等价。

## 6. 当前交付边界

本阶段已完成：

- 新建并维护独立分支 `minic-plus`
- 收紧 [resources/minic.l](/Users/llawliet/代码/seuCompiler/resources/minic.l)
- 收紧 [resources/minic.y](/Users/llawliet/代码/seuCompiler/resources/minic.y)
- 同步更新本规范文件

### 6.1 已删除内容

本轮按“只删冗余、不扩功能”的原则，额外删除了以下与当前子集无关的代码路径：

- `seuLex` 中仅用于 `resources/c99.l` 的仓库探测与 `--self-test` 生成分支
- `seuLex/tests/lex/run_lex_tests.sh` 中的 `c99.l` 回归用例
- `seuYacc` 中仅用于 `resources/c99.y` 的仓库探测与 `--self-test` 生成分支
- `seuYacc/tests/yacc/run_yacc_tests.sh` 中的 `c99.y` 回归生成与大文法性能基线
- `intermediate` 中未被当前主链路使用的 `ASTBuilder::makeUnary` / `ASTBuilder::makeTernary` helper

这些删除都没有改变当前 `minic-plus` 子集的语言边界，只是去掉了与该边界无关的冗余实现和测试入口。

本阶段明确未做：

- 不重构现有模块
- 不新增语法/语义能力
- 不修改主链路测试样例
- 不扩展 AST / TAC / LLVM IR / Jimple 支持边界

## 7. 后续建议顺序

如果后续还要继续推进 `minic-plus`，建议顺序是：

1. 先把 `resources/minic.*` 正式接入一条独立集成测试
2. 再按真实需要逐项放开更大的特性
3. 每放开一项，都要同步补 AST、IR、测试和文档
