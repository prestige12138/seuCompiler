# seuLex 算法说明

## 1. Lex 文件解析

实现位置：

- `src/lex_parser.cpp`

### 目标

把 `.l` 文件解析为结构化的 `LexSpecification`，其中包含：

- `definitionsSection`
- `verbatimDefinitions`
- `rulesSection`
- `userSubroutines`
- `rules`

### 步骤

1. `readWholeFile()` 一次性读取整个文件
2. `findSectionDelimiter()` 定位两个独立的 `%%`
3. `takeBetweenMarkers()` 提取 `%{...%}` verbatim 区块
4. `removeRanges()` 去掉这些 verbatim 区块后再解析命名定义
5. Definitions 段写入 `idreTable`
6. Rules 段逐行累积成规则
7. `splitRegexAndAction()` 拆分 `regex` 和 `action`
8. `isActionBalanced()` 判断多行动作块是否闭合

### 关键规则

- 只有独立一行的 `%%` 才是分段符
- `%{...%}` 内部的 `%%` 不会切段
- 规则 action 内部的 `%%` 不会切段
- 多行动作块在字符串、字符常量、注释中的花括号不会误计数

### 复杂度

- 总体 `O(N)`
- `N` 为文件长度

## 2. 扩展正则展开

主要实现位置：

- `src/regex_expander.cpp`

核心组件：

- `REExpander`
- `expandNamedDefinitions()`
- `ExtendedRegexParser`

### 目标

把 Lex 风格扩展正则规范化为内部普通正则表示。

内部普通表示只保留：

- 字面量 token `ch:<ascii>`
- epsilon `eps`
- 显式连接 `&`
- 并 `|`
- 闭包 `*`

### 支持的扩展特性

- 命名定义：`{DIGIT}`
- 分组：`(...)`
- 字符类：`[abc]`
- 取反类：`[^a-z]`
- 区间：`[A-Z]`
- 通配符：`.`
- 字符串：`"if"`
- 转义：`\n`、`\t`、`\\`、`\"`、`\'`、`\0`
- 后缀运算：`*`、`+`、`?`
- 有界重复：`{m}`、`{m,n}`、`{m,}`

### 命名定义替换

`expandNamedDefinitions()` 会递归替换 `{NAME}`：

- 若引用未定义，抛异常
- 若出现循环引用，抛异常

### 解析方式

当前内部用递归下降生成 `RegexAst`：

- `parseUnion()`
- `parseConcat()`
- `parseRepeat()`
- `parsePrimary()`

### 复杂度

- 常见情况下近似线性
- 但 `{m,n}` 展开会带来 AST 复制成本

## 3. 中缀转后缀

实现位置：

- `NFABuilder::toPostfix()`

### 目标

把规范化中缀表达式转成后缀表达式，以便用栈驱动 Thompson 构造。

### 运算符

- `|`
- `&`
- `*`
- 括号

### 方法

使用标准运算符栈算法：

- 操作数直接输出
- `*` 在当前内部表示下是后缀运算符
- `|` 和 `&` 按优先级处理
- 括号控制结合范围

### 复杂度

- `O(T)`
- `T` 为 token 数

## 4. Thompson NFA 构造

实现位置：

- `NFABuilder::buildNFA()`

### 目标

从单条规则的后缀正则构造一个 NFA。

### 内部表示

- `Fragment { start, accept }`
- `node` 由内部 arena 分配
- `'\0'` 表示 epsilon

### 构造规则

- `eps`
  - 生成 `start --eps--> accept`
- `ch:x`
  - 生成 `start --x--> accept`
- `&`
  - 拼接两个 fragment
- `|`
  - 生成新分叉起点和汇合终点
- `*`
  - 生成回路和 epsilon 旁路

### 词法动作绑定

每条规则的接受态会记录到：

- `nfaterstatetoaction`
- `nfaPriorityTableInternal`

目的是保留 Lex 的“前面规则优先”语义。

### 复杂度

- `O(T + E)`

## 5. 多个 NFA 合并

实现位置：

- `NFABuilder::mergeNFA()`

### 目标

把所有规则 NFA 合并为一个总 NFA。

### 方法

1. 创建新的全局起点
2. 从全局起点向每个规则 NFA 的起点连 epsilon 边
3. 汇总所有终态

