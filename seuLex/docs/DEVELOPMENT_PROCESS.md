# Development Process

## 1. Current State Of The Project

The current `seuLex` subtree is a modular refactoring of an earlier monolithic implementation. The code now separates:

- parsing
- regex normalization and NFA/DFA construction
- DFA minimization
- code generation and visualization
- CLI entry

The refactoring preserved the original functionality and the report-defined data structure names, while moving implementation details into separate headers and sources.

## 2. High-Level Implementation Phases Reflected In The Code

### Phase A. Modularization

The single-file implementation was decomposed into:

- [`include/node.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/node.h)
- [`include/nfa.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/nfa.h)
- [`include/dfa.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/dfa.h)
- [`include/lex_parser.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/lex_parser.h)
- [`include/nfa_constructor.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/nfa_constructor.h)
- [`include/dfa_minimizer.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/dfa_minimizer.h)
- [`include/code_generator.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/code_generator.h)
- [`src/lex_parser.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/lex_parser.cpp)
- [`src/nfa_constructor.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/nfa_constructor.cpp)
- [`src/dfa_minimizer.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/dfa_minimizer.cpp)
- [`src/code_generator.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/code_generator.cpp)
- [`src/main.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/main.cpp)

### Phase B. Build System Consolidation

[`CMakeLists.txt`](/Users/llawliet/代码/seuCompiler/seuLex/CMakeLists.txt) formalizes:

- C++17 compilation
- UNIX/POSIX-only constraint
- warning flags for Clang/GNU
- built-in CTest entry
- self-test compiler injection via `SEU_LEX_TEST_CXX`

### Phase C. Parser Hardening

The parser now explicitly handles several cases that previously broke valid Lex files:

- standalone `%%` detection rather than naive substring search
- `%%` inside `%{...%}` blocks
- `%%` inside rule actions
- braces inside block comments and line comments within multi-line actions

### Phase D. Regex/Runtime Safety Fixes

The current code also added guardrails without changing the intended feature set:

- repetition-bound integer overflow detection
- repetition-bound implementation limit
- self-test process execution via `fork/execvp` instead of shelling through `std::system()`

## 3. Key Code Evolution Themes

### Preserve Report Names, Improve Boundaries

The refactoring deliberately did not rename the report-defined entities. Instead, it contained them inside `namespace seu_lex` and reduced global namespace pollution.

### Separate Public Interface And Implementation

Headers now expose only the external contracts, while helper functions and parser internals remain in anonymous namespaces or internal classes.

### Prefer POSIX Runtime Utilities Over `std::filesystem`

To avoid platform/toolchain issues seen during compilation and editor analysis, the current implementation uses POSIX functions such as:

- `mkdir`
- `mkdtemp`
- `stat`
- `getcwd`
- `fork`
- `execvp`
- `waitpid`

## 4. Verification Process Reflected In The Code

The project currently verifies itself through:

### Build Verification

```bash
cmake -S seuLex -B seuLex/build
cmake --build seuLex/build
```

### Automated Self-Test

```bash
ctest --test-dir seuLex/build --output-on-failure
```

### Manual Self-Test Entry

```bash
cd seuLex
./build/seuLex --self-test
```

### What `runSelfTests()` Covers

- generation and compilation of a sample lexer
- runtime tokenization smoke test
- direct regex-to-DFA checks
- invalid regex failure checks
- parser regression tests for delimiter/comment corner cases
- generation of `minic` and `c99` lexer outputs

## 5. Known Limits In The Current Implementation

These are properties of the current code, not future design goals.

- `.l` action blocks and user code are treated as trusted input and embedded verbatim in generated C++.
- The effective runtime alphabet is ASCII-based.
- Empty-string token rules remain a constrained runtime case.
- `resources/minic.l` and `resources/c99.l` are generated in self-test but not compiled there because their surrounding runtime dependencies are outside the local `seuLex` module.

## 6. AI Usage Record

This documentation is based on the current checked-in `seuLex` implementation and the recent AI-assisted refactor history visible from the code structure and commit trail.

### Main Agent Responsibilities

- scanned all current `include/` and `src/` files
- mapped public interfaces to implementations
- reconstructed the active generation pipeline from code
- wrote the documentation set under `seuLex/docs/`

### Supporting Agent Roles Used In The Recent Refactor/Documentation Workflow

- `explorer`
  - scanned source/module boundaries
  - summarized major algorithms and caveats
- `architect`
  - proposed the documentation outline and section structure
- `reviewer` / `code-reviewer`
  - identified parser corner-case regressions and verification gaps during the refactor cycle
- `security-reviewer`
  - highlighted trusted-input and process-execution risks

### AI-Driven Issues That Were Addressed In The Current Code

- removal of `std::filesystem` usage in favor of POSIX APIs
- namespace and include cleanup during modular refactor
- parser fixes for `%%` handling and comment-aware brace balancing
- overflow/bound checks for repetition parsing
- self-test subprocess handling without shell invocation

## 7. Documentation Generation Workflow

For this documentation pass, the process was:

1. Scan all headers in `include/` to identify official interfaces and report-defined names.
2. Scan all sources in `src/` to reconstruct hidden helpers and actual control flow.
3. Cross-check build/test behavior against `CMakeLists.txt` and `README-LEX.md`.
4. Write:
   - overview
   - algorithms
   - API reference
   - data structures
   - development process
   - dependency graph
5. Review documentation for consistency with current code only.

## 8. Practical Maintenance Guidance

When updating the code later, the documentation should be revised if any of these change:

- report-defined data structure names
- generation pipeline ordering
- regex syntax supported by `ExtendedRegexParser`
- self-test coverage
- generated lexer runtime API
- module boundaries under `include/` and `src/`
