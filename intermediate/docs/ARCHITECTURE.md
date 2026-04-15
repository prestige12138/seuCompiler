# seuIntermediate 架构

## 模块划分

- `ast_builder.*`：AST 构造与 parse-root 槽位
- `symbol_table.*`：语义符号表与作用域管理
- `tri_addr_generator.*`：从 AST 生成三地址码
- `intermediate_code.*`：IR 数据结构与格式化输出
- `target_ir_emitter.*`：从 AST + 三地址码生成 LLVM IR / Jimple 文本
- `main.cpp`：CLI 与自测入口

## 外部接口

- 上游输入：`seuYacc` 语义动作构造出的 AST
- 下游输出：
  - 稳定文本格式的三地址码
  - LLVM IR 文本
  - Jimple 文本

## 数据流

1. `ASTBuilder` 负责构 AST 和 parse-root 槽位
2. `TriAddrGenerator` 负责把 AST 转成 `IntermediateCode`
3. `intermediate_code` 提供三地址码和基本块视图
4. `target_ir_emitter` 读取 AST 元信息与基本块结构，输出 LLVM IR / Jimple
