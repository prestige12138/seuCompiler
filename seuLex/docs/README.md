# seuLex 文档总览

这组文档面向两类读者：

- 需要审阅 `seuLex` 设计、算法和实现边界的开发者
- 需要把 `seuLex` 与 `seuYacc` 或上层编译器前端联调的集成人员

## 文档索引

- [ARCHITECTURE.md](./ARCHITECTURE.md)
  - 模块职责、生成链路、核心设计决策
- [ALGORITHMS.md](./ALGORITHMS.md)
  - Lex 解析、扩展正则展开、NFA / DFA / 最小化、代码生成
- [API_REFERENCE.md](./API_REFERENCE.md)
  - 公开接口与关键内部函数说明
- [DATA_STRUCTURES.md](./DATA_STRUCTURES.md)
  - 中期报告结构、全局表和辅助结构说明
- [DEPENDENCY_GRAPH.md](./DEPENDENCY_GRAPH.md)
  - Mermaid 模块依赖图和流水线图
- [DEVELOPMENT_PROCESS.md](./DEVELOPMENT_PROCESS.md)
  - 模块演化过程、验证方式、已知边界

## 推荐阅读顺序

如果你第一次读 `seuLex`：

1. 先看 [ARCHITECTURE.md](./ARCHITECTURE.md)
2. 再看 [DATA_STRUCTURES.md](./DATA_STRUCTURES.md)
3. 然后看 [ALGORITHMS.md](./ALGORITHMS.md)
4. 最后查 [API_REFERENCE.md](./API_REFERENCE.md)

如果你要调试词法生成问题：

1. 先看 [ALGORITHMS.md](./ALGORITHMS.md)
2. 再看 [API_REFERENCE.md](./API_REFERENCE.md)
3. 同时参考 [DEPENDENCY_GRAPH.md](./DEPENDENCY_GRAPH.md)

## 当前模块结论

`seuLex` 当前已经具备如下交付属性：

- 可独立构建
- 可独立运行
- 可生成独立 C++ lexer
- 支持从 `.l` 到最小 DFA 的完整生成链
- 支持 dot 可视化输出
- 具备内建自测和 `ctest` 入口

## 与 seuYacc 的边界

`seuLex` 当前负责：

- 读取源代码字符流
- 识别 token
- 返回 token 类型

`seuLex` 当前不负责：

- 语法分析
- 语义动作调度
- AST 构造

如果要和 `seuYacc` 联调，当前最合适的集成方向是：

- 保持 `seuLex` 负责 token 识别
- 让 `seuYacc` 负责消费 token 序列并进行 LR 语法分析
