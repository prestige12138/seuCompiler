# Algorithms

## 1. Lex File Parsing

Implemented in [`lex_parser.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/lex_parser.cpp).

### Goal

Convert a `.l` file into a structured `LexSpecification` containing:

- `definitionsSection`
- `verbatimDefinitions`
- `rulesSection`
- `userSubroutines`
- `rules`

### Steps

1. `readWholeFile()` loads the full file into memory.
2. `findSectionDelimiter()` finds the first and second `%%` separators.
3. `takeBetweenMarkers()` extracts `%{...%}` blocks from the Definitions section.
4. `removeRanges()` removes those verbatim blocks before parsing macro definitions.
5. Definitions are stored into `idreTable`.
6. Rules are collected line by line into `pendingRule`.
7. `splitRegexAndAction()` separates the regex from its action.
8. `isActionBalanced()` keeps accumulating lines until braces are balanced, while ignoring braces inside:
   - string literals
   - character literals
   - block comments
   - line comments

### Notable Parsing Rules

- Only a standalone `%%` line is treated as a section delimiter.
- `%%` inside `%{...%}` or inside rule actions does not split sections.
- Multi-line actions remain valid as long as braces are balanced.

### Complexity

- File load and section split: `O(N)`
- Definition parsing: `O(N)`
- Rule parsing: `O(N)`
- Overall: `O(N)`, where `N` is file size

## 2. Extended RE Expansion

Implemented mainly in [`nfa_constructor.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/nfa_constructor.cpp) by `REExpander`, `expandNamedDefinitions()`, and `ExtendedRegexParser`.

### Goal

Convert Lex-style extended regex syntax into an internal normalized ordinary RE string composed of:

- literal tokens `ch:<ascii>`
- epsilon token `eps`
- explicit concatenation `&`
- union `|`
- Kleene star `*`

### Supported Constructs

- Named definitions: `{DIGIT}`
- Grouping: `( ... )`
- Character classes: `[abc]`
- Negated classes: `[^a-c]`
- Ranges: `[A-Z]`
- Wildcard: `.`
- Quoted strings: `"if"`
- Escapes: `\n`, `\t`, `\\`, `\"`, `\'`, `\0`
- Postfix operators: `*`, `+`, `?`
- Bounded repetition: `{m}`, `{m,n}`, `{m,}`

### Expansion Strategy

#### Named Definitions

`expandNamedDefinitions()` recursively replaces `{NAME}` using `idreTable`.

- Cycles are detected with `recursionGuard`.
- Undefined names raise exceptions.

#### Parsing

`ExtendedRegexParser` uses recursive descent:

- `parseUnion()`
- `parseConcat()`
- `parseRepeat()`
- `parsePrimary()`

This produces an internal `RegexAst`.

#### Normalization

`serializeAst()` turns the AST into a flat token stream with explicit concatenation.

Example:

```text
"if" -> ( ch:105 & ch:102 )
```

### Complexity

- Named expansion: `O(M + K)` in the common case
- Recursive parsing: `O(T)`
- Serialization: `O(T)`
- Overall: linear in regex size after expansion, excluding repeated AST cloning for bounded repetition

## 3. Infix To Postfix Conversion

Implemented by `NFABuilder::toPostfix()`.

### Goal

Convert the normalized infix expression into postfix form so Thompson construction can be stack-driven.

### Operators

- `|`
- `&`
- `*`
- parentheses

### Method

A standard operator-stack algorithm is used:

- operands go directly to output
- `*` is emitted directly because the normalized stream already treats it as postfix
- `|` and `&` use precedence
- parentheses control grouping

### Complexity

`O(T)`, where `T` is the number of normalized tokens.

## 4. Thompson NFA Construction

Implemented by `NFABuilder::buildNFA()`.

### Goal

Construct a single-rule NFA from postfix RE.

### Internal Representation

- `Fragment { start, accept }`
- `node` objects stored in the internal arena `g_nodeArena`
- epsilon edge encoded as `'\0'`

### Construction Rules

- `eps`: create start -> epsilon -> accept
- `ch:x`: create start -x-> accept
- `&`: concatenate two fragments
- `|`: create new split start and join accept
- `*`: create loop and epsilon bypass

### Action Binding

The accept state of each rule NFA is recorded in:

- `nfaterstatetoaction`
- `g_nfaPriorityTable`

This preserves Lex rule priority during DFA construction.

### Complexity

`O(T + E)` where `T` is postfix token count and `E` is the number of created edges.

## 5. NFA Merge

Implemented by `NFABuilder::mergeNFA()`.

### Goal

Combine all rule NFAs into one automaton.

### Method

