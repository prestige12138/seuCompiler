# seuIntermediate API Reference

## ASTBuilder

- `makeIdentifier`
- `makeConstant`
- `makeVarDecl`
- `makeAssignment`
- `makeBinary`
- `makeCall`
- `makeFunction`
- `makeIf`
- `makeWhile`
- `makeReturn`
- `makeProgram`
- `destroyTree`

## Parse Root

- `setParseRoot`
- `getParseRoot`
- `releaseParseRoot`

## SymbolTable

- `reset`
- `enterScope`
- `exitScope`
- `declareVariable`
- `declareFunction`
- `declareParameter`
- `lookup`

## TriAddrGenerator

- `generate(ASTNode* root)`

## Formatting

- `formatTriAddrStmt`
- `formatIntermediateCode`
- `splitBasicBlocks`
- `formatBasicBlocks`
- `dumpIntermediateCode`
- `dumpBasicBlocks`

## TargetIrEmitter

- `TargetIrOptions`
  - `moduleName`：LLVM 模块名
  - `className`：Jimple 类名
  - `entryFunction`：无显式函数时的合成入口名
  - `emitComments`：是否输出模块 / 类头
- `formatLlvmIr(const ASTNode* root, const IntermediateCode& code, const TargetIrOptions& options = {})`
  - 输入：AST 根和三地址码
  - 返回：稳定 LLVM IR 文本
- `formatJimple(const ASTNode* root, const IntermediateCode& code, const TargetIrOptions& options = {})`
  - 输入：AST 根和三地址码
  - 返回：稳定 Jimple 文本
- `dumpLlvmIr`
  - 将 LLVM IR 直接写入输出流
- `dumpJimple`
  - 将 Jimple 直接写入输出流

补充约定：

- `splitBasicBlocks` 返回的块保留原始 `stmtNo`
- 分块结果中的 `stmtCount` 是该块内最大 `stmtNo`，不是块内语句条数
- 若需要块内语句数量，应使用 `stmts.size()`
- `formatBasicBlocks` 对非法跳转目标输出 `invalid(<stmtNo>)`
- `formatLlvmIr` / `formatJimple` 在 AST 含多个 `NODE_FUNC_DEF` 时，会按 AST 推断的函数语句段长度切分传入的 `IntermediateCode`
- `formatJimple` 当前输出为课程验收友好的稳定文本，不直接生成 `.class`
