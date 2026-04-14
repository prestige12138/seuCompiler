# seuIntermediate 算法说明

## AST 构建

通过 `ASTBuilder` 在语义动作里按产生式归约组树。

## 符号表

- 进入函数或局部块时 `enterScope()`
- 离开时 `exitScope()`
- 查询从内向外逐层回退

## 三地址码生成

- 表达式节点递归生成
- 算术运算生成临时变量
- `if/while` 通过 `OP_IF_GOTO` 和 `OP_GOTO` 回填目标语句号
- `return` 直接输出 `OP_RETURN`

## 基本块划分

- leader 规则：
  - 第一条语句
  - `OP_GOTO` / `OP_IF_GOTO` 的目标语句
  - `OP_GOTO` / `OP_IF_GOTO` / `OP_RETURN` 的后继语句
- `splitBasicBlocks(...)` 按 leader 把现有 `IntermediateCode` 切成块视图
- `formatBasicBlocks(...)` 进一步给出稳定文本形式和块后继关系
- 非法跳转目标不会生成新 leader，格式化时固定显示为 `invalid(<stmtNo>)`
