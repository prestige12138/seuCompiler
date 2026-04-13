#ifndef LEX_GENERATOR_H
#define LEX_GENERATOR_H

#include <map>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <vector>

class node;
typedef std::multimap<char, node*>::iterator mulit;
typedef std::multimap<char, node*> myMul;

/**
 * @brief NFA/DFA state node reused exactly with the names from the report.
 *
 * The implementation stores outgoing transitions in a multimap so a state can
 * have multiple edges for the same character. `'\0'` is reserved internally as
 * epsilon.
 */
class node {
 public:
  /**
   * @brief Construct a non-accepting state with label 0.
   *
   * Complexity: O(1).
   */
  node();

  /**
   * @brief Construct a state with a concrete label and accept tag.
   *
   * Complexity: O(1).
   */
  node(int state, bool accepttag);

  /**
   * @brief Add one outgoing transition.
   *
   * Complexity: O(log E), where E is the number of outgoing edges of the node.
   */
  void Addoutstate(char ch, node* nd);

  /**
   * @brief Query whether the node is an accepting state.
   *
   * Complexity: O(1).
   */
  bool IsAccepted();
  bool IsAccepted() const;

  /**
   * @brief Update the accepting tag.
   *
   * Complexity: O(1).
   */
  void SetAccept(bool tag);

  /**
   * @brief Return an iterator to the first transition labelled by @p ch.
   *
   * Complexity: O(log E).
   */
  mulit GetNextStates(char ch);

  /**
   * @brief Return the numeric state label.
   *
   * Complexity: O(1).
   */
  int GetState();
  int GetState() const;

  /**
   * @brief Return a copy of the outgoing transition multimap.
   *
   * Complexity: O(E).
   */
  std::multimap<char, node*> getMultimap();
  std::multimap<char, node*> getMultimap() const;

  /**
   * @brief Replace the outgoing transition multimap.
   *
   * Complexity: O(E).
   */
  void setNextState(myMul next);

  /**
   * @brief Overwrite the numeric state label.
   *
   * Complexity: O(1).
   */
  void Setstate(int state);

 private:
  int label;
  bool accepted;
  std::multimap<char, node*> outstate;
};

/**
 * @brief Report-defined NFA aggregate.
 */
typedef struct nfa {
  node* start;
  std::vector<node*> terminal;
} nfa;

/**
 * @brief Report-defined DFA aggregate.
 */
typedef struct dfa {
  node* start;
  std::vector<node> nodeVec;
  std::vector<node> endNode;

  /**
   * @brief Construct a DFA with a starting state pointer.
   *
   * Complexity: O(1).
   */
  dfa(node* st = nullptr);

  /**
   * @brief Compute epsilon closure over an NFA state set.
   *
   * Complexity: O(V + E) over the reachable epsilon subgraph.
   */
  void Eclosure(std::set<node*>& x);

  /**
   * @brief Print the DFA transition relation to stdout.
   *
   * Complexity: O(V + E).
   */
  void printDFA();
} dfa;

extern std::set<char> char_set;
extern std::map<std::string, std::string> idreTable;
extern std::vector<nfa> nfaTable;
extern std::vector<node*> dfaterminals;
extern std::map<int, std::string> nfaterstatetoaction;
extern std::map<int, std::string> TerStateActionTable;
extern std::map<int, std::string> mindfareturn;

/**
 * @brief One Lex rule consisting of a regular expression and an action.
 */
struct LexRule {
  std::string regex;
  std::string action;
  std::size_t priority = 0;
  std::string expandedRegex;
  std::string postfixRegex;
};

/**
 * @brief Parsed three-section Lex source file.
 */
struct LexSpecification {
  std::string definitionsSection;
  std::string verbatimDefinitions;
  std::string rulesSection;
  std::string userSubroutines;
  std::vector<LexRule> rules;
};

/**
 * @brief Parse Lex input files into definitions, rules, and user subroutines.
 */
class LexParser {
 public:
  /**
   * @brief Parse a Lex source file.
   *
   * Complexity: O(N), where N is the file size.
   */
  LexSpecification parseLexFile(const std::string& path) const;
};

/**
 * @brief Expand extended Lex regular expressions into ordinary RE tokens.
 */
class REExpander {
 public:
  /**
   * @brief Expand one extended RE using `idreTable`.
   *
   * The returned string is an explicit-concatenation infix form whose tokens
   * are separated by spaces. Literal tokens use the form `ch:<ascii>`, epsilon
   * uses `eps`, and operators are `|`, `&`, `*`, `(`, `)`.
   *
   * Complexity: O(M + K), where M is regex length and K is the size of the
   * normalized output.
   */
  std::string expandRE(const std::string& raw) const;
};

/**
 * @brief Convert ordinary infix RE to postfix and build Thompson NFAs.
 */
class NFABuilder {
 public:
  /**
   * @brief Convert explicit-concatenation infix RE to postfix form.
   *
   * Complexity: O(T), where T is the number of tokens.
   */
  std::string toPostfix(const std::string& infix) const;

  /**
   * @brief Build a Thompson NFA from postfix tokens.
   *
   * Complexity: O(T + A), where T is the number of tokens and A is the number
   * of generated transitions.
   */
  nfa buildNFA(const std::string& postfix,
               const std::string& action,
               std::size_t priority) const;

  /**
   * @brief Merge multiple rule NFAs with a new epsilon start state.
   *
   * Complexity: O(R), where R is the number of rule NFAs.
   */
  nfa mergeNFA(const std::vector<nfa>& automata) const;
};

/**
 * @brief Determinize an NFA via subset construction.
 */
class DFABuilder {
 public:
  /**
   * @brief Build a DFA from an NFA.
   *
   * Complexity: O(2^V * |Sigma|) in the worst case, standard subset
   * construction bound.
   */
  dfa subsetConstruct(const nfa& automaton) const;
};

/**
 * @brief Minimize a DFA by partition refinement.
 */
class DFAMinimizer {
 public:
  /**
   * @brief Minimize a DFA while preserving accepting actions.
   *
   * Complexity: O(P * V * |Sigma|), where P is the number of refinement
   * iterations.
   */
  dfa minimizeDFA(const dfa& automaton) const;
};

/**
 * @brief Emit a standalone lexer from the minimized DFA.
 */
class CodeGenerator {
 public:
  /**
   * @brief Write a complete lexer C++ source file.
   *
   * Complexity: O(V * |Sigma| + A), where A is the size of emitted actions.
   */
  void emitLexer(const dfa& automaton,
                 const LexSpecification& specification,
                 const std::string& outPath) const;
};

/**
 * @brief Export NFA/DFA visualizations in Graphviz dot format.
 */
class Visualizer {
 public:
  /**
   * @brief Export an NFA to dot.
   *
   * Complexity: O(V + E).
   */
  void dumpNFA(const nfa& automaton, const std::string& path) const;

  /**
   * @brief Export a DFA to dot.
   *
   * Complexity: O(V + E).
   */
  void dumpDFA(const dfa& automaton, const std::string& path) const;
};

/**
 * @brief End-to-end seuLex orchestration facade.
 */
class SeuLexDriver {
 public:
  /**
   * @brief Run the full seuLex pipeline on one Lex file.
   *
   * Complexity: dominated by subset construction and DFA minimization.
   */
  void generate(const std::string& lexPath,
                const std::string& outCppPath,
                const std::string& dotDir) const;

  /**
   * @brief Run built-in regression tests.
   *
   * Complexity: proportional to the total size of the embedded test suite.
   */
  bool runSelfTests(const std::string& workspaceRoot) const;
};

#endif
