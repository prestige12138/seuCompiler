# seuLex Documentation

## Overview

`seuLex` is a modular lexical-analyzer generator implemented in C++17 for UNIX/POSIX environments. The current codebase keeps the report-defined names unchanged while moving them into `namespace seu_lex`.

The implementation covers the full generation chain present in the code:

1. Parse a Lex source file into Definitions / Rules / User subroutines.
2. Expand extended regular expressions into a normalized ordinary form.
3. Convert normalized infix RE to postfix form.
4. Build per-rule Thompson NFAs.
5. Merge multiple NFAs under a new epsilon start node.
6. Determinize the merged NFA into a DFA.
7. Minimize the DFA by partition refinement.
8. Generate standalone C++ lexer code and dot visualizations.
9. Run built-in smoke, regex, and parser regression tests.

## Directory Layout

```text
seuLex/
├── include/
│   ├── node.h
│   ├── nfa.h
│   ├── dfa.h
│   ├── lex_parser.h
│   ├── nfa_constructor.h
│   ├── dfa_minimizer.h
│   └── code_generator.h
├── src/
│   ├── lex_parser.cpp
│   ├── nfa_constructor.cpp
│   ├── dfa_minimizer.cpp
│   ├── code_generator.cpp
│   └── main.cpp
├── tests/
│   └── sample_smoke.l
├── docs/
│   ├── README.md
│   ├── ALGORITHMS.md
│   ├── API_REFERENCE.md
│   ├── DATA_STRUCTURES.md
│   ├── DEVELOPMENT_PROCESS.md
│   └── DEPENDENCY_GRAPH.md
├── CMakeLists.txt
└── README-LEX.md
```

## Build And Run

```bash
cd seuLex
cmake -S . -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

Generate a lexer:

```bash
./build/seuLex ../resources/minic.l generated_minic.cpp dot
```

Run the built-in self-test:

```bash
./build/seuLex --self-test
```

## Entry Path

The executable entry is [`main.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/main.cpp). The production generation path is:

`main()` -> `seu_lex::SeuLexDriver::generate()` -> parser -> RE expansion -> postfix conversion -> NFA -> DFA -> minimization -> dot/code emission.

## Core Modules

- [`lex_parser.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/lex_parser.cpp): splits Lex files and builds `LexSpecification`.
- [`nfa_constructor.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/nfa_constructor.cpp): owns the report-defined globals, regex normalization, Thompson NFA construction, and subset construction.
- [`dfa_minimizer.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/dfa_minimizer.cpp): minimizes DFA states while preserving action semantics.
- [`code_generator.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/code_generator.cpp): emits standalone C++ lexer code, dot graphs, and self-tests.
- [`main.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/main.cpp): CLI wrapper.

## Constraints And Boundaries

- Language standard: C++17.
- Platform: UNIX/POSIX only.
- Character domain used by generated transition tables: ASCII 1..127, with `'\0'` reserved as epsilon.
- The generated lexer embeds `%{...%}`, rule actions, and user subroutines verbatim. The `.l` file is therefore treated as trusted input by design.
- `{m,n}` repetition bounds are guarded against integer overflow and are capped by the implementation limit in `nfa_constructor.cpp`.
- The self-test generates `minic` and `c99` lexer sources, but does not compile them because their runtime dependencies are outside the current `seuLex` subtree.

## Reading Guide

- Read [`ALGORITHMS.md`](/Users/llawliet/代码/seuCompiler/seuLex/docs/ALGORITHMS.md) for the end-to-end compiler-theory pipeline.
- Read [`API_REFERENCE.md`](/Users/llawliet/代码/seuCompiler/seuLex/docs/API_REFERENCE.md) for public and internal function contracts.
- Read [`DATA_STRUCTURES.md`](/Users/llawliet/代码/seuCompiler/seuLex/docs/DATA_STRUCTURES.md) for report-defined names and ownership notes.
- Read [`DEPENDENCY_GRAPH.md`](/Users/llawliet/代码/seuCompiler/seuLex/docs/DEPENDENCY_GRAPH.md) for Mermaid dependency views.
- Read [`DEVELOPMENT_PROCESS.md`](/Users/llawliet/代码/seuCompiler/seuLex/docs/DEVELOPMENT_PROCESS.md) for implementation history and AI usage notes.
