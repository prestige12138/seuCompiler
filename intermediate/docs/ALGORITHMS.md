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

## 目标 IR Lowering

### 函数单元恢复

- 当前 `IntermediateCode` 没有 `FUNC_BEGIN/FUNC_END`
- 因此 `target_ir_emitter` 先读 AST：
  - 若根节点包含多个 `NODE_FUNC_DEF`，先用 AST 推断每个函数对应的语句段长度，再切分传入的 `IntermediateCode`
  - 若没有显式函数定义，则把输入视为一个合成入口函数
- 每个函数单元同时收集：
  - 返回类型
  - 参数列表
  - 局部声明
  - 三地址码中出现的临时变量

复杂度：`O(F + N)`，其中 `F` 是函数数，`N` 是语句数。

### LLVM IR 输出

- 目标是稳定、可读的非 SSA 文本，而不是优化后的真实后端 IR
- 关键规则：
  - 局部变量、参数、临时变量统一映射到 `alloca i32`
  - 常量直接以内联立即数输出
  - 变量读取使用 `load`
  - 变量写回使用 `store`
  - `OP_IF_GOTO` 解析为 `icmp` + `br i1`
  - `OP_GOTO` 解析为 `br label`
  - `OP_RETURN` 解析为 `ret`

复杂度：`O(N + T)`，其中 `T` 是输出文本长度。

### Jimple 输出

- 目标是方法级、标签化、局部变量显式声明的稳定文本
- 关键规则：
  - 每个函数输出 `.method public static ...`
  - 参数保留在方法签名中
  - 局部变量与临时变量统一声明为 `int`
  - 基本块 leader 映射为 `label_<stmtNo>`
  - `OP_FUNC_CALL` 映射为 `staticinvoke`
  - 分支与循环继续沿用 `if ... goto` / `goto ...`

复杂度：`O(N + T)`。
