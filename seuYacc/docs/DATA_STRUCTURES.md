# seuYacc 数据结构说明

## 1. 设计原则

本模块严格复用了中期报告要求的核心 Yacc 数据结构命名，并在不破坏这些结构的前提下增加了少量工程化辅助结构。

可以把数据结构分成三层：

- 报告规定的核心结构
- 解析阶段的附加结构
- 运行期与生成期的辅助结构

## 2. 报告规定的核心结构

### `operators`

声明位置：

- `include/yacc_parser.h`

定义：

```cpp
typedef struct operators {
  std::vector<char> op;
  char* rl = nullptr;
  int level = 0;
} LROP;
```

作用：

- 描述一个优先级组

字段含义：

- `op`
  - 当前组中以字符形式保存的运算符
- `rl`
  - 结合性，取值为 `"left"`、`"right"`、`"nonassoc"`
- `level`
  - 优先级层级

说明：

- 结构名保留为 `operators`
- 同时保留别名 `LROP`

### `produce`

声明位置：

- `include/yacc_parser.h`

定义：

```cpp
typedef struct produce {
  std::string left;
  std::vector<std::string> right;
} producer;
```

作用：

- 表示一条产生式的核心文法部分

字段含义：

- `left`
  - 左部非终结符
- `right`
  - 右部符号序列

### `ITEM`

声明位置：

- `include/lr1_pda.h`

定义：

```cpp
typedef struct ITEM {
  std::string left;
  std::vector<std::string> right;
  int dotpos = 0;
  std::string predict;
} LRItem;
```

作用：

- 表示一个 LR(1) 项

字段含义：

- `left`
  - 产生式左部
- `right`
  - 产生式右部
- `dotpos`
  - 点的位置
- `predict`
  - lookahead

### `node`

声明位置：

- `include/lr1_pda.h`

定义：

```cpp
typedef struct node {
  int stateindex = 0;
  std::vector<ITEM> items;
  std::map<std::string, int> nextnode;
} LRnode;
```

作用：

- 表示 LR 自动机中的一个状态

字段含义：

- `stateindex`
  - 状态编号
- `items`
  - 本状态的项目集
- `nextnode`
  - 转移边映射

### `PDA`

声明位置：

- `include/lr1_pda.h`

定义：

```cpp
typedef struct PDA {
  std::vector<LRnode> nodes;
} LRPDA;
```

作用：

- 表示完整 LR 自动机

字段含义：

- `nodes`
  - 状态数组

### `parse_table_item`

声明位置：

- `include/parse_table.h`

定义方式：

- 类结构

作用：

- 表示分析表的一行

内部数据：

- `state`
- `action`
- `gotos`

## 3. 全局文法表

声明位置：

- `include/symbol_table.h`
- `src/symbol_table.cpp`

### `std::vector<operators> ops`

作用：

- 存放全部优先级组

### `std::vector<std::string> terminals`

作用：

- 存放全部终结符

### `std::vector<std::string> nonterminals`

作用：

- 存放全部非终结符

### `std::vector<producer> producers`

作用：

- 存放全部产生式

说明：

- 这四张表是整个 LR 构造和代码生成阶段共享的全局语法基础

## 4. 解析阶段附加结构

这些结构不替代报告结构，而是在工程层面补充更多上下文。

### `YaccRule`

作用：

- 在 `produce` 之外保存：
  - 语义动作
  - `%prec`
  - 源码行号

字段：

- `grammar`
- `action`
- `precedence_symbol`
- `source_line`

### `PrecedenceDeclaration`

作用：

- 在填充 `ops` 前暂存：
  - 结合性
  - 优先级层级
  - 受该层级影响的符号集合

### `YaccSpecification`

作用：

- 作为 `.y` 文件解析完成后的总对象

包含：

- `definitionsSection`
- `verbatimDefinitions`
- `semanticUnion`
- `rulesSection`
- `userSubroutines`
- `startSymbol`
- `tokenOrder`
- `precedenceDeclarations`
- `tokenTypes`
- `nonterminalTypes`
- `rules`

## 5. LALR 合并结构

### `LALRResult`

声明位置：

- `include/lalr_converter.h`

字段：

- `automaton`
- `old_to_new`

作用：

- 保存 LALR 自动机
- 保存 canonical 状态到 LALR 状态的映射

## 6. 运行期符号表结构

### `SemanticSymbol`

声明位置：

- `include/symbol_table.h`

作用：

- 描述运行期 parser 内部维护的语义符号

字段：

- `name`
- `type`
- `scope_level`
- `offset`
- `is_function`
- `is_parameter`

说明：

- 这是生成 parser 的辅助结构
- 不是中期报告里 LR 构造必需的数据结构

## 7. 生成代码中的隐式结构

虽然它们不在 `include/` 中声明，但文档上应知道其存在。

### 生成头中的 `YYSTYPE`

来源：

- 若 `.y` 中声明 `%union`，则据此生成
- 否则生成一个带 `std::string lexeme` 的默认结构

### 生成头中的 `Token`

字段：

- `type`
- `lexeme`
- `line`
- `column`
- `semantic`

### 生成源中的 `SemanticValue`

作用：

- 规约和移进时使用的运行期语义栈元素

字段：

- `symbol`
- `lexeme`
- `value`

## 8. 数据流关系

可以把这些结构的流转理解为：

1. `.y` 文件
2. `YaccSpecification`
3. 全局文法表 `ops / terminals / nonterminals / producers`
4. `ITEM / LRnode / LRPDA`
5. `parse_table_item`
6. 生成后的 `YYSTYPE / Token / yyparse`

## 9. 为什么同时存在“报告结构”和“工程结构”

原因有两个：

### 原因一：保持课程要求一致

报告要求的结构名是评分、审阅和对照的重要依据，不能在工程化时丢掉。

### 原因二：工程实现需要更多上下文

例如：

- 仅用 `produce` 不能保存 action
- 仅用 `ITEM` 不能表示整个 `.y` 解析结果
- 仅用全局表不适合表达生成代码时的类型信息

所以当前实现采用：

- 核心结构保留
- 补充结构只负责携带额外元数据

这样既满足报告约束，也能让模块真正可维护。
