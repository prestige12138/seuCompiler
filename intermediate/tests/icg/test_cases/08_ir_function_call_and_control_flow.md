# 08_ir_function_call_and_control_flow

- 名称：函数调用与控制流 IR
- 目的：验证 `if/else`、`while`、函数调用和回填跳转目标的组合行为。
- 输入：`icg_probe ir-control-flow` 构造带 `foo(x, 1)` 调用的条件与循环程序。
- 预期输出：`if` 条件跳转、`goto`、函数调用临时变量、循环回跳的语句编号全部稳定。
- 测试步骤：
  1. 运行 `./icg_probe ir-control-flow`。
  2. 检查跳转目标和 `call` 语句。
  3. 与 `expected/08_ir_function_call_and_control_flow.txt` 做精确比对。
