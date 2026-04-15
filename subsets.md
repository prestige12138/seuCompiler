# Minic Subset 规划分析

本文件只做分析与规划，不涉及任何代码修改。

补充说明：

- 本文件中的 `MiniC-Plus` 是分析阶段提出过的“更大备选方案”
- 当前 `minic-plus` 分支实际落地时，没有采用这里那份更大的方案
- 现分支真实边界以 [MINIC-PLUS-SPEC.md](/Users/llawliet/代码/seuCompiler/MINIC-PLUS-SPEC.md) 为准，已经收紧到接近 `MiniC-Demo` 的稳定可执行子集

分析对象：

- `resources/c99.l`
- `resources/c99.y`
- `integration/tests/ir_pipeline/test_cases/demo_ir.l`
- `integration/tests/ir_pipeline/test_cases/demo_ir.y`
- `integration/tests/ir_pipeline/test_cases/demo_ir.c`
- `intermediate/include/intermediate_code.h`
- `intermediate/src/tri_addr_generator.cpp`

结论先行：

- 当前 `c99.l / c99.y` 明显是“接近完整 C99 骨架”的教学/参考规格，不是合适的课程展示型子集。
- 若要规划一个大小合适、能完整跑通 `Lex -> Yacc -> AST -> IR` 的自洽子集，最稳妥的边界应贴近现有 `demo_ir` 这一档。
- 最推荐的方案是 `MiniC-Demo`：`int-only`、结构化控制流、仅保留声明/赋值/算术/比较/if-else/while/函数调用/return。

## 1. 当前 C99 文法复杂度总结

### 1.1 规模

- `resources/c99.l`：`186` 行
- `resources/c99.y`：`471` 行
- `resources/c99.y`：约 `68` 个非终结符
- `resources/c99.y`：约 `63` 个命名 `%token`
- `resources/c99.y`：若按每个 `:` / `|` 备选式统计，约 `237` 个候选分支
- `resources/c99.l`：约 `95` 条真正返回 token 的规则
  - 约 `37` 条关键字规则
  - 约 `42` 条运算符/界符规则
  - 约 `16` 条正则模式规则

对比当前完整 IR 演示教学文法：

- `demo_ir.y`：`16` 个非终结符，约 `44` 个候选分支
- `demo_ir.l`：`39` 行
- `demo_ir.c`：`37` 行

这说明：当前 C99 规格的量级，大约是现有稳定教学闭环文法的 `4x ~ 5x`。

### 1.2 复杂度主要来自哪里

`c99.y` 的重心不是普通语句，而是 C 语言最难的两块：

- 复杂声明器体系
  - `declarator`
  - `direct_declarator`
  - `pointer`
  - `abstract_declarator`
  - `direct_abstract_declarator`
  - `type_name`
- 完整类型与初始化系统
  - `struct/union/enum`
  - `storage class / qualifier / function specifier`
  - `initializer_list`
  - `designation / designator`

同时还完整覆盖了这些高成本语法族：

- 完整表达式优先级链
  - cast
  - `sizeof`
  - `?:`
  - 逗号表达式
  - 位运算与移位
  - 复合赋值
- 完整控制流
  - `switch/case/default`
  - `while/do/for`
  - `goto/break/continue`
- 复杂后缀表达式
  - 下标 `[]`
  - 成员访问 `.`
  - 指针成员访问 `->`
  - `++/--`

### 1.3 词法复杂度

`c99.l` 也不是轻量版 scanner。它同时承担了：

- 大量关键字识别
- 十进制/八进制/十六进制整数
- 十进制/十六进制浮点
- 字符常量、字符串常量
- 复合操作符
  - `++ -- -> && || <= >= == != << >> += -= *= ...`
- digraph 支持
  - `<% %> <: :>`
- `TYPE_NAME` 预留通道

对课程展示型子集而言，这些绝大部分都不是必要成本。

## 2. 设计 Minic 子集时必须满足的约束

### 2.1 课程与仓库对齐约束

从当前仓库文档和测试链路看，一个“合格子集”至少要 demonstrably 覆盖：

- 函数定义
- 参数列表
- 局部变量声明
- 赋值
- 算术表达式
- 比较表达式
- `if / else`
- `while`
- 简单函数调用
- `return`

并且必须能完整跑通：

- `Lex -> Yacc -> AST -> TAC`
- 最好继续保持 `Lex -> Yacc -> AST -> TAC -> LLVM IR / Jimple`

### 2.2 现有 AST/IR 的天然边界

当前 `intermediate` 已稳定支持的 AST/TAC 核心节点是：

