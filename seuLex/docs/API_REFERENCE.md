# seuLex API 参考

本文档基于当前 `include/` 和 `src/` 的实际实现整理。

## 1. 命令行入口

### `int main(int argc, char** argv)`

文件：

- `src/main.cpp`

功能：

- 解析命令行参数
- 进入生成模式或自测模式

参数：

- `argc`：参数个数
- `argv`：参数数组

行为：

- `--self-test` 调用 `SeuLexDriver::runSelfTests()`
- 否则进入普通生成模式

返回：

- 成功返回 `0`
- 参数错误或异常返回 `1`

## 2. CLI 辅助函数

### `std::string currentWorkingDirectory()`

文件：

- `src/main.cpp`

功能：

- 获取当前工作目录，用于自测定位仓库根目录

失败行为：

- `getcwd()` 失败时抛异常

## 3. `node`

定义位置：

- `include/node.h`

实现位置：

- `src/node.cpp`

### `node()`

功能：

- 构造默认非接受态

### `node(int state, bool accepttag)`

功能：

- 构造带状态编号和接受标记的节点

### `void Addoutstate(char ch, node* nd)`

功能：

- 添加一条出边

参数：

- `ch`：边字符，`'\0'` 表示 epsilon
- `nd`：目标节点

### `bool IsAccepted()`
### `bool IsAccepted() const`

功能：

- 查询是否为接受态

### `void SetAccept(bool tag)`

功能：

- 更新接受态标记

### `mulit GetNextStates(char ch)`

功能：

- 获取给定字符对应的出边迭代器

### `int GetState()`
### `int GetState() const`

功能：

- 读取状态编号

### `std::multimap<char, node*> getMultimap()`
### `std::multimap<char, node*> getMultimap() const`

功能：

- 返回当前出边表的副本

说明：

- 当前是拷贝返回，不是引用返回

### `void setNextState(myMul next)`

功能：

- 替换整个出边表

### `void Setstate(int state)`

功能：

- 更新状态编号

## 4. `nfa`

定义位置：

- `include/nfa.h`

字段：

- `node* start`
- `std::vector<node*> terminal`

作用：

- 表示单个规则 NFA 或合并后的总 NFA

## 5. 全局表

### `extern std::set<char> char_set`

作用：

- 保存所有非 epsilon 输入字符

### `extern std::map<std::string, std::string> idreTable`

作用：

- 保存 Definitions 段的命名正则定义

### `extern std::vector<nfa> nfaTable`

作用：

- 保存每条规则构造出的 NFA

### `extern std::map<int, std::string> nfaterstatetoaction`

作用：

- NFA 接受态编号到动作代码的映射

### `extern std::vector<node*> dfaterminals`

作用：

- 当前 DFA 阶段的接受态集合

### `extern std::map<int, std::string> TerStateActionTable`

作用：

- DFA 接受态编号到动作代码的映射

### `extern std::map<int, std::string> mindfareturn`

作用：

- 最小 DFA 接受态编号到动作代码的映射

## 6. `dfa`

定义位置：

- `include/dfa.h`

字段：

- `node* start`
- `std::vector<node> nodeVec`
- `std::vector<node> endNode`

### `dfa(node* st = nullptr)`

功能：

- 构造一个可选起点的 DFA 对象

### `void Eclosure(std::set<node*>& x)`

功能：

- 对 NFA 状态集合做 epsilon-closure

### `void printDFA()`

功能：

- 输出 DFA 调试信息到标准输出

## 7. `LexParser`

### `LexSpecification parseLexFile(const std::string& path) const`

文件：

- `include/lex_parser.h`
- `src/lex_parser.cpp`

功能：

- 解析 `.l` 文件并返回结构化结果

输入：

- `path`：Lex 文件路径

返回：

- `LexSpecification`

异常：

- 文件无法打开
- 缺失 `%%`
- action 块不平衡
- 规则语法非法

## 8. `REExpander`

### `std::string expandRE(const std::string& raw) const`

文件：

- `include/regex_expander.h`
- `src/regex_expander.cpp`

功能：

- 把扩展 Lex 正则展开为内部普通表示

输入：

- 原始正则字符串

