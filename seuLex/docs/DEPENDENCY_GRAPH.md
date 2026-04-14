# seuLex 依赖图

## 1. 模块级依赖图

```mermaid
graph TD
  main_cpp[main.cpp] --> code_generator_h[code_generator.h]
  code_generator_cpp[code_generator.cpp] --> code_generator_h
  code_generator_cpp --> regex_expander_h[regex_expander.h]
  code_generator_cpp --> dfa_builder_h[dfa_builder.h]
  code_generator_cpp --> dfa_minimizer_h[dfa_minimizer.h]
  code_generator_cpp --> nfa_constructor_h[nfa_constructor.h]
  lex_state_cpp[lex_state.cpp] --> nfa_constructor_h
  lex_state_cpp --> lex_state_h[internal/lex_state.h]
  node_cpp[node.cpp] --> node_h[node.h]
  dfa_minimizer_cpp[dfa_minimizer.cpp] --> dfa_minimizer_h
  dfa_minimizer_cpp --> nfa_h[nfa.h]
  dfa_builder_cpp[dfa_builder.cpp] --> dfa_builder_h
  dfa_builder_cpp --> dfa_h[dfa.h]
  dfa_builder_cpp --> lex_state_h
  lex_parser_cpp[lex_parser.cpp] --> lex_parser_h[lex_parser.h]
  lex_parser_cpp --> nfa_h
  regex_expander_cpp[regex_expander.cpp] --> regex_expander_h
  regex_expander_cpp --> nfa_h
  nfa_constructor_cpp[nfa_constructor.cpp] --> nfa_constructor_h
  nfa_constructor_cpp --> lex_state_h
  nfa_constructor_h --> nfa_h
  nfa_constructor_h --> regex_expander_h
  nfa_constructor_h --> dfa_builder_h
  code_generator_h --> dfa_h
  code_generator_h --> lex_parser_h
  code_generator_h --> nfa_h
  dfa_h --> node_h[node.h]
  nfa_h --> node_h
```

## 2. 端到端生成流程图

```mermaid
graph LR
  A[LexParser::parseLexFile] --> B[REExpander::expandRE]
  B --> C[NFABuilder::toPostfix]
  C --> D[NFABuilder::buildNFA]
  D --> E[NFABuilder::mergeNFA]
  E --> F[DFABuilder::subsetConstruct]
  F --> G[DFAMinimizer::minimizeDFA]
  G --> H[Visualizer::dumpNFA]
  G --> I[Visualizer::dumpDFA]
  G --> J[CodeGenerator::emitLexer]
```

## 3. 运行与自测流程图

```mermaid
graph LR
  M[main] --> N{argv}
  N -->|--self-test| O[SeuLexDriver::runSelfTests]
  N -->|generate| P[SeuLexDriver::generate]
  O --> P
  O --> Q[evaluateDFA]
  O --> R[runProcess]
```

## 4. Lex 解析器内部依赖

```mermaid
graph TD
  parseLexFile --> readWholeFile
  parseLexFile --> findSectionDelimiter
  parseLexFile --> takeBetweenMarkers
  parseLexFile --> removeRanges
  parseLexFile --> splitByLines
  parseLexFile --> stripInlineComment
  parseLexFile --> splitRegexAndAction
  parseLexFile --> isActionBalanced
  parseLexFile --> trim
  parseLexFile --> idreTable[(idreTable)]
```

## 5. 正则与自动机构造依赖

```mermaid
graph TD
  expandRE --> expandNamedDefinitions
  expandRE --> ExtendedRegexParser
  expandRE --> serializeAst

  subsetConstruct --> Eclosure
  subsetConstruct --> pickActionFromSet
  subsetConstruct --> char_set[(char_set)]
  subsetConstruct --> TerStateActionTable[(TerStateActionTable)]
  subsetConstruct --> dfaterminals[(dfaterminals)]
  subsetConstruct --> nfaPriorityTableInternal[(nfaPriorityTableInternal)]

  buildNFA --> createState
  buildNFA --> char_set
  buildNFA --> nfaterstatetoaction[(nfaterstatetoaction)]
  buildNFA --> nfaPriorityTableInternal
```

## 6. 最小化依赖

```mermaid
graph TD
  minimizeDFA --> actionForState
  minimizeDFA --> char_set[(char_set)]
  minimizeDFA --> TerStateActionTable[(TerStateActionTable)]
  minimizeDFA --> mindfareturn[(mindfareturn)]
  minimizeDFA --> dfaterminals[(dfaterminals)]
```

## 7. 代码生成依赖

```mermaid
graph TD
  emitLexer --> actionForState
  emitLexer --> ensureDirectory
  emitLexer --> dirnameOf
  emitLexer --> mindfareturn[(mindfareturn)]
  emitLexer --> TerStateActionTable[(TerStateActionTable)]
  emitLexer --> LexSpecification
  emitLexer --> dfa

  dumpNFA --> ensureDirectory
  dumpNFA --> escapeDotLabel
  dumpDFA --> ensureDirectory
  dumpDFA --> escapeDotLabel
```

## 8. 全局状态交互总结

| 组件 | 读取 | 写入 |
|---|---|---|
| `LexParser::parseLexFile` | `idreTable` | `idreTable` |
| `REExpander::expandRE` | `idreTable` | 无 |
| `NFABuilder::buildNFA` | 无 | `char_set`、`nfaterstatetoaction`、`nfaPriorityTableInternal` |
| `NFABuilder::mergeNFA` | 无 | 无 |
| `DFABuilder::subsetConstruct` | `char_set`、`nfaterstatetoaction`、`nfaPriorityTableInternal` | `TerStateActionTable`、`dfaterminals` |
| `DFAMinimizer::minimizeDFA` | `char_set`、`TerStateActionTable`、`mindfareturn` | `TerStateActionTable`、`mindfareturn`、`dfaterminals` |
| `CodeGenerator::emitLexer` | `TerStateActionTable`、`mindfareturn` | 输出文件 |
| `SeuLexDriver::generate` | 各阶段产物 | `nfaTable`、dot 文件、生成的 lexer |
| `resetGlobalTables` | 所有全局表 | 清空所有全局表 |

## 9. 依赖关系评价

`seuLex` 当前已经有明显模块边界，但依赖图里有一个很直观的现象：

- `code_generator.*` 是总控枢纽
- 全局表仍然是阶段耦合中心

这说明当前 Lex 端的单文件耦合已经下降，但状态共享仍然比 `seuYacc` 更重。