### 复杂度

- `O(R)`
- `R` 为规则数

## 6. DFA 子集构造

实现位置：

- `src/dfa_builder.cpp`
- `dfa::Eclosure()`
- `DFABuilder::subsetConstruct()`

### 目标

把合并后的 NFA 确定化为 DFA。

### 算法

0. 若输入 NFA 没有起始状态，直接返回空 DFA
1. 计算 `{nfa.start}` 的 epsilon-closure
2. 每个不同的 NFA 状态子集映射为一个 DFA 状态
3. 对 `char_set` 中每个字符做 move + closure
4. 使用排序后的 NFA 状态编号串做状态去重 key
5. 若子集包含接受态，则把该 DFA 状态标记为接受态

### 动作优先级保持

`pickActionFromSet()` 会从接受态集合中选出优先级最小的规则动作，保持：

- 最长匹配之外的规则优先级
- “先写先匹配”的 Lex 语义

### 复杂度

最坏情况下：

- `O(2^V * |Sigma|)`

其中：

- `V` 为 NFA 状态数
- `|Sigma|` 为字母表大小

## 7. DFA 最小化

实现位置：

- `src/dfa_minimizer.cpp`

### 目标

在不改变识别行为和接受动作的前提下压缩 DFA 状态数。

### 与教科书基础版的差异

不能仅按“接受 / 不接受”划分状态，因为：

- 不同接受态可能绑定不同动作
- 这些状态若合并会破坏词法语义

因此当前实现对接受态按 action 字符串进一步分组。

### 算法步骤

1. 初始划分：
   - 一个非接受态分区
   - 若干按 action 区分的接受态分区
2. 重复细化分区：
   - 当前分区号
   - action 字符串
   - 每个字符转移到的目标分区
3. 直到不再发生分裂
4. 用分区代表构造最小 DFA

### 复杂度

- `O(P * V * |Sigma|)`

其中：

- `P` 为分裂轮数
- `V` 为 DFA 状态数
- `|Sigma|` 为字母表大小

## 8. 代码生成

实现位置：

- `CodeGenerator::emitLexer()`

### 目标

把最小 DFA 输出为可独立编译的 C++ 词法分析器源码。

### 生成内容

- 状态转移表
- 接受态动作表
- `analysis(std::string yytext)`
- `input()`
- `next_token()`
- `tokenize(const std::string& source)`
- `tokenize_detailed(const std::string& source)`
- `SeuLexToken`
- verbatim definitions
- 规则动作
- 用户子程序

### 设计特点

- 生成器直接嵌入 `%{...%}` 和用户代码
- 因此输入 `.l` 被视为可信源
- 运行时统一维护 `yytext`、`yylineno`、`column`
- 详细 token 输出为后续 `seuYacc` 联通提供稳定桥接面

## 9. dot 可视化输出

实现位置：

- `Visualizer::dumpNFA()`
- `Visualizer::dumpDFA()`

### 目标

输出 Graphviz dot 文件，便于：

- 审阅自动机构造结果
- 调试规则和状态转移

### 当前输出

- `merged_nfa.dot`
- `dfa.dot`
- `min_dfa.dot`

## 10. 自测策略

实现位置：

- `SeuLexDriver::runSelfTests()`

### 当前覆盖

- 生成并编译一个小样例 lexer
- 直接验证 DFA 对若干词素的识别结果
- 校验若干非法正则应正确失败
- 验证 Lex 解析器在分隔符、注释、多行动作上的边界情况
- 生成 `resources/minic.l`

### 边界

当前自测会生成 `minic` 词法器，但不在 `seuLex` 子树内完成最终编译联调，因为其完整运行时环境位于整仓集成链路。

## 11. 复杂度总结

| 阶段 | 复杂度 |
|---|---|
| `.l` 解析 | `O(N)` |
| 扩展正则展开 | 近似线性，受重复展开影响 |
| 中缀转后缀 | `O(T)` |
| Thompson NFA | `O(T + E)` |
| NFA 合并 | `O(R)` |
| DFA 子集构造 | 最坏 `O(2^V * |Sigma|)` |
| DFA 最小化 | `O(P * V * |Sigma|)` |
| 代码生成 | 与状态数和动作文本总量线性相关 |
