# 22_jimple_call_and_assign

- 名称：Jimple 调用与赋值
- 目的：验证 `call -> temp -> assign -> return` 的 Jimple 输出。
- 输入：`icg_probe jimple-call-assign` 直接构造 `t1 = call foo(a, 1); x = t1; return x;`。
- 预期输出：包含 `.class`、`.method`、`staticinvoke`、局部变量声明和 `return x;`。
- 测试步骤：
  1. 运行 `./icg_probe jimple-call-assign`。
  2. 检查方法头、局部变量和调用语句。
  3. 与 `expected/22_jimple_call_and_assign.txt` 做精确比对。
