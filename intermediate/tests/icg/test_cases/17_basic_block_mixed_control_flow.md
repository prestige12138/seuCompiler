# 17_basic_block_mixed_control_flow

- 目的：验证复杂控制流下的基本块划分、循环回边和后继块计算。
- 输入：`icg_probe basic-block-mixed`，内部复用模块现有 `if/while/call` 风格 IR。
- 预期输出：8 个基本块，leaders 为 `1,2,3,6,7,8,9,12`。
- 测试步骤：
  1. 运行 `./icg_probe basic-block-mixed`。
  2. 检查循环头 `B5`、回边 `B7 -> B5` 和退出块 `B8`。
  3. 与 `expected/17_basic_block_mixed_control_flow.txt` 做精确比对。
