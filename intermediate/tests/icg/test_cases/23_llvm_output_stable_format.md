# 23_llvm_output_stable_format

- 名称：LLVM 输出格式稳定性
- 目的：锁定参数签名、槽位顺序和寄存器编号格式。
- 输入：`icg_probe llvm-stable` 直接构造 `add(lhs, rhs)` 的函数级三地址码。
- 预期输出：`lhs/rhs/sum/t1` 的顺序、`entry` 块和寄存器编号稳定。
- 测试步骤：
  1. 运行 `./icg_probe llvm-stable`。
  2. 检查函数头、alloca 顺序和返回序列。
  3. 与 `expected/23_llvm_output_stable_format.txt` 做精确比对。
