# seuYacc 文档总览

这组文档面向两类读者：

- 审阅代码结构和算法正确性的开发者
- 需要把 `seuYacc` 接入 `seuLex` / 前端总控程序的集成人员

## 文档索引

- [ARCHITECTURE.md](./ARCHITECTURE.md)
  - 模块职责、调用链、生成流程
- [ALGORITHMS.md](./ALGORITHMS.md)
  - 文法解析、FIRST/FOLLOW、LR(1)、LALR(1)、冲突处理、代码生成
- [API_REFERENCE.md](./API_REFERENCE.md)
  - 头文件公开接口与关键内部函数说明
- [DATA_STRUCTURES.md](./DATA_STRUCTURES.md)
  - 中期报告数据结构复用情况和新增辅助结构
- [DEPENDENCY_GRAPH.md](./DEPENDENCY_GRAPH.md)
  - Mermaid 模块依赖图与生成流水线图
- [DEVELOPMENT_PROCESS.md](./DEVELOPMENT_PROCESS.md)
  - 设计决策、验证路径、已知边界

## 推荐阅读顺序

如果你第一次读 `seuYacc`：

1. 先看 [ARCHITECTURE.md](./ARCHITECTURE.md)
2. 再看 [DATA_STRUCTURES.md](./DATA_STRUCTURES.md)
3. 然后看 [ALGORITHMS.md](./ALGORITHMS.md)
4. 最后查 [API_REFERENCE.md](./API_REFERENCE.md)

如果你要调试生成错误：

1. 先看 [ALGORITHMS.md](./ALGORITHMS.md)
2. 再看 [API_REFERENCE.md](./API_REFERENCE.md)
3. 同时参考 [DEPENDENCY_GRAPH.md](./DEPENDENCY_GRAPH.md)

## 当前模块结论

`seuYacc` 当前已经具备如下交付属性：

- 可独立构建
- 可独立运行
- 可生成可编译 parser
- 支持 `lr1` 和 `lalr` 两种模式
- 支持 `%union`、typed action、mid-rule action、第三段用户代码
- 具备自测与 `ctest` 验证入口

## 与 seuLex 的边界

`seuYacc` 当前不负责词法分析过程本身，只假定上游能提供：

- 与生成头文件 token 编号一致的 token 类型
- token 文本 `lexeme`
- 可选的语义值 `YYSTYPE semantic`

因此整链路接入时，`seuLex` 或手写 lexer 的职责是构造 `std::vector<Token>`，`seuYacc` 的职责是消费它并完成语法分析。

当前仓库内的最小正式联通样例位于：

- [integration/tests/pipeline/run_pipeline_test.sh](/Users/llawliet/代码/seuCompiler/integration/tests/pipeline/run_pipeline_test.sh)
