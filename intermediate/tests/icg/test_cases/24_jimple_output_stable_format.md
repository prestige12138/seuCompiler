# 24_jimple_output_stable_format

- 名称：Jimple 输出格式稳定性
- 目的：锁定类头、方法头、标签和显式跳转的文本形态。
- 输入：`icg_probe jimple-stable` 直接构造包含条件跳转和两个返回块的三地址码。
- 预期输出：包含 `.class public final SeuDemo`、`label_1`、`label_3`、`label_4` 和显式 `goto`。
- 测试步骤：
  1. 运行 `./icg_probe jimple-stable`。
  2. 检查类头、标签和跳转语句。
  3. 与 `expected/24_jimple_output_stable_format.txt` 做精确比对。
