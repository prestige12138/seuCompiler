# Data Structures

## Naming Policy

The current implementation intentionally preserves the report-defined names:

- `node`
- `nfa`
- `dfa`
- `idreTable`
- `nfaTable`
- `dfaterminals`
- `TerStateActionTable`
- `mindfareturn`
- `char_set`
- `mulit`
- `myMul`

All of them live inside `namespace seu_lex`.

## 1. `node`

Defined in [`node.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/node.h), implemented in [`nfa_constructor.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/nfa_constructor.cpp).

### Fields

| Field | Type | Meaning |
|---|---|---|
| `label` | `int` | numeric state ID |
| `accepted` | `bool` | whether the state is accepting |
| `outstate` | `std::multimap<char, node*>` | outgoing transitions |

### Semantic Notes

- `'\0'` is reserved as epsilon.
- Multiple outgoing edges with the same label are allowed because the representation is a `multimap`.
- `getMultimap()` returns a copy, not a reference.

### Ownership

For NFA construction, `node` instances are allocated in the internal arena `g_nodeArena`. The lifetime is controlled by `resetGlobalTables()`.

For DFA/minimized DFA, `node` instances are stored by value inside `dfa::nodeVec`.

## 2. `mulit`

Defined in [`node.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/node.h).

```cpp
typedef std::multimap<char, node*>::iterator mulit;
```

Purpose:
- historical/report-defined iterator alias

## 3. `myMul`

Defined in [`node.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/node.h).

```cpp
typedef std::multimap<char, node*> myMul;
```

Purpose:
- historical/report-defined transition-table alias

## 4. `nfa`

Defined in [`nfa.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/nfa.h).

### Fields

| Field | Type | Meaning |
|---|---|---|
| `start` | `node*` | entry state |
| `terminal` | `std::vector<node*>` | accepting state set |

### Usage

- represents one rule NFA during Thompson construction
- also represents the merged super-NFA before determinization

## 5. `dfa`

Defined in [`dfa.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/dfa.h).

### Fields

| Field | Type | Meaning |
|---|---|---|
| `start` | `node*` | start state pointer |
| `nodeVec` | `std::vector<node>` | all DFA states |
| `endNode` | `std::vector<node>` | accepting states copied by value |

### Usage

- stores either the raw DFA from subset construction or the minimized DFA
- `start` points into `nodeVec`
- transition targets also point into `nodeVec`

### Ownership Note

Because states are stored by value in `nodeVec`, any code that builds transitions must finish resizing first and only then store `node*` links into the vector.

## 6. `LexRule`

Defined in [`lex_parser.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/lex_parser.h).

### Fields

| Field | Type | Meaning |
|---|---|---|
| `regex` | `std::string` | original regex in Rules section |
| `action` | `std::string` | associated action code |
| `priority` | `std::size_t` | rule order, lower means earlier |
| `expandedRegex` | `std::string` | normalized ordinary RE |
| `postfixRegex` | `std::string` | postfix form used for Thompson construction |

### Role

Acts as the per-rule carrier across parsing, normalization, and automaton construction.

## 7. `LexSpecification`

Defined in [`lex_parser.h`](/Users/llawliet/代码/seuCompiler/seuLex/include/lex_parser.h).

### Fields

| Field | Type | Meaning |
|---|---|---|
| `definitionsSection` | `std::string` | raw Definitions section |
| `verbatimDefinitions` | `std::string` | concatenated `%{...%}` blocks |
| `rulesSection` | `std::string` | raw Rules section |
| `userSubroutines` | `std::string` | raw trailing user code |
| `rules` | `std::vector<LexRule>` | parsed rules |

### Role

This is the main parser output consumed by `SeuLexDriver::generate()`.

## 8. Global Tables

### `char_set`

Type:

```cpp
std::set<char>
```

Role:
- alphabet of all non-epsilon symbols seen while building NFAs

Used by:
- subset construction
- minimization
- code generation table size decisions

### `idreTable`

Type:

```cpp
std::map<std::string, std::string>
```

Role:
- named regex definitions from the Definitions section

### `nfaTable`

Type:

```cpp
std::vector<nfa>
```

Role:
- stores all per-rule NFAs before merge

### `nfaterstatetoaction`

Type:

```cpp
std::map<int, std::string>
```

Role:
- accepting NFA state ID -> action code

### `dfaterminals`

Type:

```cpp
std::vector<node*>
```

Role:
- accepting DFA states in the current DFA stage

### `TerStateActionTable`

Type:

```cpp
std::map<int, std::string>
```

Role:
- accepting DFA state ID -> action code before minimization

### `mindfareturn`

Type:

```cpp
std::map<int, std::string>
```

Role:
- accepting minimized DFA state ID -> action code after minimization

## 9. Internal Structures

These are not part of the required report names, but they matter for understanding the implementation.

### `RegexAst`

Defined internally in [`nfa_constructor.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/nfa_constructor.cpp).

Fields:
- `kind`
- `literal`
- `charset`
- `left`
- `right`

Role:
- internal abstract syntax tree for extended regex parsing

### `Fragment`

Defined internally in [`nfa_constructor.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/nfa_constructor.cpp).

Fields:
- `start`
- `accept`

Role:
- Thompson construction stack element

### `Expectation`

Defined locally inside `SeuLexDriver::runSelfTests()` in [`code_generator.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/code_generator.cpp).

Fields:
- `lexeme`
- `expected`

Role:
- simple self-test case representation

## 10. Ownership And Lifetime Summary

| Object | Lifetime owner | Notes |
|---|---|---|
| NFA nodes | internal arena `g_nodeArena` | cleared by `resetGlobalTables()` |
| DFA states | `dfa::nodeVec` | transitions point into the vector |
| minimized DFA states | `dfa::nodeVec` of result DFA | action maps rebuilt after minimization |
| rule metadata | `LexSpecification` | owned by parser output |
| generated lexer source | output file path chosen by caller | emitted by `CodeGenerator::emitLexer()` |

## 11. Data Flow Summary

```text
Lex source
  -> LexSpecification
  -> LexRule.expandedRegex
  -> LexRule.postfixRegex
  -> nfaTable
  -> merged nfa
  -> dfa
  -> minimized dfa
  -> generated lexer source
```
