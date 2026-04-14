# 19_basic_block_invalid_target

- 目的：验证非法跳转目标不会导致崩溃，并以固定文本形式输出。
- 输入：`icg_probe basic-block-invalid-target`，内部构造 `goto 99`。
- 预期输出：后继块显示为 `invalid(99)`，下一条语句仍被切成新块。
- 测试步骤：
  1. 运行 `./icg_probe basic-block-invalid-target`。
  2. 检查 leaders 为 `10,20`。
  3. 检查首块后继为 `invalid(99)`。
  4. 与 `expected/19_basic_block_invalid_target.txt` 做精确比对。
