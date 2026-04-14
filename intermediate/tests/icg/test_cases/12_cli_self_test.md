# 12_cli_self_test

- 名称：命令行自测回归
- 目的：验证 `seuIntermediate --self-test` 的官方入口仍可完整通过。
- 输入：`./build/seuIntermediate --self-test`。
- 预期输出：退出码为 `0`，六条内建自测全部输出 `ok`。
- 测试步骤：
  1. 通过 CMake 构建 `seuIntermediate`。
  2. 运行 `./build/seuIntermediate --self-test`。
  3. 将退出码、stdout、stderr 归一化后与 `expected/12_cli_self_test.txt` 比对。
