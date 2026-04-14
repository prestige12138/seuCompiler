# 13_perf_batch_generation

- 名称：批量生成性能烟雾测试
- 目的：验证重复构造 AST 并生成 IR 时行为稳定，同时记录原始耗时供回归观察。
- 输入：`icg_probe perf-batch 2000`，重复执行算术赋值 IR 生成 `2000` 次。
- 预期输出：最终每轮 `stmtCount` 为 `3`，总语句数为 `6000`，状态为 `ok`。
- 测试步骤：
  1. 运行 `./icg_probe perf-batch 2000`。
  2. 将标准输出与 `expected/13_perf_batch_generation.txt` 做精确比对。
  3. 将原始耗时写入 `results/<run_id>/logs/13_perf_batch_generation.metrics`。
