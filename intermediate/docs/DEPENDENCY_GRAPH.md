# seuIntermediate 依赖图

```mermaid
graph TD
  A[ast_builder] --> B[intermediate_code]
  C[symbol_table] --> B
  D[tri_addr_generator] --> A
  D --> B
  D --> C
  E[target_ir_emitter] --> A
  E --> B
  E --> D
  F[main] --> A
  F --> B
  F --> C
  F --> D
  F --> E
```
