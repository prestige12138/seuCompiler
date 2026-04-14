# 01_ast_construction_basic

- 名称：AST 基础构造与类型注入
- 目的：验证 `ASTBuilder` 的节点构造、子节点拼接以及 `setNodeType` 覆盖行为。
- 输入：`icg_probe ast-shape` 在内存中构造 `int counter = 0; counter = counter + 1;` 的 AST。
- 预期输出：树形前序文本，根节点为 `NODE_PROGRAM`，赋值节点类型被改写为 `int`。
- 测试步骤：
  1. 编译 `icg_probe.cpp`。
  2. 运行 `./icg_probe ast-shape`。
  3. 将输出与 `expected/01_ast_construction_basic.txt` 做精确比对。
