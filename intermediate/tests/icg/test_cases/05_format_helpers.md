# 05_format_helpers

- 名称：格式化辅助函数
- 目的：验证 `astNodeTypeName`、`triOpName`、`formatTriAddrStmt`、`dumpIntermediateCode` 的稳定文本输出。
- 输入：`icg_probe formatters` 直接构造枚举值与三地址语句样本。
- 预期输出：节点名、操作名和标准化后的 IR 文本均固定。
- 测试步骤：
  1. 运行 `./icg_probe formatters`。
  2. 检查四行输出。
  3. 与 `expected/05_format_helpers.txt` 做精确比对。
