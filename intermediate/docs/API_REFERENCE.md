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
- `dumpIntermediateCode`
