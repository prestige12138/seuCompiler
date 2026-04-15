# 25_llvm_invalid_target_fallback

- 名称：LLVM 非法目标兜底块
- 目的：验证非法跳转目标和条件末尾 false-path 会被 emitter 收敛到已定义的兜底 block。
- 输入：`icg_probe llvm-invalid-target` 直接构造 `if x > 0 goto 99`。
- 预期输出：包含 `bb_invalid:`、`bb_exit:` 和默认 `ret i32 0`。
- 测试步骤：
  1. 运行 `./icg_probe llvm-invalid-target`。
  2. 检查条件跳转、无效目标块和退出块。
  3. 与 `expected/25_llvm_invalid_target_fallback.txt` 做精确比对。
