# SEU Compiler 2026

A course project repository for compiler construction at Southeast University, currently centered on a modular `seuLex` implementation in C++17.

## Status

Current repository state:

- `seuLex`: implemented and modularized
- project-level technical docs: added
- `Yacc` / parser generator integration: not implemented in this repo state
- AST / semantic analysis / IR generation: not implemented in this repo state

## Highlights

- C++17 implementation
- modular `seuLex` project under [`seuLex/`](/Users/llawliet/代码/seuCompiler/seuLex)
- strict preservation of report-defined names such as `node`, `nfa`, `dfa`, `idreTable`, `nfaTable`
- full lexer-generation pipeline currently present in code:
  - Lex file parsing
  - extended regular expression expansion
  - infix-to-postfix conversion
  - Thompson NFA construction
  - NFA merge
  - subset-construction DFA
  - DFA minimization
  - standalone lexer code generation
  - dot visualization
  - built-in self-test

## Repository Layout

```text
.
├── README.md
├── AGENTS.md
├── resources/
│   ├── c99.l
│   ├── c99.y
│   ├── minic.l
│   ├── 编译原理中期报告.docx
│   └── 编译原理课程实践 2026.pptx
└── seuLex/
    ├── include/
    ├── src/
    ├── tests/
    ├── docs/
    ├── CMakeLists.txt
    └── README-LEX.md
```

## Quick Start

Build `seuLex`:

```bash
cd seuLex
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Run the built-in self-test:

```bash
cd seuLex
./build/seuLex --self-test
```

Generate a lexer from a Lex source:

```bash
cd seuLex
./build/seuLex ../resources/minic.l generated_minic.cpp dot
```

## seuLex

`seuLex` is the active implemented module in this repository.

Main entry points:

- CLI: [`seuLex/src/main.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/main.cpp)
- driver facade: [`seuLex/include/code_generator.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/code_generator.h)
- parser: [`seuLex/include/lex_parser.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/lex_parser.h)
- regex/NFA/DFA construction: [`seuLex/include/nfa_constructor.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/nfa_constructor.h)
- DFA minimization: [`seuLex/include/dfa_minimizer.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/dfa_minimizer.h)

Detailed module docs:

- overview: [`seuLex/docs/README.md`](/Users/llawliet/代码/seuCompiler/seuLex/docs/README.md)
- algorithms: [`seuLex/docs/ALGORITHMS.md`](/Users/llawliet/代码/seuCompiler/seuLex/docs/ALGORITHMS.md)
- API reference: [`seuLex/docs/API_REFERENCE.md`](/Users/llawliet/代码/seuCompiler/seuLex/docs/API_REFERENCE.md)
- data structures: [`seuLex/docs/DATA_STRUCTURES.md`](/Users/llawliet/代码/seuCompiler/seuLex/docs/DATA_STRUCTURES.md)
- dependency graphs: [`seuLex/docs/DEPENDENCY_GRAPH.md`](/Users/llawliet/代码/seuCompiler/seuLex/docs/DEPENDENCY_GRAPH.md)
- development notes: [`seuLex/docs/DEVELOPMENT_PROCESS.md`](/Users/llawliet/代码/seuCompiler/seuLex/docs/DEVELOPMENT_PROCESS.md)

## Current Constraints

- platform: UNIX/POSIX
- language: C++17
- current code generation path is lexer-only
- `.l` actions and user code are treated as trusted input and embedded verbatim into generated C++
- `resources/minic.l` and `resources/c99.l` are used as reference inputs

## Recommended Reading Order

1. [`README-LEX.md`](/Users/llawliet/代码/seuCompiler/seuLex/README-LEX.md)
2. [`seuLex/docs/README.md`](/Users/llawliet/代码/seuCompiler/seuLex/docs/README.md)
3. [`seuLex/docs/ALGORITHMS.md`](/Users/llawliet/代码/seuCompiler/seuLex/docs/ALGORITHMS.md)
4. [`seuLex/docs/API_REFERENCE.md`](/Users/llawliet/代码/seuCompiler/seuLex/docs/API_REFERENCE.md)

## Roadmap

Reasonable next steps from the current repository state:

- stabilize `seuLex` runtime semantics for empty-string rules
- add compileable harnesses for generated `minic` / `c99` lexers
- implement the `Yacc` side as a separate modular subtree
- continue toward AST, semantic analysis, and intermediate representation stages

## Notes

- Generated artifacts such as `generated_lexer.cpp` and `dot/*.dot` are outputs, not source-of-truth files.
- `resources/` should be treated as reference material and sample inputs.
