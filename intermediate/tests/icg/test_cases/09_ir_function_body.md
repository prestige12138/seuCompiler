# 09_ir_function_body

- 名称：函数体 IR 与作用域回收
- 目的：验证函数参数登记、函数体代码生成以及退出函数后局部/参数不再可见。
- 输入：`icg_probe ir-function` 构造 `int add(int a, int b) { int c; c = a + b; return c; }`。
- 预期输出：函数体 IR 为三条语句，且函数符号保留、局部 `c` 与参数 `a` 已不可见。
- 测试步骤：
  1. 运行 `./icg_probe ir-function`。
  2. 检查 IR 及三个可见性字段。
  3. 与 `expected/09_ir_function_body.txt` 做精确比对。
