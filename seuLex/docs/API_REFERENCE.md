# API Reference

This document is based strictly on the current implementation in `include/` and `src/`.

## 1. CLI Entry

### `int main(int argc, char** argv)`

Defined in [`main.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/main.cpp).

Purpose:
- Parse CLI arguments.
- Dispatch to generation mode or self-test mode.

Parameters:
- `argc`: argument count.
- `argv`: argument vector.

Behavior:
- `--self-test` calls `SeuLexDriver::runSelfTests()`.
- Otherwise expects:
  - `argv[1]`: Lex file path
  - `argv[2]`: optional generated lexer output path
  - `argv[3]`: optional dot output directory

Return:
- `0` on success.
- `1` on usage error or thrown exception.

Dependencies:
- `currentWorkingDirectory()`
- `seu_lex::SeuLexDriver`

## 2. Internal CLI Helper

### `std::string currentWorkingDirectory()`

Defined in anonymous namespace in [`main.cpp`](/Users/llawliet/代码/seuCompiler/seuLex/src/main.cpp).

Purpose:
- Resolve the current workspace root for self-tests.

Return:
- current working directory as `std::string`

Failure:
- throws if `getcwd()` fails

## 3. `node.h`

### `class node`

Report-defined automaton state node.

#### `node()`

Purpose:
- Construct a default non-accepting node.

Parameters:
- none

Return:
- constructed object

Dependencies:
- none

#### `node(int state, bool accepttag)`

Purpose:
- Construct a node with an explicit numeric state label and accept flag.

Parameters:
- `state`: state ID
- `accepttag`: accepting flag

Return:
- constructed object

#### `void Addoutstate(char ch, node* nd)`

Purpose:
- Add an outgoing transition.

Parameters:
- `ch`: edge label, with `'\0'` representing epsilon
- `nd`: target node pointer

Return:
- none

Dependencies:
- `outstate`

#### `bool IsAccepted()`
#### `bool IsAccepted() const`

Purpose:
- Query whether this node is accepting.

Return:
- `true` if accepting, else `false`

#### `void SetAccept(bool tag)`

Purpose:
- Update the accepting flag.

Parameters:
- `tag`: new accept value

Return:
- none

#### `mulit GetNextStates(char ch)`

Purpose:
- Return an iterator to the first matching outgoing transition.

Parameters:
- `ch`: edge label

Return:
- iterator into `outstate`

#### `int GetState()`
#### `int GetState() const`

Purpose:
- Return the numeric state label.

Return:
- state ID

#### `std::multimap<char, node*> getMultimap()`
#### `std::multimap<char, node*> getMultimap() const`

Purpose:
- Return a copy of the outgoing transition multimap.

Return:
- copied transition multimap

Note:
- This is copy-based, not reference-based.

#### `void setNextState(myMul next)`

Purpose:
- Replace the transition multimap.

Parameters:
- `next`: new multimap

Return:
- none

#### `void Setstate(int state)`

Purpose:
- Replace the numeric state label.

Parameters:
- `state`: new state ID

Return:
- none

## 4. `nfa.h`

### `typedef struct nfa`

Fields:
- `node* start`
- `std::vector<node*> terminal`

Purpose:
- Represent one NFA fragment or merged NFA.

### Global Tables

#### `extern std::set<char> char_set`

Purpose:
- Alphabet seen in non-epsilon transitions.

Used by:
- `buildNFA()`
- `subsetConstruct()`
- `minimizeDFA()`

#### `extern std::map<std::string, std::string> idreTable`

Purpose:
- Named regular-definition table from the Definitions section.

Used by:
- `parseLexFile()`
- `expandRE()`

#### `extern std::vector<nfa> nfaTable`

Purpose:
- Stores one NFA per parsed rule before merging.

Used by:
- `SeuLexDriver::generate()`
- `SeuLexDriver::runSelfTests()`

#### `extern std::map<int, std::string> nfaterstatetoaction`

Purpose:
- Map accepting NFA state ID to its action text.

Used by:
- `buildNFA()`
- `pickActionFromSet()`

## 5. `dfa.h`

### `typedef struct dfa`

Fields:
- `node* start`
- `std::vector<node> nodeVec`
- `std::vector<node> endNode`

#### `dfa(node* st = nullptr)`

Purpose:
- Construct a DFA with an optional start pointer.

Parameters:
- `st`: start state pointer

#### `void Eclosure(std::set<node*>& x)`

Purpose:
- Expand an NFA-state set by following epsilon edges.

Parameters:
- `x`: input/output NFA-state set

Return:
- none

Dependencies:
- `node::getMultimap()`
- epsilon symbol `'\0'`

#### `void printDFA()`

Purpose:
- Print all DFA states and outgoing transitions to stdout for debugging.

Return:
- none

### Global Tables

#### `extern std::vector<node*> dfaterminals`

Purpose:
- Accepting DFA state pointers.

#### `extern std::map<int, std::string> TerStateActionTable`

Purpose:
- Accepting DFA state ID to action string.

#### `extern std::map<int, std::string> mindfareturn`

Purpose:
- Accepting minimized DFA state ID to action string.

## 6. `lex_parser.h`

### `struct LexRule`

Fields:
- `regex`
- `action`
- `priority`
- `expandedRegex`
- `postfixRegex`

Purpose:
- Store one parsed rule across all compilation stages.

### `struct LexSpecification`

Fields:
- `definitionsSection`
- `verbatimDefinitions`
- `rulesSection`
- `userSubroutines`
- `rules`

Purpose:
- Store the parsed Lex source file.

### `LexSpecification LexParser::parseLexFile(const std::string& path) const`

Purpose:
- Parse a Lex file into `LexSpecification`.

Parameters:
- `path`: source `.l` file path

Return:
- parsed specification

Failure:
- throws on file open failure
- throws on missing `%%` separators
- throws on unterminated `%{...%}` blocks
- throws on malformed multi-line rules

Dependencies:
- `idreTable`
- internal helpers in `lex_parser.cpp`

## 7. `nfa_constructor.h`

### `std::string REExpander::expandRE(const std::string& raw) const`

Purpose:
- Expand extended Lex regex syntax into normalized ordinary RE form.

Parameters:
- `raw`: original regex text

Return:
- normalized token string

Failure:
- throws on undefined names
- throws on cyclic named definitions
- throws on malformed regex syntax
- throws on repetition overflow or implementation-limit breach

Dependencies:
- `idreTable`
- `expandNamedDefinitions()`
- `ExtendedRegexParser`
- `serializeAst()`

### `std::string NFABuilder::toPostfix(const std::string& infix) const`

Purpose:
- Convert normalized infix RE to postfix form.

Parameters:
- `infix`: normalized RE string

Return:
- postfix token string

Failure:
- throws on mismatched parentheses
- throws on unknown token kinds

### `nfa NFABuilder::buildNFA(const std::string& postfix, const std::string& action, std::size_t priority) const`

Purpose:
- Build one Thompson NFA from one postfix RE.

Parameters:
- `postfix`: postfix token string
- `action`: action attached to the accepting state
- `priority`: rule priority, lower means earlier rule

Return:
- constructed NFA

Failure:
- throws on malformed postfix RE
- throws on unsupported literal range

Dependencies:
- `char_set`
- `nfaterstatetoaction`
- internal arena `g_nodeArena`
- internal priority table `g_nfaPriorityTable`

### `nfa NFABuilder::mergeNFA(const std::vector<nfa>& automata) const`

Purpose:
- Merge multiple rule NFAs under one epsilon start node.

Parameters:
- `automata`: per-rule NFAs

Return:
- merged NFA

Dependencies:
- internal state allocator

### `dfa DFABuilder::subsetConstruct(const nfa& automaton) const`

Purpose:
- Determinize an NFA using subset construction.

Parameters:
- `automaton`: merged NFA

Return:
- constructed DFA

Dependencies:
- `char_set`
- `TerStateActionTable`
- `dfaterminals`
- `pickActionFromSet()`
- `dfa::Eclosure()`

### `void resetGlobalTables()`

Purpose:
- Reset all report-defined global state and internal construction arenas.

Return:
- none

Dependencies reset:
- `char_set`
- `idreTable`
- `nfaTable`
- `dfaterminals`
- `nfaterstatetoaction`
- `TerStateActionTable`
- `mindfareturn`
- internal `g_nfaPriorityTable`
- internal `g_nodeArena`
- internal next-state counter

## 8. `dfa_minimizer.h`

### `dfa DFAMinimizer::minimizeDFA(const dfa& automaton) const`

Purpose:
- Minimize DFA states while preserving accepting action semantics.

Parameters:
- `automaton`: source DFA

Return:
- minimized DFA

Dependencies:
- `char_set`
- `TerStateActionTable`
- `mindfareturn`
- `dfaterminals`

Failure:
- assumes state IDs are valid indices into `nodeVec`

## 9. `code_generator.h`

### `void CodeGenerator::emitLexer(const dfa& automaton, const LexSpecification& specification, const std::string& outPath) const`

Purpose:
- Emit standalone C++ lexer source code.

Parameters:
- `automaton`: minimized DFA
- `specification`: parsed Lex source, used for verbatim blocks and user code
- `outPath`: output C++ file path

Return:
- none

Failure:
- throws if DFA has no start state
- throws if output file cannot be opened
- throws if output directories cannot be created

Dependencies:
- `mindfareturn`
- `TerStateActionTable`
- internal filesystem helpers in `code_generator.cpp`

### `void Visualizer::dumpNFA(const nfa& automaton, const std::string& path) const`

Purpose:
- Emit Graphviz dot for an NFA.

Parameters:
- `automaton`: source NFA
- `path`: output path

Return:
- none

Dependencies:
- `escapeDotLabel()`
- `ensureDirectory()`

### `void Visualizer::dumpDFA(const dfa& automaton, const std::string& path) const`

Purpose:
- Emit Graphviz dot for a DFA.

Parameters:
- `automaton`: source DFA
- `path`: output path

Return:
- none

### `void SeuLexDriver::generate(const std::string& lexPath, const std::string& outCppPath, const std::string& dotDir) const`

Purpose:
- Run the full seuLex generation pipeline.

Parameters:
- `lexPath`: source `.l` file
- `outCppPath`: generated lexer output path
- `dotDir`: visualization output directory

Return:
- none

Dependencies:
- `resetGlobalTables()`
- `LexParser`
- `REExpander`
- `NFABuilder`
- `DFABuilder`
- `DFAMinimizer`
- `CodeGenerator`
- `Visualizer`
- `nfaTable`

### `bool SeuLexDriver::runSelfTests(const std::string& workspaceRoot) const`

Purpose:
- Run built-in generation, parser, regex, and smoke tests.

Parameters:
- `workspaceRoot`: current working directory used to locate repository resources

Return:
- `true` if all internal checks pass
- `false` if any internal logical check fails

Failure:
- throws on temp directory creation failure
- throws on sample lexer compile failure
- throws on sample lexer runtime failure
- throws if repository resources cannot be located

Dependencies:
- `resolveRepoRoot()`
- `makeTempDir()`
- `runProcess()`
- full generation pipeline

## 10. Internal Helpers In `lex_parser.cpp`

### `findSectionDelimiter(const std::string&, std::size_t)`

Purpose:
- Find a standalone `%%` line starting at a given offset.

### `trim(const std::string&)`

Purpose:
- Remove leading and trailing whitespace.

### `splitByLines(const std::string&)`

Purpose:
- Split text into line vector while preserving trailing empty line information.

### `stripInlineComment(const std::string&)`

Purpose:
- Remove `/* ...` inline suffix in definition lines.

### `isActionBalanced(const std::string&)`

Purpose:
- Decide whether a possibly multi-line action block is complete.

### `splitRegexAndAction(const std::string&)`

Purpose:
- Split one accumulated rule string into regex and action text.

### `readWholeFile(const std::string&)`

Purpose:
- Read a full file into memory.

### `takeBetweenMarkers(const std::string&, const std::string&, const std::string&, std::vector<std::pair<std::size_t, std::size_t>>*)`

Purpose:
- Extract all blocks between markers and optionally record source ranges.

### `removeRanges(const std::string&, const std::vector<std::pair<std::size_t, std::size_t>>&)`

Purpose:
- Remove a set of source ranges from text.

## 11. Internal Helpers In `nfa_constructor.cpp`

### State And Storage Helpers

- `isIdentifierLike()`: validate named-definition references
- `splitSpaceTokens()`: split normalized token streams
- `joinKey()`: canonicalize NFA-state subsets
- `createState()`: allocate one node from the arena
- `buildAsciiUniverse()`: create ASCII character domain
- `decodeEscape()`: decode escape sequences

### Regex AST Helpers

- `makeLiteral()`
- `makeEpsilon()`
- `makeSet()`
- `makeConcat()`
- `makeUnion()`
- `makeStar()`
- `cloneAst()`
- `serializeAst()`

Purpose:
- Construct, copy, and serialize regex AST nodes.

### Named-Definition Expansion

- `expandNamedDefinitions()`

Purpose:
- Recursively expand `{NAME}` references.

### `class ExtendedRegexParser`

Internal recursive-descent parser with methods:

- `parse()`
- `parseUnion()`
- `parseConcat()`
- `parseRepeat()`
- `parseBoundedRepeat()`
- `parsePrimary()`
- `parseQuotedString()`
- `parseCharacterClass()`
- `parseInteger()`
- `skipSpace()`
- `expect()`
- `peek()`
- `canStartPrimary()`

### Thompson/DFA Helpers

- `Fragment`: NFA fragment pair
- `pickActionFromSet()`: choose highest-priority accepting action from an NFA-state subset

## 12. Internal Helpers In `dfa_minimizer.cpp`

### `actionForState(int stateId)`

Purpose:
- Resolve the best action table currently in force for a DFA state.

## 13. Internal Helpers In `code_generator.cpp`

### Path And Process Helpers

- `trim()`
- `dirnameOf()`
- `joinPath()`
- `ensureDirectory()`
- `makeTempDir()`
- `fileExists()`
- `resolveRepoRoot()`
- `runProcess()`

Purpose:
- Support path creation, repository discovery, and self-test subprocess execution.

### Codegen Helpers

- `escapeDotLabel()`: printable labels for dot output
- `actionForState()`: resolve action text for emitted tables
- `evaluateDFA()`: internal DFA evaluator used by self-test
