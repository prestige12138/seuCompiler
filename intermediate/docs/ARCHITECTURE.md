# seuIntermediate 架构

## 模块划分

- `ast_builder.*`：AST 构造与 parse-root 槽位
- `symbol_table.*`：语义符号表与作用域管理
- `tri_addr_generator.*`：从 AST 生成三地址码
- `intermediate_code.*`：IR 数据结构与格式化输出
- `main.cpp`：CLI 与自测入口

## 外部接口

- 上游输入：`seuYacc` 语义动作构造出的 AST
- 下游输出：稳定文本格式的三地址码
