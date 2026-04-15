# seuYacc API 参考

本文档分为两部分：

- 公开接口
- 关键内部函数

公开接口以 `include/` 为准，关键内部函数以当前实现中承担核心逻辑的函数为准。

## 1. 命令行入口

### `main(int argc, char** argv)`

文件：

- `src/main.cpp`

功能：

- 解析命令行参数
- 调用 `SeuYaccDriver`
- 输出生成结果
- 捕获异常并打印错误信息

支持形式：

```bash
./seuYacc <yacc-file> [generated-parser.cpp] [generated-tokens.h] [lalr|lr1]
./seuYacc --self-test
```

## 2. `YaccParser`

### `YaccSpecification parseYaccFile(const std::string& path) const`

文件：

- `include/yacc_parser.h`
- `src/yacc_parser.cpp`

功能：

- 读取 `.y` 文件
- 切分三段
- 提取定义、规则和用户代码
- 返回结构化文法对象

输入：

- `path`：`.y` 文件路径

返回：

- `YaccSpecification`

异常：

- 文件打开失败
- 缺失 `%%`
- 动作块或 `%union` 块未闭合
- 产生式语法非法

## 3. `SymbolTableManager`

### `void reset()`

功能：

- 清空所有全局文法表和运行期表

### `void registerTerminal(const std::string& symbol)`

功能：

- 注册终结符
- 自动分配 token 编号

规则：

- `$` 编号固定为 `0`
- 单字符 quoted terminal 使用字符 ASCII 编号
- 具名 token 从 `256` 开始分配

### `void registerNonterminal(const std::string& symbol)`

功能：

- 注册非终结符
- 分配内部编号

### `void addOperatorGroup(const operators& group, const std::vector<std::string>& precedence_symbols)`

功能：

- 存储优先级组
- 建立 `symbol -> (level, associativity)` 映射

### `void addProducer(const producer& production)`

功能：

- 向全局 `producers` 追加产生式

### `void setStartSymbol(const std::string& symbol)`

功能：

- 设置开始符号

### `const std::string& startSymbol() const`

功能：

- 读取开始符号

### `void setTokenType(const std::string& symbol, const std::string& type_name)`

功能：

- 记录 `%token <type>`

### `void setNonterminalType(const std::string& symbol, const std::string& type_name)`

功能：

- 记录 `%type <type>`

### `bool isTerminal(const std::string& symbol) const`

功能：

- 查询符号是否是终结符

### `bool isNonterminal(const std::string& symbol) const`

功能：

- 查询符号是否是非终结符

### `int terminalId(const std::string& symbol) const`

功能：

- 获取终结符编号

异常：

- 未知终结符

### `int nonterminalId(const std::string& symbol) const`

功能：

- 获取非终结符编号

异常：

- 未知非终结符

### `std::string symbolType(const std::string& symbol) const`

功能：

- 获取符号的声明类型

### `std::pair<int, std::string> precedenceOf(const std::string& symbol) const`

功能：

- 获取优先级和结合性

### `void enterScope()`

功能：

- 进入运行期作用域

### `void exitScope()`

功能：

- 退出运行期作用域

### `void declareSymbol(const SemanticSymbol& symbol)`

功能：

- 声明运行期语义符号

### `const SemanticSymbol* lookupSymbol(const std::string& name) const`

功能：

- 自内向外查找符号

### `const std::vector<std::unordered_map<std::string, SemanticSymbol>>& semanticScopes() const`

功能：

- 取回全部运行期作用域

## 4. `LR1Builder`

### `std::map<std::string, std::set<std::string>> computeFirstSets() const`

功能：

- 计算当前全局文法的 FIRST 集

### `std::map<std::string, std::set<std::string>> computeFollowSets(const std::map<std::string, std::set<std::string>>& first_sets, const std::string& start_symbol) const`

功能：

- 基于 FIRST 集计算 FOLLOW 集

### `std::vector<ITEM> closure(const std::vector<ITEM>& kernel, const std::map<std::string, std::set<std::string>>& first_sets, const std::string& start_symbol) const`

功能：

- 计算 LR(1) closure

### `std::vector<ITEM> gotoSet(const std::vector<ITEM>& items, const std::string& symbol, const std::map<std::string, std::set<std::string>>& first_sets, const std::string& start_symbol) const`

功能：

- 计算 LR(1) goto

