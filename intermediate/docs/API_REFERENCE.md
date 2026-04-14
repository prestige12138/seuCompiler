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

补充约定：

- `splitBasicBlocks` 返回的块保留原始 `stmtNo`
- 分块结果中的 `stmtCount` 是该块内最大 `stmtNo`，不是块内语句条数
- 若需要块内语句数量，应使用 `stmts.size()`
- `formatBasicBlocks` 对非法跳转目标输出 `invalid(<stmtNo>)`
