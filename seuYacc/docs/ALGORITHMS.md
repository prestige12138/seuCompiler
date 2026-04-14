# seuYacc 算法说明

## 1. `.y` 文件解析

### 1.1 三段切分

`YaccParser::parseYaccFile()` 先扫描两个独立的 `%%` 分隔符：

- 第一段：Definitions
- 第二段：Rules
- 第三段：User subroutines

切分时会避开 `%{...%}` verbatim 区块，避免把区块内部内容误判成分隔符。

时间复杂度：

- `O(N)`
- `N` 为输入文件长度

### 1.2 定义段解析

当前实现解析的定义类指令：

- `%token`
- `%type`
- `%left`
- `%right`
- `%nonassoc`
- `%start`
- `%union`

其中：

- `%union` 支持多行块解析
- `%{...%}` 会被单独提取并原样注入生成代码
- token / nonterminal 的类型信息会保存在 `tokenTypes` / `nonterminalTypes`

### 1.3 规则段解析

规则段支持：

- 普通产生式
- quoted terminal
- `%prec`
- 终结符 / 非终结符混合 RHS
- 嵌套动作块
- mid-rule action

#### mid-rule action 的处理方式

例如：

```yacc
A : B { action } C ;
```

会被转换为：

```text
__midrule_A_k : /* empty */ { action }
A : B __midrule_A_k C
```

这样生成器只需要在“规约发生时”执行动作，而不需要在 shift 过程中插入专门的运行时钩子。

这是当前实现中保证 mid-rule action 正确性的关键。

### 1.4 语义动作翻译

代码生成阶段会把 Yacc 风格动作翻译为 C++ 代码，支持：

- `$$`
- `$1`, `$2`, ...
- `$<tag>$`
- `$<tag>1`, `$<tag>2`, ...

策略：

- 若用户显式写 `<tag>`，直接映射到 `YYSTYPE` 对应字段
- 若未显式写 `<tag>`，则根据 `%token` / `%type` 的声明进行类型推断
- 若无可用类型信息，则退回到整个 `YYSTYPE`

## 2. FIRST 集与 FOLLOW 集

### 2.1 FIRST

`computeFirstSets()` 使用经典不动点迭代：

1. 对所有终结符，`FIRST(a) = {a}`
2. 对非终结符初始化为空
3. 反复遍历所有产生式，直到不再变化

对于产生式：

```text
A -> X1 X2 ... Xn
```

按从左到右顺序累加：

- 加入 `FIRST(X1) - {epsilon}`
- 若 `X1` 可空，继续看 `X2`
- 全部可空则加入 `epsilon`

时间复杂度：

- `O(P * K * I)`
- `P` 为产生式数
- `K` 为平均 RHS 长度
- `I` 为迭代轮数

### 2.2 FOLLOW

`computeFollowSets()` 同样采用不动点迭代。

初始条件：

- 开始符号加入 `$`

对每条产生式扫描 RHS：

- 把后缀的 FIRST 信息加入当前非终结符的 FOLLOW
- 若后缀可空，则把左部 FOLLOW 继续传播下来

时间复杂度：

- `O(P * K * I)`

## 3. canonical LR(1) 构造

### 3.1 closure

对项目：

```text
[A -> alpha . B beta, a]
```

若点后是非终结符 `B`，则：

1. 计算 `FIRST(beta a)`
2. 对每个 `B -> gamma`
3. 加入 `[B -> . gamma, b]`

实现细节：

- 先构造带增广开始符的产生式集
- 使用去重集合和工作队列而不是朴素重复扫描
- 项目按 `(left, right, dotpos, predict)` 标准化

### 3.2 goto

对所有点后恰好是 `X` 的项目：

1. 把点右移
2. 对移动后的 kernel 再做 closure

### 3.3 自动机构造

`buildCanonicalPDA()`：

1. 从增广开始项目出发
2. 反复对每个状态上的点后符号执行 goto
3. 用完整 LR(1) 项集 key 去重状态

复杂度：

- 最坏指数级

这也是大文法下不适合作为默认模式的原因。

## 4. direct LALR(1) 构造

### 4.1 先构 LR(0) core 自动机

`buildLALRPDA()` 先构造不含 lookahead 的 LR(0) 自动机：

1. 只按 `(left, right, dotpos)` 表示 item
2. 用 LR(0) closure / goto 建状态图

### 4.2 在 core 上传播 lookahead

之后对每个 core item 维护 lookahead 集。

传播规则：

- 若状态中有 `A -> alpha . B beta`
- 则把 `FIRST(beta a)` 传播到 `B -> . gamma`
- 对转移边 `X`，把源状态中 `A -> alpha . X beta` 的 lookahead 传播到目标状态中 `A -> alpha X . beta`

