# 11_generate_empty_root

- 名称：空根节点生成
- 目的：验证 `generate(nullptr)` 与空 `NODE_PROGRAM` 都返回空 IR，而不是崩溃。
- 输入：`icg_probe generate-empty`。
- 预期输出：两种输入的 `stmtCount` 均为 `0`，格式化文本均为空。
- 测试步骤：
  1. 运行 `./icg_probe generate-empty`。
  2. 检查计数和空文本标志。
  3. 与 `expected/11_generate_empty_root.txt` 做精确比对。
