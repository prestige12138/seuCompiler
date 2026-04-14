# 02_parse_root_handoff

- 名称：Parse Root 交接
- 目的：验证 `setParseRoot`、`getParseRoot`、`releaseParseRoot` 的所有权交接语义。
- 输入：`icg_probe parse-root` 在内存中构造 `return 0;` 根节点并执行导出/释放。
- 预期输出：首次读取与释放均返回同一指针，释放后全局槽位清空，再次释放得到空指针。
- 测试步骤：
  1. 运行 `./icg_probe parse-root`。
  2. 检查四个布尔字段均为 `yes`。
  3. 与 `expected/02_parse_root_handoff.txt` 做精确比对。