不断传播直到不再变化。

### 4.3 为什么默认使用这个版本

对 `c99.y` 这类文法：

- canonical LR(1) 的状态膨胀会拖慢生成
- direct LALR 保留了 LALR(1) 所需的 lookahead 信息
- 代价更适合课程项目和生成器用途

## 5. LR(1) 到 LALR(1) 合并

`LALRConverter::convert()` 保留了经典做法：

1. 以 LR(0) core 为 key 给 canonical LR(1) 分组
2. 合并同组项目的 lookahead
3. 重写边关系

这个模块主要用于：

- 算法对照
- 自测时验证 direct-LALR 的合理性

## 6. ACTION / GOTO 表生成

`ParseTableBuilder::buildLR1Table()` 遍历每个状态：

### 6.1 shift 项

若存在终结符边：

- `ACTION[state, terminal] = shift target`

### 6.2 goto 项

若存在非终结符边：

- `GOTO[state, nonterminal] = target`

### 6.3 reduce 项

若 item 满足点在末尾：

- 对其 lookahead 填入 `reduce production`

若是增广开始项目：

- 填入 `accept`

## 7. 冲突处理

当前实现显式处理：

- shift/reduce
- reduce/reduce

### 7.1 shift/reduce

使用文法中的优先级与结合性：

1. 取 lookahead token 的优先级
2. 取待规约产生式的优先级
3. 若 token 优先级更高，选择 shift
4. 若产生式优先级更高，选择 reduce
5. 若相同：
   - `left` 选择 reduce
   - `right` 选择 shift
   - `nonassoc` 置为 `err`

若缺失优先级信息：

- 当前策略偏向 shift

### 7.2 reduce/reduce

当前策略：

- 选择产生式编号较小者

同时可把冲突信息输出到 `conflicts`。

## 8. 代码生成

### 8.1 头文件生成

输出内容：

- `YYSTYPE`
- `TokenKind`
- `Token`
- `yyparse` 声明

对命名终结符会做：

- C++ 标识符合法化
- 枚举成员命名规整化

### 8.2 源文件生成

输出内容：

- `%{...%}` verbatim definitions
- 第三段用户代码
- `SemanticValue`
- 运行期符号表
- 产生式元数据
- ACTION / GOTO 常量表
- token 到文法符号的映射函数
- 语义动作执行器
- `yyparse`

### 8.3 第三段用户代码的放置

第三段代码现在放在生成文件的全局作用域，并在其前面注入：

```cpp
using <generated_ns>::Token;
using <generated_ns>::YYSTYPE;
```

这样做的原因是：

- 保持用户辅助函数的全局链接行为
- 允许语义动作直接调用第三段定义的 helper
- 同时允许第三段代码复用生成头中的核心类型

## 9. 运行期解析总控

生成的 `yyparse` 使用标准 LR 栈机：

1. 状态栈初始化为 `0`
2. 语义栈为空
3. 若输入结尾没有 EOF token，则自动补 `$`
4. 查 ACTION
5. `shift`
   - 推入状态
   - 保存 token 对应语义值
6. `reduce`
   - 弹出 RHS 长度个状态和语义值
   - 执行语义动作
   - 查 GOTO
   - 推入 LHS
7. `accept`
   - 返回 `true`
8. 无合法动作
   - 返回 `false`

## 10. 自测策略

`runSelfTests()` 做三组验证：

### 10.1 表达式文法

目的：

- 验证最小闭环
- 验证 direct-LALR 与经典 LALR 合并至少在小文法上一致

### 10.2 含语义动作的文法

目的：

- 验证 `%union`
- 验证 `%type`
- 验证 typed action
- 验证第三段 helper

### 10.3 `c99.y`

目的：

- 验证真实规模文法可生成
- 验证生成出的 parser 可被 C++17 编译器接受

### 10.4 超时保护

`runProcess()` 对子进程设置 60 秒等待上限。

目的：

- 防止自测无限挂住
- 防止大文法下错误实现导致 CI 卡死

## 11. 复杂度总结

| 阶段 | 复杂度 |
|---|---|
| `.y` 解析 | `O(N)` |
| FIRST / FOLLOW | `O(P * K * I)` |
| canonical LR(1) | 最坏指数级 |
| direct LALR(1) | 取决于 LR(0) 状态数和 lookahead 传播轮数 |
| 分析表生成 | `O(S * I * log M)` |
| 代码生成 | `O(S * T + P)` |

记号说明：

- `N`：文件长度
- `P`：产生式数量
- `K`：平均 RHS 长度
- `I`：不动点迭代轮数
- `S`：状态数
- `T`：终结符数
- `M`：单行表项数
