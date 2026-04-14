# 06_ir_constant_assignment

- 名称：常量初始化与返回 IR
- 目的：验证变量声明初始化会生成赋值语句，并能继续生成 `return`。
- 输入：`icg_probe ir-constant-assign` 构造 `int x = 42; return x;`。
- 预期输出：两条 IR，分别为 `x = 42` 和 `return x`。
- 测试步骤：
  1. 运行 `./icg_probe ir-constant-assign`。
  2. 检查语句编号和输出顺序。
  3. 与 `expected/06_ir_constant_assignment.txt` 做精确比对。