### `LRPDA buildLALRPDA(const std::string& start_symbol, std::map<std::string, std::set<std::string>>* first_sets = nullptr, std::map<std::string, std::set<std::string>>* follow_sets = nullptr) const`

功能：

- 直接构造 LALR(1) 自动机

说明：

- 这是默认 `lalr` 模式使用的核心入口

### `LRPDA buildCanonicalPDA(const std::string& start_symbol, std::map<std::string, std::set<std::string>>* first_sets = nullptr, std::map<std::string, std::set<std::string>>* follow_sets = nullptr) const`

功能：

- 构造 canonical LR(1) 自动机

说明：

- 用于 `lr1` 模式和对照验证

## 5. `LALRConverter`

### `LALRResult convert(const LRPDA& canonical) const`

功能：

- 将 canonical LR(1) 自动机按 LR(0) core 合并

输入：

- `canonical`：规范 LR(1) 自动机

返回：

- `LALRResult`

## 6. `parse_table_item`

### `explicit parse_table_item(LRnode x)`

功能：

- 用 LR 状态初始化一个分析表行

### `std::map<std::string, std::string> GetAction() const`

功能：

- 返回 ACTION 列

### `std::map<std::string, int> GetGoto() const`

功能：

- 返回 GOTO 列

### `void AddtoAction(const std::string& ter, const std::string& rs)`

功能：

- 写入 ACTION 条目

### `void AddtoGoto(const std::string& nonter, int s)`

功能：

- 写入 GOTO 条目

### `int State() const`

功能：

- 获取本行对应的状态号

## 7. `ParseTableBuilder`

### `std::vector<parse_table_item> buildLR1Table(const LRPDA& automaton, const YaccSpecification& specification, const std::string& start_symbol, std::vector<std::string>* conflicts = nullptr) const`

功能：

- 从 LR(1) 自动机构建分析表

输出：

- 表行数组
- 可选冲突列表

### `std::vector<parse_table_item> buildLALRTable(const LRPDA& automaton, const YaccSpecification& specification, const std::string& start_symbol, std::vector<std::string>* conflicts = nullptr) const`

功能：

- 从 LALR(1) 自动机构建分析表

说明：

- 当前实现直接复用 `buildLR1Table`
- 因为 direct-LALR 的状态已经携带最终 lookahead

## 8. `ParserCodeGenerator`

### `void emitParser(const std::vector<parse_table_item>& table, const LRPDA& automaton, const YaccSpecification& specification, const std::string& start_symbol, const std::string& out_cpp_path, const std::string& out_header_path) const`

功能：

- 输出完整 parser 源文件和头文件

行为：

- 生成 `YYSTYPE`
- 生成 token 枚举
- 生成 `Token`
- 生成 ACTION / GOTO 常量
- 生成 `yyparse`
- 生成动作执行器
- 嵌入用户代码

## 9. `SeuYaccDriver`

### `void generate(const std::string& yacc_path, const std::string& out_cpp_path, const std::string& out_header_path, const std::string& mode) const`

功能：

- 调度整个生成流程

步骤：

1. 重置符号表
2. 解析 `.y`
3. 填充全局文法表
4. 根据模式构造自动机
5. 构建分析表
6. 生成代码

### `bool runSelfTests(const std::string& workspace_root) const`

功能：

- 运行内建生成测试

行为：

- 生成并编译小表达式文法
- 生成并编译语义动作文法
- 生成并编译 `minic.y`

## 10. 关键内部函数

这些函数不暴露在头文件中，但在阅读和调试时很关键。

### `parseRules`

文件：

- `src/yacc_parser.cpp`

功能：

- 解析 Rules 段
- 识别 final action 与 mid-rule action
- 生成合成 mid-rule 非终结符

### `translateSemanticAction`

文件：

- `src/parse_table.cpp`

功能：

- 把 Yacc 风格动作翻译成生成 parser 中可执行的 C++ 语义动作代码

### `runProcess`

文件：

- `src/parse_table.cpp`

功能：

- 以子进程方式运行编译器或测试程序
- 自带 60 秒超时

### `resolveConflict`

文件：

- `src/parse_table.cpp`

功能：

- 处理 shift/reduce 与 reduce/reduce 冲突

## 11. 错误处理约定

公共入口采用 C++ 异常向上抛出：

- 文件错误
- 语法定义错误
- 符号未定义
- 生成失败
- 自测失败

命令行入口统一捕获并打印：

```text
seuYacc error: ...
```
