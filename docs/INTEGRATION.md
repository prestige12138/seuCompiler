# 三模块集成设计

## 数据流

最小整链路如下：

1. `seuLex` 根据 `.l` 生成 scanner
2. `seuYacc` 根据 `.y` 生成 parser 和 token ABI 头
3. scanner 在 `--token-header <generated_tokens.h>` 模式下执行 `tokenize_for_parser(source)`
4. `seuYacc` 生成的 `yyparse(tokens)` 执行语义动作并构建 AST
5. 开始符号归约完成时通过 `seu_icg::setParseRoot(...)` 导出根节点
6. 外层驱动通过 `seu_icg::releaseParseRoot()` 取得 AST
7. `seu_icg::TriAddrGenerator` 生成三地址码

## Token 契约

`seuLex` 详细 token：

```cpp
struct SeuLexToken {
  int type;
  std::string lexeme;
  int line;
  int column;
};
```

`seuYacc` parser token：

```cpp
struct Token {
  int type;
  std::string lexeme;
  int line;
  int column;
  YYSTYPE semantic;
};
```

统一 ABI 规则：

- 命名 token：使用 `generated_tokens.h` 中的枚举值
- 单字符终结符：直接使用 ASCII
- EOF：`0`
- `lexeme/line/column` 由 generated lexer 原样填入 parser `Token`
- `semantic` 通过 `.l` 动作中的 `yylval` 直接传入 parser `Token`
- `generated_tokens.h` 稳定导出：
  - `SEU_YACC_TOKEN_NAMESPACE`
  - `SEU_YACC_TOKEN_TYPE`
  - `SEU_YACC_SEMANTIC_TYPE`
- 对需要把指针语义绑定到最终 token 存储的规则，可在 `.l` 中定义 `SEU_LEX_FINALIZE_PARSER_TOKEN(token_ref, lex_token_ref)`

典型直连接口：

```cpp
void begin_lexing(const std::string& source);
std::vector<parser_ns::Token> tokenize_for_parser(const std::string& source);
bool yyparse(const std::vector<parser_ns::Token>& tokens);
```

若使用逐 token API，则先调用 `begin_lexing(source)`，再调用 `lex_one_parser_token(...)` 或 `next_token()`。
若在 `.l` 中定义 `SEU_LEX_FINALIZE_PARSER_TOKEN(token_ref, token_view_ref)`，第二个参数只保证暴露稳定的 `type/lexeme/line/column` 视图；
批量模式下它可能是最终存储中的 parser token，而不是原始 `SeuLexToken` 局部对象。

## AST 交接

当前稳定方案保持 parser 返回值为 `bool`，AST 通过全局槽位交接：

```cpp
const bool ok = parser_ns::yyparse(tokens);
if (!ok) { /* parse error */ }
seu_icg::ASTNode* root = seu_icg::releaseParseRoot();
```

这避免了修改现有 parser 总控函数签名，也与当前 `intermediate` 设计保持一致。

## 生成产物约定

- `seuYacc` 生成的 parser `.cpp` 现在对生成头使用相对 include
- `seuLex` 可在 ABI 模式下对 parser token 头使用相对 include
- 集成样例产物统一落在临时结果目录，而不是源码树根目录
- 顶层 `CMakeLists.txt` 已把三模块和整链路测试纳入统一入口

## 安全与适用边界

- `seuLex` 和 `seuYacc` 都会把规范文件中的用户动作、嵌入代码和用户子程序原样写入生成的 C++。
- 当前测试脚本会继续把这些生成产物编译并执行，因此整条链路默认假设 `.l` / `.y` 输入是可信的。
- 如果未来需要支持在线评测、外部提交或其他不可信文法场景，必须把“代码生成 + 编译 + 运行”整体迁移到容器或沙箱中。

## 当前最小样例

最小工作样例位于：

- [integration/tests/pipeline/test_cases/pipeline_expr.l](/Users/llawliet/代码/seuCompiler/integration/tests/pipeline/test_cases/pipeline_expr.l)
- [integration/tests/pipeline/test_cases/pipeline_expr.y](/Users/llawliet/代码/seuCompiler/integration/tests/pipeline/test_cases/pipeline_expr.y)
- [integration/tests/pipeline/run_pipeline_test.sh](/Users/llawliet/代码/seuCompiler/integration/tests/pipeline/run_pipeline_test.sh)
