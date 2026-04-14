# 04_symbol_offsets_global_local_param

- 名称：全局/局部/参数偏移分配
- 目的：验证 `SymbolTable` 对不同存储类别使用独立偏移计数器，并按类型宽度累计。
- 输入：`icg_probe symbol-offsets` 构造全局变量、函数参数和局部变量。
- 预期输出：全局、参数、局部偏移分别为 `0/4`、`0/8`、`0/2`。
- 测试步骤：
  1. 运行 `./icg_probe symbol-offsets`。
  2. 检查偏移值和当前作用域深度。
  3. 与 `expected/04_symbol_offsets_global_local_param.txt` 做精确比对。
