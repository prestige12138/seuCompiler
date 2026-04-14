# 18_basic_block_sparse_stmt_numbers

- 目的：验证非连续语句号下的基本块显示不会误写成连续区间。
- 输入：`icg_probe basic-block-sparse`，内部构造 `10, 20, 30` 三条语句。
- 预期输出：单块成员显示为 `stmts=[10,20,30]`。
- 测试步骤：
  1. 运行 `./icg_probe basic-block-sparse`。
  2. 检查 leader 为 `10`，块成员列表为精确语句号集合。
  3. 与 `expected/18_basic_block_sparse_stmt_numbers.txt` 做精确比对。