- `NODE_PROGRAM`
- `NODE_FUNC_DEF`
- `NODE_VAR_DECL`
- `NODE_ASSIGN`
- `NODE_ARITH`
- `NODE_FUNC_CALL`
- `NODE_IF`
- `NODE_WHILE`
- `NODE_RETURN`
- `NODE_ID`
- `NODE_CONSTANT`

这意味着，合适的子集应该尽量避免引入需要新增核心节点或复杂 lowering 的特性。

## 3. 子集规划原则

### 3.1 必须保留

- 结构化函数定义
- `int` 标量声明
- 表达式优先级
- 比较条件
- `if/else`
- `while`
- 函数调用
- `return`

### 3.2 应优先删除

- `typedef / TYPE_NAME / type_name / cast / sizeof`
- 指针、数组、函数指针、抽象声明器
- `struct / union / enum`
- `switch / do / for / goto / break / continue`
- `++ -- += -= *= /= %=`
- `&& || ! ?: ,`
- 浮点、字符、字符串、多类型宽度体系
- 初始化列表和 designator

### 3.3 核心判断

最危险的不是多几个关键字，而是：

- 声明器体系
- 类型参与语法判定

只要保留这两块，子集很快就会重新膨胀回接近完整 C。

## 4. 三个历史备选方案对比

说明：

- 本节是最初做子集规划时的历史备选方案分析
- 这里的 `MiniC-Plus` 是“候选更大方案”，不是当前 `minic-plus` 分支已经落地的实现边界
- 当前分支实际采用的边界，以 [MINIC-PLUS-SPEC.md](/Users/llawliet/代码/seuCompiler/MINIC-PLUS-SPEC.md) 为准

| 方案 | 定位 | 支持特性 | 明确不支持 | 预计删减量 | 全流程验证难度 |
| --- | --- | --- | --- | --- | --- |
| `TinyC-Core` | 更小方案 | `int` 函数、参数、局部声明 `int x;`、赋值、`+ - * /`、`< > ==`、`if`、`while`、函数调用、`return`、块语句 | `else`、声明初始化、`% <= >= !=`、多声明符、全局变量、`for/do/switch`、逻辑运算、数组/指针/结构体/枚举/限定符 | `c99.y` 约删 `80%~85%`，`c99.l` 约删 `70%+` | 低 |
| `MiniC-Demo` | 推荐方案 | `int` 函数定义、参数列表、局部声明 `int x;` 或 `int x = expr;`、赋值、函数调用、`+ - * / %`、`< > <= >= == !=`、`if/else`、`while`、`return`、块语句 | `for/do/switch`、`break/continue/goto`、数组/指针/结构体/枚举、`typedef/static/const` 等、字符串/字符/浮点、`++ -- +=`、逻辑短路、条件运算符、逗号表达式、复杂声明器 | `c99.y` 约删 `70%~80%`，`c99.l` 约删 `65%~75%` | 低到中 |
| `MiniC-Plus` | 更大方案 | 推荐方案全部能力，再加多声明符、简单全局 `int` 声明、`for`、`break/continue`、一元 `- !`、`&& ||` | 指针、数组、结构体、枚举、类型转换、函数指针、`switch`、`goto`、完整浮点/字符串/字符语义 | `c99.y` 约删 `55%~65%`，`c99.l` 约删 `50%~60%` | 中到高 |

## 5. 当时的推荐方案：MiniC-Demo

`MiniC-Demo` 也可以理解为“Structured Int-Only MiniC”。

这里仍然是分析阶段的方案定义，不代表当前 `minic-plus` 分支源码已经逐项实现了本节所有描述。

它的目标不是做最小玩具文法，而是做一个：

- 规模明显小于 C99
- 语言边界完整自洽
- 能展示课程核心能力
- 和当前 `demo_ir` 全流程最接近

的正式子集。

### 5.1 完整定义：支持哪些语法

#### 类型系统

- 仅支持 `int`

#### 顶层

- 仅支持多个函数定义
- 不支持全局变量声明
- 不支持函数原型声明

#### 函数

- 形式：`int f(int a, int b) { ... }`
- 参数类型仅允许 `int`
- 允许空参数表

#### 局部声明

- `int x;`
- `int x = expr;`

#### 语句

- 赋值语句：`x = expr;`
- 复合语句：`{ ... }`
- `if (expr) stmt`
- `if (expr) stmt else stmt`
- `while (expr) stmt`
- `return expr;`

#### 表达式

- 标识符
- 十进制整数常量
- 括号表达式
- 函数调用表达式：`f()`、`f(a, b)`
- 算术运算：`+ - * / %`
- 比较运算：`< > <= >= == !=`

#### 词法 token 集

