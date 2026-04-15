# 26_jimple_invalid_target_fallback

- 名称：Jimple 非法目标兜底块
- 目的：验证非法跳转目标和条件末尾 false-path 会被 emitter 收敛到显式兜底标签。
- 输入：`icg_probe jimple-invalid-target` 直接构造 `if x > 0 goto 99`。
- 预期输出：包含 `label_invalid:`、`label_exit:` 和默认 `return 0;`。
- 测试步骤：
  1. 运行 `./icg_probe jimple-invalid-target`。
  2. 检查条件语句后的显式 `goto label_exit;` 与两个兜底标签。
  3. 与 `expected/26_jimple_invalid_target_fallback.txt` 做精确比对。
