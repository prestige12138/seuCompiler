# seuLex 架构说明

## 1. 总体目标

`seuLex` 的目标是把 `.l` 规格文件编译成一个新的、独立的 C++17 词法分析器，而不是在运行时解释 `.l` 文件。

因此它本质上是一个“词法分析器生成器”。

完整流水线如下：

1. 解析 `.l` 文件三段结构
2. 提取命名正则定义和规则
3. 将扩展正则表达式规范化
4. 转换为后缀表达式
5. 构造每条规则的 Thompson NFA
6. 合并多个 NFA
7. 进行子集构造得到 DFA
8. 最小化 DFA
9. 输出独立的 C++ lexer 代码和 dot 图

## 2. 模块划分

### `lex_parser.*`

职责：

- 读取 `.l` 文件
- 切分 Definitions / Rules / User subroutines
- 提取 `%{...%}`
- 解析 Definitions 段中的命名正则
- 解析 Rules 段中的 `regex + action`

输出：

- `LexSpecification`

### `regex_expander.*`

职责：

- 负责扩展正则展开
- 负责命名定义递归替换
- 负责扩展 RE 的递归下降解析
- 负责把 `RegexAst` 序列化为内部普通 RE

### `nfa_constructor.*`

职责：

- 负责中缀转后缀
- 负责 Thompson NFA 构造
- 负责多个 NFA 的合并

### `lex_state.*`

职责：

- 负责报告要求的全局表定义
- 负责 `resetGlobalTables()`
- 负责 NFA 构造期共享状态的底层持有
- 通过私有头 `src/internal/lex_state.h` 暴露受控内部契约

### `node.cpp`

职责：

- 负责 `node` 的报告兼容实现
- 把基础数据结构实现和 NFA 构造逻辑分离

### `dfa_builder.*`

职责：

- 负责 `dfa` 的核心行为实现
- 负责 epsilon-closure
- 负责子集构造
- 负责把 NFA 接受态动作映射到 DFA 接受态

### `dfa_minimizer.*`

职责：

- 负责 DFA 最小化
- 保持接受态动作语义不丢失

### `code_generator.*`

职责：

- 输出独立 lexer 源码
- 输出 NFA / DFA dot 可视化
- 提供 `SeuLexDriver`
- 提供内建自测

### `main.cpp`

职责：

- 提供命令行入口
- 调用 `SeuLexDriver`

## 3. 主调用链

生产生成路径：

1. `main`
2. `SeuLexDriver::generate`
3. `LexParser::parseLexFile`
4. `REExpander::expandRE`
5. `NFABuilder::toPostfix`
6. `NFABuilder::buildNFA`
7. `NFABuilder::mergeNFA`
8. `DFABuilder::subsetConstruct`
9. `DFAMinimizer::minimizeDFA`
10. `Visualizer::dumpNFA / dumpDFA`
11. `CodeGenerator::emitLexer`

自测路径：

1. `main`
2. `SeuLexDriver::runSelfTests`
3. 生成小样例 lexer
4. 直接在内存中对 DFA 做行为验证
5. 生成 `minic.l` 的输出文件

## 4. 设计决策

### 保留报告原始命名

例如：

- `node`
- `nfa`
- `dfa`
- `idreTable`
- `nfaTable`
- `dfaterminals`

这些名字保留不变，但统一放进 `namespace seu_lex`，用来平衡：

- 课程报告一致性
- 工程化命名污染控制

### 使用 POSIX API 而不是 `std::filesystem`

当前实现选择：

- `mkdir`
- `mkdtemp`
- `stat`
- `getcwd`
- `fork`
- `execvp`
- `waitpid`

原因：

- 兼容当前编译环境
- 避免 `std::filesystem` 在工具链不完整时带来的编辑器和编译错误

### DFA 最小化必须保留动作语义

`seuLex` 不是单纯做语言识别，因此最小化时不能只按“接受/不接受”划分状态。

当前策略是：

- 接受态按 action 字符串分组
- 不同 action 的接受态绝不合并

这属于词法生成器实现上最关键的正确性约束之一。

## 5. 当前结构的优点

- 模块边界已经明显好于单文件实现
- CLI、解析、自动机构造、最小化、生成输出已有分层
- 构建、可视化、自测链路都在模块内部自洽

## 6. 当前结构的主要弱点

和 `seuYacc` 相比，`seuLex` 当前仍有两个明显设计短板：

### `nfa_constructor.cpp` 过重

目前 `nfa_constructor.cpp` 已经收敛为：

- 中缀转后缀
- Thompson NFA 构造
- 多个 NFA 合并

这使得 Lex 核心构造链的职责边界已经接近 `seuYacc` 当前的模块粒度。

### 全局状态耦合偏强

当前多个阶段共享并直接读写：

- `char_set`
- `idreTable`
- `nfaTable`
- `nfaterstatetoaction`
- `TerStateActionTable`
- `mindfareturn`
- `nfaPriorityTableInternal`

这让生成链是“可工作”的，但不如 `seuYacc` 那样便于做更细粒度的模块测试。

### `code_generator.cpp` 仍然偏大

当前它同时承担：

- dot 可视化
- 生成 lexer 代码
- 自测驱动
- 若干 POSIX 运行辅助函数

这不会影响正确性，但仍然是后续继续精拆时最值得处理的文件。

## 7. 与 seuYacc 的设计水平对比

结论先说：

- 现在两者已经比较接近
- `seuYacc` 仍略占优势，但差距已经明显缩小

当前剩余差距主要不是功能缺失，而是共享状态管理方式：

- `seuLex` 仍依赖多张全局表串联阶段
- `code_generator.cpp` 仍承担了较多辅助职责
- `seuYacc` 在对象化封装和阶段数据传递上略更自然

如果把两者放在同一工程成熟度坐标上看：

- `seuYacc` 更接近“对象化的生成器框架”
- `seuLex` 已达到“模块化且可维护的生成器实现”

## 8. 推荐的后续重构方向

如果后续继续提升 `seuLex` 设计质量，优先顺序建议是：

1. 缩减全局表共享面，改为阶段性对象传递
2. 继续拆分 `code_generator.cpp` 的运行辅助和自测逻辑
3. 为 `regex_expander` / `nfa_constructor` / `dfa_builder` 增加更细的单元测试
4. 在不破坏报告命名的前提下，增加更强的只读接口与内部封装
