# 03_symbol_scope_shadowing

- 名称：符号表作用域遮蔽
- 目的：验证全局声明、内层同名遮蔽、同层重复声明拒绝以及退栈后的恢复查找。
- 输入：`icg_probe symbol-scope` 依次声明全局 `x/g`、内层 `x/y`。
- 预期输出：内层 `x` 类型为 `char`、作用域为 `1`，退出作用域后 `x` 恢复为全局 `int`，`y` 消失。
- 测试步骤：
  1. 运行 `./icg_probe symbol-scope`。
  2. 检查遮蔽、重复拒绝和退栈恢复字段。
  3. 与 `expected/03_symbol_scope_shadowing.txt` 做精确比对。
