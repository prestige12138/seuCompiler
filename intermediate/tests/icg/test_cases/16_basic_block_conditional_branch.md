# 16_basic_block_conditional_branch

- 目的：验证条件跳转目标、跳转后继语句和 `return` 终止都能正确形成 leader。
- 输入：`icg_probe basic-block-conditional`，内部构造带条件分支和双 `return` 的 IR。
- 预期输出：4 个基本块，leaders 为 `1,2,3,5`。
- 测试步骤：
  1. 运行 `./icg_probe basic-block-conditional`。
  2. 检查 `B1 -> B3, B2` 和 `B2 -> B4`。
  3. 与 `expected/16_basic_block_conditional_branch.txt` 做精确比对。
