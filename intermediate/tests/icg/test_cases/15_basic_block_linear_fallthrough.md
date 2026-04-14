# 15_basic_block_linear_fallthrough

- 目的：验证纯顺序三地址码不会被错误切成多个基本块。
- 输入：`icg_probe basic-block-linear`，内部构造 4 条顺序语句。
- 预期输出：只有 1 个基本块，leader 为 `1`，无后继。
- 测试步骤：
  1. 运行 `./icg_probe basic-block-linear`。
  2. 检查块数量、leader 和 `successors=none`。
  3. 与 `expected/15_basic_block_linear_fallthrough.txt` 做精确比对。
