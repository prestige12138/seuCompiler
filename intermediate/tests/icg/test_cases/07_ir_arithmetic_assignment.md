# 07_ir_arithmetic_assignment

- 名称：算术表达式赋值 IR
- 目的：验证 `1 + 2 * 3` 的临时变量生成顺序与赋值落点。
- 输入：`icg_probe ir-arithmetic` 构造 `x = 1 + 2 * 3;`。
- 预期输出：先生成乘法临时变量，再生成加法临时变量，最后赋给 `x`。
- 测试步骤：
  1. 运行 `./icg_probe ir-arithmetic`。
  2. 检查 `t1`、`t2` 的使用顺序。
  3. 与 `expected/07_ir_arithmetic_assignment.txt` 做精确比对。
