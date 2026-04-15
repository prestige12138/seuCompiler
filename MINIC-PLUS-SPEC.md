# MINIC-PLUS 规范

本文件定义 `minic-plus` 分支第一阶段的初始化边界。

当前阶段目标只有三件事：

1. 建立独立分支 `minic-plus`
2. 在 `resources/` 中放置 `minic-plus` 资源规格
3. 固化子集规则，后续实现严格以本规范为边界

本阶段不修改任何 `seuLex/`、`seuYacc/`、`intermediate/` 实现代码。

## 1. 设计目标

`MiniC-Plus` 是一个明显小于当前 `c99.l / c99.y` 的子集语言，同时比 `demo_ir` 对应的最小教学子集稍大一档。

它的目标不是逼近完整 C99，而是：

- 保留课程展示的核心能力
- 为后续 `Lex -> Yacc -> AST -> IR` 全流程提供足够表达力
- 避开 C 语言最昂贵的两类复杂度
  - 复杂声明器体系
  - 类型参与语法判定

## 2. 当前阶段状态

当前分支仅完成资源初始化：

- [resources/minic.l](/Users/llawliet/代码/seuCompiler/resources/minic.l)
- [resources/minic.y](/Users/llawliet/代码/seuCompiler/resources/minic.y)

说明：

- 这两个文件当前是“初始化规格稿”
- 它们尚未接入现有生成链和运行时
- 语义动作、AST 映射、IR 对接留到后续阶段

## 3. 支持的语言特性

### 3.1 类型

- 支持 `int`
- 支持 `void`

约束：

- 变量仅允许 `int`
- 函数返回类型允许 `int` 或 `void`

### 3.2 顶层结构

支持：

- 多个函数定义
- 简单全局 `int` 变量声明

不支持：

- 全局复杂初始化
- 函数原型声明
- `typedef`

### 3.3 函数

支持：

- `int f(int a, int b) { ... }`
- `void f(int a) { ... }`
- `int main(void) { ... }`
- 空参数表
- 逗号分隔参数列表

约束：

- 参数类型仅允许 `int`
- 不支持数组参数、指针参数、函数指针参数

### 3.4 声明

支持：

- `int x;`
- `int x = expr;`
- `int a, b;`
- `int a = 1, b = 2;`

支持范围：

- 全局声明
- 局部声明
- `for` 初始化子句中的简单 `int` 声明

不支持：

- `const` / `static` / `extern` / `register` / `volatile`
- 指针声明
- 数组声明
- 结构体、联合体、枚举声明

### 3.5 语句

支持：

- 赋值语句：`x = expr;`
- 空语句：`;`
- 复合语句：`{ ... }`
- `if (expr) stmt`
- `if (expr) stmt else stmt`
- `while (expr) stmt`
- `for (init; cond; step) stmt`
- `break;`
- `continue;`
- `return;`
- `return expr;`

不支持：

- `switch/case/default`
- `do-while`
- `goto`

### 3.6 表达式

支持：

- 标识符
- 十进制整数常量
- 括号表达式
- 函数调用表达式
- 一元运算
  - `-expr`
  - `!expr`
- 算术运算
  - `+ - * / %`
- 比较运算
  - `< > <= >= == !=`
- 逻辑运算
  - `&& ||`

约束：

- 赋值不作为通用表达式开放，只按“赋值语句”与 `for` 子句处理

不支持：

- `++ --`
- `+= -= *= /= %=`
- `?:`
- 逗号表达式
- cast
- `sizeof`
- 位运算与移位

### 3.7 词法

建议词法资源仅覆盖：

- 关键字
  - `int`
  - `void`
  - `if`
  - `else`
  - `while`
  - `for`
  - `break`
  - `continue`
  - `return`
- 标识符
- 十进制整数常量
- 注释
  - `/* ... */`
  - `// ...`
- 空白
- 运算符与界符
  - `= + - * / % ! && || < > <= >= == !=`
  - `(` `)` `{` `}` `,` `;`

## 4. 明确不支持的特性

以下内容全部排除在 `MiniC-Plus` 第一版边界之外：

- 指针
- 数组
- 函数指针
- 抽象声明器
- `type_name`
- `typedef`
- `struct`
- `union`
- `enum`
- 成员访问 `.` / `->`
- 下标 `[]`
- 字符串常量
- 字符常量
- 浮点常量
- `char short long float double signed unsigned _Bool _Complex _Imaginary`
- `switch/case/default`
- `do-while`
- `goto`
- `++ --`
- 复合赋值
- 位运算
- 移位运算
- 条件运算符 `?:`
- `sizeof`
- cast

## 5. 推荐语义边界

后续实现时建议保持以下约束不变：

- 只有一个标量变量类型：`int`
- 函数返回类型只做 `int / void`
- 块语句内部仍采用“声明在前，语句在后”的简化约束
- `for` 的 `init` 与 `step` 只支持简单赋值或简单 `int` 声明
- 逻辑表达式先允许语法存在，后续若实现 IR 时可再决定：
  - 采用短路求值
  - 或先降级为布尔化普通表达式

## 6. 与 main 分支相比的复杂度变化

相较当前接近完整 C99 的 `resources/c99.l / c99.y`，`MiniC-Plus` 会显著收缩：

- 删除大部分类型系统
- 删除复杂声明器
- 删除聚合类型
- 删除复杂表达式系统
- 删除大部分非结构化控制流

但它比最小教学子集更大，因为额外保留了：

- `void`
- 简单全局声明
- `for`
- `break / continue`
- 一元 `!`
- `&& / ||`

## 7. 第一阶段交付边界

本阶段已完成：

- 新建分支 `minic-plus`
- 初始化 `resources/minic.l`
- 新增 `resources/minic.y`
- 新增本规范文件

本阶段明确未做：

- 不修改 `seuLex/` 源码
- 不修改 `seuYacc/` 源码
- 不修改 `intermediate/` 源码
- 不调整构建脚本
- 不接入测试
- 不添加语义动作

## 8. 后续建议顺序

后续若继续推进，建议顺序如下：

1. 固化 token 集与非终结符集合
2. 把 `resources/minic.l / minic.y` 收紧到无二义的最终资源稿
3. 再接 AST 语义动作
4. 再接 TAC
5. 最后补 LLVM IR / Jimple

一句话边界：

`MiniC-Plus` 是“明显小于 C99、但足以展示结构化控制流与简单函数程序”的课程型子集，不是完整 C，也不是只剩表达式的玩具文法。
