# 20_llvm_linear_arith_and_return

- 名称：LLVM 线性算术与返回
- 目的：验证 `assign/add/return` 到 LLVM IR 的最小映射。
- 输入：`icg_probe llvm-linear` 直接构造 `a = 1; b = 2; t1 = a + b; return t1;`。
- 预期输出：包含 `define i32 @main()`、`alloca i32`、`add nsw i32` 和 `ret i32`。
- 测试步骤：
  1. 运行 `./icg_probe llvm-linear`。
  2. 检查生成的 LLVM IR 结构。
  3. 与 `expected/20_llvm_linear_arith_and_return.txt` 做精确比对。