建议保留：

- 关键字：`int` `if` `else` `while` `return`
- 标识符：`IDENTIFIER`
- 整数：`NUMBER`
- 运算符：`=` `+` `-` `*` `/` `%` `<` `>` `<=` `>=` `==` `!=`
- 界符：`(` `)` `{` `}` `,` `;`
- 空白与注释

### 5.2 完整定义：明确不支持哪些语法

- 所有复杂类型
  - `char short long float double signed unsigned void _Bool _Complex _Imaginary`
- 所有复杂声明器
  - 指针
  - 数组
  - 函数指针
  - 抽象声明器
  - `type_name`
- 所有聚合类型
  - `struct`
  - `union`
  - `enum`
- 所有复杂控制流
  - `switch/case/default`
  - `do-while`
  - `for`
  - `goto`
  - `break`
  - `continue`
- 所有复杂表达式
  - `++ --`
  - `+= -= *= /= %=`
  - `&& || !`
  - `?:`
  - 逗号表达式
  - cast
  - `sizeof`
  - 位运算与移位
- 所有复杂字面量
  - 浮点
  - 字符常量
  - 字符串常量
  - 十六进制浮点
  - 宽字符/宽字符串

### 5.3 为什么这个方案最合适

- 它已经覆盖课程展示最核心的语言功能。
- 它和现有 `demo_ir` 文法、AST、TAC、LLVM IR、Jimple 边界几乎一一对应。
- 它把最难的 C 语言复杂度一次性砍掉：
  - 复杂声明器
  - 类型参与语法判定
- 它不会像 `TinyC-Core` 那样过于 toy。
- 它也不会像 `MiniC-Plus` 那样重新把 grammar/semantic/IR 复杂度拉高。

## 6. 三个方案的优缺点

### 6.1 `TinyC-Core`

优点：

- 最容易实现
- 最容易验证
- 最容易快速稳定

缺点：

- 展示性偏弱
- 去掉 `else` 和声明初始化后，语言味道偏“演示专用”
- 和课程答辩里希望展示的“完整一点的子集语言”相比，略显保守

适用场景：

- 如果目标是“最快收缩到绝对稳态”

### 6.2 `MiniC-Demo`

优点：

- 和当前仓库的稳定全流程最匹配
- 能完整展示课程核心能力
- 验证难度仍然可控
- 对 AST/IR 几乎不需要新增核心抽象

缺点：

- 仍然不是“真正有用的 C 子集”
- 不支持全局变量、`for`、逻辑短路等常见语法

适用场景：

- 如果目标是“做一个课程实践提交最合适的正式子集”

### 6.3 `MiniC-Plus`

优点：

- 更像一门“可用的小语言”
- 展示能力更强

缺点：

- 控制流 lowering 会明显变复杂
- 逻辑短路和 `for` 会增加额外语义规则与 IR 约定
- 容易重新膨胀到中等规模前端

适用场景：

- 如果目标是“提交之后继续扩展”，而不是当前阶段收敛

## 7. 预计代码删减量与验证成本

### 7.1 若从当前 `c99.l / c99.y` 回退

`MiniC-Demo` 的合理目标规模大致应是：

- 词法：从约 `95` 条 token 规则，收缩到约 `18 ~ 24` 条
- 语法：从约 `68` 个非终结符、`237` 个候选分支，收缩到约 `16 ~ 22` 个非终结符、`40 ~ 60` 个候选分支

也就是说：

- `Lex` 侧预计可删 `65% ~ 75%`
- `Yacc` 侧预计可删 `70% ~ 80%`

### 7.2 全流程验证难度判断

`TinyC-Core`：

- 低
- 但展示面偏小

`MiniC-Demo`：

- 低到中
- 是“展示面”和“稳定性”的最好平衡点

`MiniC-Plus`：

- 中到高
- 主要风险来自控制流和逻辑表达式的语义/IR 复杂度上升

## 8. 推荐方案及理由

最终推荐：

- `MiniC-Demo`

推荐理由：

1. 它是当前仓库最自然的“正式子集化”方向。
2. 它已经被现有 `demo_ir` 样例证明能跑通完整链路。
3. 它保留了课程实践最需要展示的核心能力。
4. 它显著小于当前 C99。
5. 它避免了 C 语言真正最昂贵的两大爆点：
   - 复杂声明器系统
   - 类型参与语法判定

一句话结论：

如果后续真的要从当前 `c99.l / c99.y` 回退到子集版，最合适的不是“表达式玩具文法”，也不是“半个 C99”，而是以 `demo_ir` 这一档为骨架、补成正式规范的 `MiniC-Demo`。