- create one fresh global start node
- add epsilon edges from that node to each rule-NFA start
- append all rule terminal states to the merged terminal list

### Complexity

`O(R)` where `R` is the number of rule NFAs.

## 6. DFA Determinization

Implemented by:

- `dfa::Eclosure()`
- `DFABuilder::subsetConstruct()`

### Goal

Convert the merged NFA into a DFA by subset construction.

### Algorithm

1. Compute epsilon-closure of `{nfa.start}`.
2. Treat each unique NFA-state subset as one DFA state.
3. For each symbol in `char_set`, compute move + epsilon-closure.
4. Intern subsets using a canonical string key from sorted NFA state IDs.
5. Mark DFA states as accepting if any member NFA state is accepting.

### Priority Preservation

`pickActionFromSet()` chooses the action associated with the accepting NFA state that has the smallest rule priority. This preserves “first rule wins” among matching rules.

### Complexity

Worst case:

`O(2^V * |Sigma|)`

where:

- `V` is the number of NFA states
- `|Sigma|` is the number of input symbols in `char_set`

## 7. DFA Minimization

Implemented by `DFAMinimizer::minimizeDFA()` in [`dfa_minimizer.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/dfa_minimizer.cpp).

### Goal

Reduce DFA state count while preserving transition behavior and accepting actions.

### Key Difference From Basic Minimization

Accepting states are partitioned by action string, not merely by “accepting vs non-accepting”. This is necessary because two accepting states with different actions cannot be merged safely.

### Algorithm

1. Initial partitions:
   - one partition for non-accepting states
   - one partition per distinct accepting action
2. Repeatedly refine partitions using state signatures:
   - current partition ID
   - action string
   - target partition for each symbol in `char_set`
3. Stop when no partition splits further.
4. Build a new minimized DFA from partition representatives.

### Complexity

`O(P * V * |Sigma|)`

where:

- `P` is the number of refinement rounds
- `V` is the number of DFA states
- `|Sigma|` is the alphabet size

## 8. Lexer Code Generation

Implemented by `CodeGenerator::emitLexer()`.

### Goal

Emit a self-contained C++ lexer source file from the minimized DFA and parsed Lex specification.

### Generated Components

- static transition table `kTransitions`
- accept bitmap `kAcceptStates`
- embedded `%{...%}` content
- embedded user subroutines
- `dispatch_action(int state)`
- `reset_source(const std::string&)`
- `int analysis(std::string yytext)`
- `int next_token()`
- `std::vector<int> tokenize(const std::string&)`
- `int input()`

### Runtime Strategy

- `analysis()` evaluates one lexeme directly against the DFA.
- `next_token()` performs longest-prefix scanning over `yy_source`.
- `dispatch_action()` executes the action for an accepting state.
- blank or `;` actions return `0` and behave as skip rules.

### Complexity

- Table generation: `O(V * |Sigma|)`
- Action emission: `O(A)` where `A` is total action text size
- Overall emission: `O(V * |Sigma| + A)`

## 9. Dot Visualization

Implemented by `Visualizer::dumpNFA()` and `Visualizer::dumpDFA()`.

### Goal

Export Graphviz dot graphs for inspection and debugging.

### Method

- NFA export uses BFS from the NFA start node.
- DFA export iterates through `nodeVec`.
- Accepting states use `doublecircle`.
- `escapeDotLabel()` prints readable labels for special characters and epsilon.

### Complexity

`O(V + E)`

## 10. Self-Test Algorithm

Implemented by `SeuLexDriver::runSelfTests()`.

### Coverage Provided By Current Code

- sample lexer generation and compilation
- runtime validation of `analysis()` and `tokenize()`
- regex-level evaluation using minimized DFA
- parser regression tests for:
  - `%%` inside rule actions
  - braces inside comments in multi-line actions
- generation of `minic` and `c99` lexer outputs

### Process

1. Create a temporary directory under `/tmp`.
2. Write a sample `.l` file.
3. Generate and compile a sample lexer and driver.
4. Execute the produced binary with `fork/execvp`.
5. Re-run internal DFA-based checks directly.
6. Generate `minic` and `c99` lexer outputs.

### Complexity

Dominated by:

- one full generation pipeline
- one external compile
- several smaller regex/DFA checks

## 11. End-To-End Pipeline Summary

```text
Lex file
  -> LexParser::parseLexFile
  -> REExpander::expandRE
  -> NFABuilder::toPostfix
  -> NFABuilder::buildNFA
  -> NFABuilder::mergeNFA
  -> DFABuilder::subsetConstruct
  -> DFAMinimizer::minimizeDFA
  -> Visualizer::dumpNFA / dumpDFA
  -> CodeGenerator::emitLexer
```
