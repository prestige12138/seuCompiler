# seuLex 数据结构说明

## 1. 命名策略

当前实现严格保留了中期报告中定义的名称：

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

这些名字统一放在 `namespace seu_lex` 中。

## 2. `node`

定义位置：

- `include/node.h`

实现位置：

- 相关行为主要在 `src/node.cpp`

### 字段

| 字段 | 类型 | 含义 |
|---|---|---|
| `label` | `int` | 状态编号 |
| `accepted` | `bool` | 是否为接受态 |
| `outstate` | `std::multimap<char, node*>` | 出边表 |

### 语义说明

- `'\0'` 表示 epsilon
- 允许同一字符有多条出边，因此使用 `multimap`
- `getMultimap()` 当前返回副本，不是引用

### 生命周期

NFA 阶段：

- `node` 由内部 arena `g_nodeArena` 分配
- 调用 `resetGlobalTables()` 会整体释放该 arena，对应 `node*` 全部失效

DFA 阶段：

- `node` 存在于 `dfa::nodeVec` 中，按值存储

## 3. `mulit`

定义位置：

- `include/node.h`

定义：

```cpp
typedef std::multimap<char, node*>::iterator mulit;
```

作用：

- 历史兼容 / 报告规定的迭代器别名

## 4. `myMul`

定义位置：

- `include/node.h`

定义：

```cpp
typedef std::multimap<char, node*> myMul;
```

作用：

- 历史兼容 / 报告规定的转移表别名

## 5. `nfa`

定义位置：

- `include/nfa.h`

字段：

| 字段 | 类型 | 含义 |
|---|---|---|
| `start` | `node*` | 起始状态 |
| `terminal` | `std::vector<node*>` | 接受态集合 |

作用：

- 表示一条规则的 NFA
- 也表示合并后的总 NFA

## 6. `dfa`

定义位置：

- `include/dfa.h`

字段：

| 字段 | 类型 | 含义 |
|---|---|---|
| `start` | `node*` | 起始状态指针 |
| `nodeVec` | `std::vector<node>` | 全部 DFA 状态 |
| `endNode` | `std::vector<node>` | 接受态拷贝 |

### 使用说明

- `start` 指向 `nodeVec` 内部元素
- 转移边目标也指向 `nodeVec` 内部元素

### 工程注意

由于 `nodeVec` 内部元素地址会受扩容影响，因此建立指针链接时必须注意容器稳定性。

## 7. `LexRule`

定义位置：

- `include/lex_parser.h`

字段：

| 字段 | 类型 | 含义 |
|---|---|---|
| `regex` | `std::string` | Rules 段原始正则 |
| `action` | `std::string` | 对应动作代码 |
| `priority` | `std::size_t` | 规则优先级，越小越靠前 |
| `expandedRegex` | `std::string` | 展开后的规范化正则 |
| `postfixRegex` | `std::string` | 后缀形式 |

作用：

- 作为一条规则在解析、展开、构造、生成之间的载体

## 8. `LexSpecification`

定义位置：

- `include/lex_parser.h`

字段：

| 字段 | 类型 | 含义 |
|---|---|---|
| `definitionsSection` | `std::string` | Definitions 原文 |
| `verbatimDefinitions` | `std::string` | `%{...%}` 聚合内容 |
| `rulesSection` | `std::string` | Rules 原文 |
| `userSubroutines` | `std::string` | 用户子程序原文 |
| `rules` | `std::vector<LexRule>` | 解析后的规则集 |

作用：

- 这是 `LexParser` 的总输出对象

## 9. 全局表

### `char_set`

类型：

```cpp
std::set<char>
```

作用：

- 保存所有非 epsilon 字符
- 为子集构造、最小化、代码生成提供字母表基础

### `idreTable`

类型：

```cpp
std::map<std::string, std::string>
```

作用：

- 保存 Definitions 段中的命名正则

### `nfaTable`

类型：

```cpp
std::vector<nfa>
```

作用：

- 保存所有规则对应的单 NFA

### `nfaterstatetoaction`

类型：

```cpp
std::map<int, std::string>
```

作用：

- NFA 接受态编号到动作代码的映射

### `dfaterminals`

类型：

```cpp
std::vector<node*>
```

作用：

- 当前 DFA 阶段的接受态集合

### `TerStateActionTable`

类型：

```cpp
std::map<int, std::string>
```

作用：

- 原始 DFA 接受态编号到动作代码的映射

### `mindfareturn`

类型：

```cpp
std::map<int, std::string>
```

作用：

- 最小 DFA 接受态编号到动作代码的映射

### `nfaPriorityTableInternal`

类型：

```cpp
std::map<int, std::size_t>
```

定义位置：

- `src/lex_state.cpp`
- 私有声明位于 `src/internal/lex_state.h`

作用：

- 记录 NFA 接受态到规则优先级的映射
- 供确定化阶段恢复 Lex 规则优先级

## 10. 内部辅助结构

这些结构不是中期报告强制命名的一部分，但对理解当前实现很重要。

### `RegexAst`

定义位置：

- `src/regex_expander.cpp`

字段：

- `kind`
- `literal`
- `charset`
- `left`
- `right`

作用：

- 表示扩展正则解析后的抽象语法树

### `Fragment`

定义位置：

- `src/nfa_constructor.cpp`

字段：

- `start`
- `accept`

作用：

- Thompson 构造阶段的栈元素

### NFA 构造状态池

定义位置：

- `src/lex_state.cpp`

作用：

- 统一持有 NFA 阶段分配的 `node`
- 在 `resetGlobalTables()` 时集中释放

### `Expectation`

定义位置：

- `src/code_generator.cpp` 中 `SeuLexDriver::runSelfTests()`

字段：

- `lexeme`
- `expected`

作用：

- 自测中描述一个词素及其期望识别结果

## 11. 数据流关系

整体数据流可以概括为：

1. `.l` 文件
2. `LexSpecification`
3. `LexRule`
4. `nfaTable`
5. 合并 `nfa`
6. `dfa`
7. 最小 `dfa`
8. 生成出的 lexer.cpp

## 12. 当前数据结构设计评价

优点：

- 报告要求的结构全部保留
- 对自动机构造过程直观
- 与课程理论概念对应清晰

不足：

- 全局表较多，阶段耦合偏强
- `node*` 和按值存储的 `dfa::nodeVec` 同时存在，要求实现上非常注意生命周期
- 虽然扩展正则、NFA 构造、DFA 构造已经拆成独立模块，但阶段之间仍通过共享全局表协同
