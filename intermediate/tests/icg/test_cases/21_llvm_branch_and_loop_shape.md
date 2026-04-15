# 21_llvm_branch_and_loop_shape

- 名称：LLVM 分支与循环形状
- 目的：验证条件跳转、退出块和回边的 block 结构。
- 输入：`icg_probe llvm-branch-loop` 直接构造带 `if/while` 形状的三地址码。
- 预期输出：包含多个 `bb_*` 标签、`icmp slt`、`br i1` 和回跳。
- 测试步骤：
  1. 运行 `./icg_probe llvm-branch-loop`。
  2. 检查控制流标签和跳转指令。
  3. 与 `expected/21_llvm_branch_and_loop_shape.txt` 做精确比对。
