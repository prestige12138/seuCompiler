# 14_basic_block_empty_code

- 目的：验证空 `IntermediateCode` 不会伪造基本块。
- 输入：`icg_probe basic-block-empty`，内部直接传入空代码对象。
- 预期输出：`block_count=0`，`leaders=<empty>`。
- 测试步骤：
  1. 运行 `./icg_probe basic-block-empty`。
  2. 检查输出为空分块结果。
  3. 与 `expected/14_basic_block_empty_code.txt` 做精确比对。