返回：

- 规范化后的正则串

## 9. `NFABuilder`

### `std::string toPostfix(const std::string& infix) const`

功能：

- 中缀转后缀

### `nfa buildNFA(const std::string& postfix, const std::string& action, std::size_t priority) const`

功能：

- 用 Thompson 算法构造单规则 NFA

参数：

- `postfix`：后缀正则
- `action`：规则动作代码
- `priority`：规则优先级

### `nfa mergeNFA(const std::vector<nfa>& automata) const`

功能：

- 合并多个规则 NFA

## 10. `DFABuilder`

### `dfa subsetConstruct(const nfa& automaton) const`

文件：

- `include/dfa_builder.h`
- `src/dfa_builder.cpp`

功能：

- 用子集构造把 NFA 变成 DFA

输入：

- 合并后的 NFA

返回：

- 原始 DFA

说明：

- 若 `automaton.start == nullptr`，返回空 DFA，而不是解引用空指针

## 11. `resetGlobalTables()`

功能：

- 清空所有报告要求的全局表和内部构造状态

说明：

- 每次新生成前都应调用
- 调用后，旧 `nfa`、`nfaTable`、`dfaterminals` 中保存的 `node*` 都不再有效

## 12. `DFAMinimizer`

### `dfa minimizeDFA(const dfa& automaton) const`

文件：

- `include/dfa_minimizer.h`
- `src/dfa_minimizer.cpp`

功能：

- 最小化 DFA

说明：

- 当前实现按接受动作分组，避免错误合并

## 13. `CodeGenerator`

### `void emitLexer(const dfa& automaton, const LexSpecification& specification, const std::string& outPath) const`

功能：

- 输出最终 lexer C++ 源码

输出内容包括：

- 状态转移表
- 接受态动作分发
- `analysis(std::string yytext)`
- `next_token()`
- `tokenize(const std::string& source)`
- `tokenize_detailed(const std::string& source)`
- `input()`
- verbatim definitions
- 用户动作和子程序

补充运行时接口：

- `struct SeuLexToken`
  - `type`
  - `lexeme`
  - `line`
  - `column`
- `char yytext[]`
- `int yylineno`
- `int column`
- 若单个词素长度超过实现上限 1 MiB，生成的 scanner 会在动作执行前抛出运行时异常，避免静默截断 `yytext`

## 14. `Visualizer`

### `void dumpNFA(const nfa& automaton, const std::string& path) const`

功能：

- 输出 NFA 的 dot 文件

### `void dumpDFA(const dfa& automaton, const std::string& path) const`

功能：

- 输出 DFA 的 dot 文件

## 15. `SeuLexDriver`

### `void generate(const std::string& lexPath, const std::string& outCppPath, const std::string& dotDir) const`

功能：

- 调度整个词法生成流程

主要步骤：

1. 清理全局表
2. 解析 `.l`
3. 扩展每条规则正则
4. 中缀转后缀
5. 构造 NFA
6. 合并 NFA
7. 子集构造
8. DFA 最小化
9. 输出 dot
10. 输出 lexer

### `bool runSelfTests(const std::string& workspaceRoot) const`

功能：

- 运行内建自测

当前覆盖：

- 小样例 lexer 编译运行
- DFA 行为测试
- 非法正则异常测试
- Lex 解析器边界回归
- `minic.l` / `c99.l` 生成

## 16. 关键内部函数

以下函数不是公开 API，但在调试时非常关键。

### `expandNamedDefinitions`

文件：

- `src/regex_expander.cpp`

功能：

- 递归替换 `{NAME}`
- 检查未定义引用和循环引用

### `evaluateDFA`

文件：

- `src/code_generator.cpp`

功能：

- 在自测阶段直接驱动 DFA，检查词素识别和动作选择

### `runProcess`

文件：

- `src/code_generator.cpp`

功能：

- 以子进程运行编译器或测试可执行文件

## 17. 错误处理约定

公共入口采用 C++ 异常向上抛出：

- 文件错误
- 规格错误
- 正则非法
- 自动机构造失败
- 自测失败

命令行入口统一捕获并打印：

```text
seuLex error: ...
```
