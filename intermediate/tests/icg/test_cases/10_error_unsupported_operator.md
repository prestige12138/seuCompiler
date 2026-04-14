# 10_error_unsupported_operator

- 名称：不支持运算符错误路径
- 目的：验证 `TriAddrGenerator` 对报告外算术运算符会抛出明确异常。
- 输入：`icg_probe ir-unsupported-op` 构造 `2 ^ 3`。
- 预期输出：错误文本为 `unsupported arithmetic operator: ^`。
- 测试步骤：
  1. 运行 `./icg_probe ir-unsupported-op`。
  2. 捕获并输出异常文本。
  3. 与 `expected/10_error_unsupported_operator.txt` 做精确比对。
