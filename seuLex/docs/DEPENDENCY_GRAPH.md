# Dependency Graph

## 1. Module-Level Dependency Graph

```mermaid
graph TD
  main_cpp[main.cpp] --> code_generator_h[code_generator.h]
  code_generator_cpp[code_generator.cpp] --> code_generator_h
  code_generator_cpp --> dfa_minimizer_h[dfa_minimizer.h]
  code_generator_cpp --> nfa_constructor_h[nfa_constructor.h]
  dfa_minimizer_cpp[dfa_minimizer.cpp] --> dfa_minimizer_h
  dfa_minimizer_cpp --> nfa_h[nfa.h]
  lex_parser_cpp[lex_parser.cpp] --> lex_parser_h[lex_parser.h]
  lex_parser_cpp --> nfa_h
  nfa_constructor_cpp[nfa_constructor.cpp] --> nfa_constructor_h
  nfa_constructor_h --> dfa_h[dfa.h]
  nfa_constructor_h --> lex_parser_h
  nfa_constructor_h --> nfa_h
  code_generator_h --> dfa_h
  code_generator_h --> lex_parser_h
  code_generator_h --> nfa_h
  dfa_h --> node_h[node.h]
  nfa_h --> node_h
```

## 2. End-To-End Generation Pipeline

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

## 3. Runtime And Self-Test Flow

```mermaid
graph LR
  M[main] --> N{argv}
  N -->|--self-test| O[SeuLexDriver::runSelfTests]
  N -->|generate| P[SeuLexDriver::generate]
  O --> P
  O --> Q[evaluateDFA]
  O --> R[runProcess]
```

## 4. Lex Parser Internal Dependencies

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

## 5. Regex And Automaton Construction Dependencies

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

  buildNFA --> createState
  buildNFA --> char_set
  buildNFA --> nfaterstatetoaction[(nfaterstatetoaction)]
```

## 6. Minimization Dependencies

```mermaid
graph TD
  minimizeDFA --> actionForState
  minimizeDFA --> char_set[(char_set)]
  minimizeDFA --> TerStateActionTable[(TerStateActionTable)]
  minimizeDFA --> mindfareturn[(mindfareturn)]
  minimizeDFA --> dfaterminals[(dfaterminals)]
```

## 7. Code Generation Dependencies

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

## 8. Global-State Interaction Summary

| Component | Reads | Writes |
|---|---|---|
| `LexParser::parseLexFile` | `idreTable` | `idreTable` |
| `REExpander::expandRE` | `idreTable` | none |
| `NFABuilder::buildNFA` | none | `char_set`, `nfaterstatetoaction` |
| `NFABuilder::mergeNFA` | none | none |
| `DFABuilder::subsetConstruct` | `char_set`, `nfaterstatetoaction` | `TerStateActionTable`, `dfaterminals` |
| `DFAMinimizer::minimizeDFA` | `char_set`, `TerStateActionTable`, `mindfareturn` | `TerStateActionTable`, `mindfareturn`, `dfaterminals` |
| `CodeGenerator::emitLexer` | `TerStateActionTable`, `mindfareturn` | emitted file |
| `SeuLexDriver::generate` | all stage outputs | `nfaTable`, dot files, generated lexer |
| `resetGlobalTables` | all global tables | clears all global tables |
