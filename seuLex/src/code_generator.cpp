#include "code_generator.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <stdexcept>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "dfa_builder.h"
#include "dfa_minimizer.h"
#include "nfa_constructor.h"
#include "regex_expander.h"

namespace seu_lex {
namespace {

constexpr int kAsciiLimit = 128;
constexpr char kEpsilon = '\0';

#ifdef SEU_LEX_TEST_CXX
constexpr const char* kSelfTestCompiler = SEU_LEX_TEST_CXX;
#else
constexpr const char* kSelfTestCompiler = "c++";
#endif

std::string trim(const std::string& input) {
  std::size_t begin = 0;
  while (begin < input.size() && std::isspace(static_cast<unsigned char>(input[begin])) != 0) {
    ++begin;
  }
  std::size_t end = input.size();
  while (end > begin && std::isspace(static_cast<unsigned char>(input[end - 1])) != 0) {
    --end;
  }
  return input.substr(begin, end - begin);
}

std::string dirnameOf(const std::string& path) {
  const std::size_t slash = path.find_last_of("/\\");
  if (slash == std::string::npos) {
    return "";
  }
  return path.substr(0, slash);
}

std::string joinPath(const std::string& left, const std::string& right) {
  if (left.empty()) {
    return right;
  }
  if (left.back() == '/') {
    return left + right;
  }
  return left + "/" + right;
}

void ensureDirectory(const std::string& path) {
  if (path.empty() || path == ".") {
    return;
  }
  std::size_t start = 0;
  if (!path.empty() && path[0] == '/') {
    start = 1;
  }
  while (true) {
    const std::size_t slash = path.find('/', start);
    const std::string current = slash == std::string::npos ? path : path.substr(0, slash);
    if (!current.empty() && current != ".") {
      if (::mkdir(current.c_str(), 0755) != 0 && errno != EEXIST) {
        throw std::runtime_error("failed to create directory: " + current);
      }
    }
    if (slash == std::string::npos) {
      break;
    }
    start = slash + 1;
  }
}

std::string makeTempDir() {
  char pattern[] = "/tmp/seuLex_runtimeXXXXXX";
  char* created = ::mkdtemp(pattern);
  if (created == nullptr) {
    throw std::runtime_error("failed to create temporary directory");
  }
  return std::string(created);
}

bool fileExists(const std::string& path) {
  struct stat info {};
  return ::stat(path.c_str(), &info) == 0;
}

std::string escapeDotLabel(char ch) {
  switch (ch) {
    case '\n':
      return "\\n";
    case '\t':
      return "\\t";
    case '\r':
      return "\\r";
    case '\f':
      return "\\f";
    case '\v':
      return "\\v";
    case '"':
      return "\\\"";
    case '\\':
      return "\\\\";
    case kEpsilon:
      return "eps";
    default:
      if (std::isprint(static_cast<unsigned char>(ch)) != 0) {
        return std::string(1, ch);
      }
      std::ostringstream oss;
      oss << "\\x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
          << static_cast<int>(static_cast<unsigned char>(ch));
      return oss.str();
  }
}

std::string actionForState(int stateId) {
  const auto minimal = mindfareturn.find(stateId);
  if (minimal != mindfareturn.end()) {
    return minimal->second;
  }
  const auto terminal = TerStateActionTable.find(stateId);
  if (terminal != TerStateActionTable.end()) {
    return terminal->second;
  }
  return "";
}

int evaluateDFA(const dfa& automaton, const std::string& yytext) {
  if (automaton.start == nullptr) {
    return -1;
  }
  const node* current = automaton.start;
  for (unsigned char byte : yytext) {
    bool matched = false;
    const auto transitions = current->getMultimap();
    const auto range = transitions.equal_range(static_cast<char>(byte));
    for (auto it = range.first; it != range.second; ++it) {
      current = it->second;
      matched = true;
      break;
    }
    if (!matched) {
      return -1;
    }
  }
  if (!current->IsAccepted()) {
    return -1;
  }
  const std::string action = actionForState(current->GetState());
  if (action.find("return") == std::string::npos) {
    return 0;
  }
  std::size_t pos = action.find("return") + 6;
  while (pos < action.size() && std::isspace(static_cast<unsigned char>(action[pos])) != 0) {
    ++pos;
  }
  std::size_t end = pos;
  while (end < action.size() &&
         (std::isalnum(static_cast<unsigned char>(action[end])) != 0 || action[end] == '_')) {
    ++end;
  }
  const std::string token = trim(action.substr(pos, end - pos));
  if (token.empty()) {
    return 0;
  }
  if (std::all_of(token.begin(), token.end(), [](unsigned char ch) {
        return std::isdigit(ch) != 0;
      })) {
    return std::stoi(token);
  }
  static std::unordered_map<std::string, int> tokenIds;
  static int nextTokenId = 1;
  const auto inserted = tokenIds.emplace(token, nextTokenId);
  if (inserted.second) {
    ++nextTokenId;
  }
  return inserted.first->second;
}

std::string resolveRepoRoot(const std::string& workspaceRoot) {
  const std::vector<std::string> candidates = {
      workspaceRoot,
      joinPath(workspaceRoot, ".."),
      joinPath(joinPath(workspaceRoot, ".."), ".."),
  };
  for (const std::string& candidate : candidates) {
    if (fileExists(joinPath(candidate, "resources/minic.l")) &&
        fileExists(joinPath(candidate, "resources/c99.l"))) {
      return candidate;
    }
  }
  throw std::runtime_error("failed to locate repository resources directory");
}

int runProcess(const std::string& program, const std::vector<std::string>& args) {
  std::vector<char*> argv;
  argv.reserve(args.size() + 2);
  argv.push_back(const_cast<char*>(program.c_str()));
  for (const std::string& arg : args) {
    argv.push_back(const_cast<char*>(arg.c_str()));
  }
  argv.push_back(nullptr);

  const pid_t pid = ::fork();
  if (pid < 0) {
    throw std::runtime_error("failed to fork child process");
  }
  if (pid == 0) {
    ::execvp(program.c_str(), argv.data());
    std::perror("execvp");
    _exit(127);
  }

  int status = 0;
  if (::waitpid(pid, &status, 0) < 0) {
    throw std::runtime_error("failed to wait for child process");
  }
  if (!WIFEXITED(status)) {
    return -1;
  }
  return WEXITSTATUS(status);
}

}  // namespace

void CodeGenerator::emitLexer(const dfa& automaton,
                              const LexSpecification& specification,
                              const std::string& outPath) const {
  if (automaton.start == nullptr) {
    throw std::runtime_error("cannot emit lexer from an empty DFA");
  }
  ensureDirectory(dirnameOf(outPath));

  std::vector<std::array<int, kAsciiLimit>> table(automaton.nodeVec.size());
  for (auto& row : table) {
    row.fill(-1);
  }
  for (const node& state : automaton.nodeVec) {
    const auto transitions = state.getMultimap();
    for (const auto& entry : transitions) {
      table[state.GetState()][static_cast<unsigned char>(entry.first)] = entry.second->GetState();
    }
  }

  std::ofstream output(outPath);
  if (!output) {
    throw std::runtime_error("failed to open generated lexer path: " + outPath);
  }

  output << "#include <array>\n"
         << "#include <cstddef>\n"
         << "#include <iostream>\n"
         << "#include <stdexcept>\n"
         << "#include <string>\n"
         << "#include <vector>\n\n"
         << specification.verbatimDefinitions << '\n'
         << "struct SeuLexToken {\n"
         << "  int type = 0;\n"
         << "  std::string lexeme;\n"
         << "  int line = 0;\n"
         << "  int column = 0;\n"
         << "};\n\n"
         << "static constexpr std::size_t kYYTextCapacity = 1u << 20;\n"
         << "static std::string yytext_storage;\n"
         << "char yytext[kYYTextCapacity] = {0};\n"
         << "static std::string yy_source;\n"
         << "static std::size_t yy_cursor = 0;\n"
         << "int yylineno = 1;\n"
         << "int column = 1;\n"
         << "#ifndef ECHO\n"
         << "#define ECHO do { std::cout << yytext; } while (0)\n"
         << "#endif\n"
         << "#ifdef YY_USER_INIT\n"
         << "#define SEU_LEX_CALL_USER_INIT() do { YY_USER_INIT; } while (false)\n"
         << "#else\n"
         << "#define SEU_LEX_CALL_USER_INIT() do {} while (false)\n"
         << "#endif\n\n"
         << "static void sync_yytext() {\n"
         << "  if (yytext_storage.size() >= kYYTextCapacity) {\n"
         << "    throw std::runtime_error(\"matched lexeme exceeds yytext capacity\");\n"
         << "  }\n"
         << "  for (std::size_t index = 0; index < yytext_storage.size(); ++index) {\n"
         << "    yytext[index] = yytext_storage[index];\n"
         << "  }\n"
         << "  yytext[yytext_storage.size()] = '\\0';\n"
         << "}\n\n"
         << "static void advance_position(unsigned char ch, int* line, int* current_column) {\n"
         << "  if (ch == '\\n') {\n"
         << "    ++(*line);\n"
         << "    *current_column = 1;\n"
         << "    return;\n"
         << "  }\n"
         << "  ++(*current_column);\n"
         << "}\n\n"
         << "int input() {\n"
         << "  if (yy_cursor >= yy_source.size()) {\n"
         << "    return 0;\n"
         << "  }\n"
         << "  const unsigned char ch = static_cast<unsigned char>(yy_source[yy_cursor]);\n"
         << "  ++yy_cursor;\n"
         << "  advance_position(ch, &yylineno, &column);\n"
         << "  return ch;\n"
         << "}\n\n"
         << "static const int kStartState = " << automaton.start->GetState() << ";\n"
         << "static const std::vector<std::array<int, " << kAsciiLimit << ">> kTransitions = {\n";

  for (std::size_t row = 0; row < table.size(); ++row) {
    output << "  {";
    for (int col = 0; col < kAsciiLimit; ++col) {
      if (col != 0) {
        output << ", ";
      }
      output << table[row][col];
    }
    output << "}";
    if (row + 1 != table.size()) {
      output << ",";
    }
    output << '\n';
  }
  output << "};\n\n";

  output << "static const std::vector<int> kAcceptStates = {";
  for (std::size_t index = 0; index < automaton.nodeVec.size(); ++index) {
    if (index != 0) {
      output << ", ";
    }
    output << (actionForState(static_cast<int>(index)).empty() ? 0 : 1);
  }
  output << "};\n\n";

  output << specification.userSubroutines << '\n';

  output << "static int dispatch_action(int state) {\n"
         << "  switch (state) {\n";
  const std::map<int, std::string>& actionTable =
      mindfareturn.empty() ? TerStateActionTable : mindfareturn;
  for (const auto& entry : actionTable) {
    output << "    case " << entry.first << ":\n";
    if (trim(entry.second).empty() || trim(entry.second) == ";") {
      output << "      return 0;\n";
    } else {
      output << "      do {\n"
             << "        " << entry.second << "\n"
             << "      } while (false);\n"
             << "      return 0;\n";
    }
  }
  output << "    default:\n"
         << "      return -1;\n"
         << "  }\n"
         << "}\n\n"
         << "static void reset_source(const std::string& source) {\n"
         << "  yy_source = source;\n"
         << "  yy_cursor = 0;\n"
         << "  yytext_storage.clear();\n"
         << "  yytext[0] = '\\0';\n"
         << "  yylineno = 1;\n"
         << "  column = 1;\n"
         << "  SEU_LEX_CALL_USER_INIT();\n"
         << "}\n\n"
         << "int analysis(std::string yytext_input) {\n"
         << "  yytext_storage = yytext_input;\n"
         << "  sync_yytext();\n"
         << "  int state = kStartState;\n"
         << "  for (unsigned char ch : yytext_storage) {\n"
         << "    if (ch >= " << kAsciiLimit << ") {\n"
         << "      return -1;\n"
         << "    }\n"
         << "    state = kTransitions[state][ch];\n"
         << "    if (state < 0) {\n"
         << "      return -1;\n"
         << "    }\n"
         << "  }\n"
         << "  return dispatch_action(state);\n"
         << "}\n\n"
         << "static int lex_one(SeuLexToken* token_out) {\n"
         << "  if (yy_cursor >= yy_source.size()) {\n"
         << "    return 0;\n"
         << "  }\n"
         << "  const std::size_t start = yy_cursor;\n"
         << "  const int start_line = yylineno;\n"
         << "  const int start_column = column;\n"
         << "  std::size_t pos = yy_cursor;\n"
         << "  int scan_line = yylineno;\n"
         << "  int scan_column = column;\n"
         << "  int state = kStartState;\n"
         << "  int last_accept_state = kAcceptStates[state] != 0 ? state : -1;\n"
         << "  std::size_t last_accept_pos = start;\n"
         << "  int last_accept_line = start_line;\n"
         << "  int last_accept_column = start_column;\n"
         << "  while (pos < yy_source.size()) {\n"
         << "    const unsigned char ch = static_cast<unsigned char>(yy_source[pos]);\n"
         << "    if (ch >= " << kAsciiLimit << ") {\n"
         << "      break;\n"
         << "    }\n"
         << "    const int next = kTransitions[state][ch];\n"
         << "    if (next < 0) {\n"
         << "      break;\n"
         << "    }\n"
         << "    state = next;\n"
         << "    ++pos;\n"
         << "    advance_position(ch, &scan_line, &scan_column);\n"
         << "    if (kAcceptStates[state] != 0) {\n"
         << "      last_accept_state = state;\n"
         << "      last_accept_pos = pos;\n"
         << "      last_accept_line = scan_line;\n"
         << "      last_accept_column = scan_column;\n"
         << "    }\n"
         << "  }\n"
         << "  if (last_accept_state < 0) {\n"
         << "    yytext_storage = yy_source.substr(start, 1);\n"
         << "    sync_yytext();\n"
         << "    if (token_out != nullptr) {\n"
         << "      token_out->type = -1;\n"
         << "      token_out->lexeme = yytext_storage;\n"
         << "      token_out->line = start_line;\n"
         << "      token_out->column = start_column;\n"
         << "    }\n"
         << "    if (yy_cursor < yy_source.size()) {\n"
         << "      const unsigned char ch = static_cast<unsigned char>(yy_source[yy_cursor]);\n"
         << "      ++yy_cursor;\n"
         << "      advance_position(ch, &yylineno, &column);\n"
         << "    }\n"
         << "    return -1;\n"
         << "  }\n"
         << "  yytext_storage = yy_source.substr(start, last_accept_pos - start);\n"
         << "  sync_yytext();\n"
         << "  if (last_accept_pos == start && yy_cursor < yy_source.size()) {\n"
         << "    const unsigned char ch = static_cast<unsigned char>(yy_source[yy_cursor]);\n"
         << "    ++yy_cursor;\n"
         << "    advance_position(ch, &yylineno, &column);\n"
         << "  } else {\n"
         << "    yy_cursor = last_accept_pos;\n"
         << "    yylineno = last_accept_line;\n"
         << "    column = last_accept_column;\n"
         << "  }\n"
         << "  const int token = dispatch_action(last_accept_state);\n"
         << "  if (token_out != nullptr) {\n"
         << "    token_out->type = token;\n"
         << "    token_out->lexeme = yytext_storage;\n"
         << "    token_out->line = start_line;\n"
         << "    token_out->column = start_column;\n"
         << "  }\n"
         << "  return token;\n"
         << "}\n\n"
         << "int next_token() {\n"
         << "  return lex_one(nullptr);\n"
         << "}\n\n"
         << "std::vector<SeuLexToken> tokenize_detailed(const std::string& source) {\n"
         << "  reset_source(source);\n"
         << "  std::vector<SeuLexToken> tokens;\n"
         << "  while (yy_cursor < yy_source.size()) {\n"
         << "    SeuLexToken token;\n"
         << "    lex_one(&token);\n"
         << "    if (token.type == 0) {\n"
         << "      continue;\n"
         << "    }\n"
         << "    tokens.push_back(token);\n"
         << "  }\n"
         << "  return tokens;\n"
         << "}\n\n"
         << "std::vector<int> tokenize(const std::string& source) {\n"
          << "  std::vector<int> tokens;\n"
         << "  for (const SeuLexToken& token : tokenize_detailed(source)) {\n"
         << "    tokens.push_back(token.type);\n"
         << "  }\n"
         << "  return tokens;\n"
         << "}\n";
}

void Visualizer::dumpNFA(const nfa& automaton, const std::string& path) const {
  ensureDirectory(dirnameOf(path));
  std::ofstream output(path);
  if (!output) {
    throw std::runtime_error("failed to write NFA dot file: " + path);
  }
  output << "digraph NFA {\n  rankdir=LR;\n";
  std::queue<node*> work;
  std::unordered_set<int> visited;
  work.push(automaton.start);
  visited.insert(automaton.start->GetState());
  while (!work.empty()) {
    node* current = work.front();
    work.pop();
    output << "  " << current->GetState()
           << " [shape=" << (current->IsAccepted() ? "doublecircle" : "circle") << "];\n";
    const auto transitions = current->getMultimap();
    for (const auto& entry : transitions) {
      output << "  " << current->GetState() << " -> " << entry.second->GetState() << " [label=\""
             << escapeDotLabel(entry.first) << "\"];\n";
      if (visited.insert(entry.second->GetState()).second) {
        work.push(entry.second);
      }
    }
  }
  output << "}\n";
}

void Visualizer::dumpDFA(const dfa& automaton, const std::string& path) const {
  ensureDirectory(dirnameOf(path));
  std::ofstream output(path);
  if (!output) {
    throw std::runtime_error("failed to write DFA dot file: " + path);
  }
  output << "digraph DFA {\n  rankdir=LR;\n";
  for (const node& state : automaton.nodeVec) {
    output << "  " << state.GetState()
           << " [shape=" << (state.IsAccepted() ? "doublecircle" : "circle") << "];\n";
    const auto transitions = state.getMultimap();
    for (const auto& entry : transitions) {
      output << "  " << state.GetState() << " -> " << entry.second->GetState() << " [label=\""
             << escapeDotLabel(entry.first) << "\"];\n";
    }
  }
  output << "}\n";
}

void SeuLexDriver::generate(const std::string& lexPath,
                            const std::string& outCppPath,
                            const std::string& dotDir) const {
  resetGlobalTables();

  LexParser parser;
  REExpander expander;
  NFABuilder nfaBuilder;
  DFABuilder dfaBuilder;
  DFAMinimizer minimizer;
  CodeGenerator generator;
  Visualizer visualizer;

  LexSpecification spec = parser.parseLexFile(lexPath);
  for (LexRule& rule : spec.rules) {
    rule.expandedRegex = expander.expandRE(rule.regex);
    rule.postfixRegex = nfaBuilder.toPostfix(rule.expandedRegex);
    nfaTable.push_back(nfaBuilder.buildNFA(rule.postfixRegex, rule.action, rule.priority));
  }

  const nfa mergedNfa = nfaBuilder.mergeNFA(nfaTable);
  const dfa rawDfa = dfaBuilder.subsetConstruct(mergedNfa);
  const dfa minDfa = minimizer.minimizeDFA(rawDfa);

  ensureDirectory(dotDir);
  visualizer.dumpNFA(mergedNfa, joinPath(dotDir, "merged_nfa.dot"));
  visualizer.dumpDFA(rawDfa, joinPath(dotDir, "dfa.dot"));
  visualizer.dumpDFA(minDfa, joinPath(dotDir, "min_dfa.dot"));
  generator.emitLexer(minDfa, spec, outCppPath);
}

bool SeuLexDriver::runSelfTests(const std::string& workspaceRoot) const {
  const std::string repoRoot = resolveRepoRoot(workspaceRoot);
  const std::string tempDir = makeTempDir();
  const std::string specPath = joinPath(tempDir, "sample.l");
  const std::string outPath = joinPath(tempDir, "generated_sample_lexer.cpp");
  const std::string driverPath = joinPath(tempDir, "generated_sample_driver.cpp");
  const std::string binaryPath = joinPath(tempDir, "generated_sample_driver");
  const std::string dotDir = joinPath(tempDir, "dot");

  const std::string sampleSpec =
      "%{\n"
      "#include <string>\n"
      "%}\n"
      "DIGIT [0-9]\n"
      "ALPHA [A-Za-z_]\n"
      "%%\n"
      "\"if\" { return 1; }\n"
      "\"else\" { return 2; }\n"
      "{ALPHA}({ALPHA}|{DIGIT})* { return 3; }\n"
      "{DIGIT}+ { return 4; }\n"
      "[ \\t\\n]+ ;\n"
      "%%\n"
      "int yywrap() { return 1; }\n";

  const std::string actionDelimiterSpec =
      "%%\n"
      "\"a\" { puts(\"%%\"); return 1; }\n"
      "%%\n"
      "int yywrap() { return 1; }\n";

  const std::string commentBraceSpec =
      "%%\n"
      "\"a\" {\n"
      "  /* } */\n"
      "  return 1;\n"
      "}\n"
      "%%\n"
      "int yywrap() { return 1; }\n";

  {
    std::ofstream sampleFile(specPath);
    sampleFile << sampleSpec;
  }

  generate(specPath, outPath, dotDir);
  {
    std::ofstream driverFile(driverPath);
    driverFile
        << "#include <iostream>\n"
        << "#include <string>\n"
        << "#include <vector>\n"
        << "int analysis(std::string yytext);\n"
        << "std::vector<int> tokenize(const std::string& source);\n"
        << "int main() {\n"
        << "  const int a = analysis(\"if\");\n"
        << "  const int b = analysis(\"abc123\");\n"
        << "  const int c = analysis(\"42\");\n"
        << "  const auto tokens = tokenize(\"if 42 abc123\");\n"
        << "  std::cout << a << ' ' << b << ' ' << c << ' ' << tokens.size() << '\\n';\n"
        << "  return (a == 1 && b == 3 && c == 4 && tokens.size() == 3 && tokens[0] == 1 && "
           "tokens[1] == 4 && tokens[2] == 3) ? 0 : 1;\n"
        << "}\n";
  }
  if (runProcess(kSelfTestCompiler,
                 {"-std=c++17", outPath, driverPath, "-o", binaryPath}) != 0) {
    throw std::runtime_error("generated sample lexer failed to compile");
  }
  if (runProcess(binaryPath, {}) != 0) {
    throw std::runtime_error("generated sample lexer failed runtime validation");
  }

  const std::string overflowSpecPath = joinPath(tempDir, "overflow.l");
  const std::string overflowOutPath = joinPath(tempDir, "generated_overflow_lexer.cpp");
  const std::string overflowDriverPath = joinPath(tempDir, "generated_overflow_driver.cpp");
  const std::string overflowBinaryPath = joinPath(tempDir, "generated_overflow_driver");
  const std::string overflowSpec =
      "%{\n"
      "#include <cstring>\n"
      "%}\n"
      "%%\n"
      "a+ { return static_cast<int>(std::strlen(yytext)); }\n"
      "%%\n"
      "int yywrap() { return 1; }\n";
  {
    std::ofstream overflowFile(overflowSpecPath);
    overflowFile << overflowSpec;
  }
  generate(overflowSpecPath, overflowOutPath, dotDir);
  {
    std::ofstream overflowDriver(overflowDriverPath);
    overflowDriver
        << "#include <stdexcept>\n"
        << "#include <string>\n"
        << "int analysis(std::string yytext);\n"
        << "int main() {\n"
        << "  try {\n"
        << "    (void)analysis(std::string((1u << 20), 'a'));\n"
        << "  } catch (const std::runtime_error&) {\n"
        << "    return 0;\n"
        << "  }\n"
        << "  return 1;\n"
        << "}\n";
  }
  if (runProcess(kSelfTestCompiler,
                 {"-std=c++17", overflowOutPath, overflowDriverPath, "-o", overflowBinaryPath}) != 0) {
    throw std::runtime_error("generated overflow lexer failed to compile");
  }
  if (runProcess(overflowBinaryPath, {}) != 0) {
    throw std::runtime_error("generated overflow lexer failed overflow validation");
  }

  resetGlobalTables();
  LexParser parser;
  REExpander expander;
  NFABuilder nfaBuilder;
  DFABuilder dfaBuilder;
  DFAMinimizer minimizer;

  LexSpecification spec = parser.parseLexFile(specPath);
  for (LexRule& rule : spec.rules) {
    rule.expandedRegex = expander.expandRE(rule.regex);
    rule.postfixRegex = nfaBuilder.toPostfix(rule.expandedRegex);
    nfaTable.push_back(nfaBuilder.buildNFA(rule.postfixRegex, rule.action, rule.priority));
  }
  const dfa minimized =
      minimizer.minimizeDFA(dfaBuilder.subsetConstruct(nfaBuilder.mergeNFA(nfaTable)));

  struct Expectation {
    std::string lexeme;
    int expected;
  };
  bool allPassed = true;
  const std::vector<Expectation> cases = {
      {"if", 1}, {"else", 2}, {"abc123", 3}, {"42", 4}, {" \t", 0}, {"@", -1}};
  for (const Expectation& item : cases) {
    const int actual = evaluateDFA(minimized, item.lexeme);
    std::cout << "[self-test] " << std::quoted(item.lexeme) << " => " << actual
              << " (expected " << item.expected << ")\n";
    if (actual != item.expected) {
      allPassed = false;
    }
  }

  auto checkRegex = [&](const std::string& rawRegex,
                        const std::vector<Expectation>& localCases,
                        const std::map<std::string, std::string>& definitions = {}) {
    resetGlobalTables();
    idreTable = definitions;
    REExpander localExpander;
    NFABuilder localNfaBuilder;
    DFABuilder localDfaBuilder;
    DFAMinimizer localMinimizer;
    const std::string expanded = localExpander.expandRE(rawRegex);
    const std::string postfix = localNfaBuilder.toPostfix(expanded);
    const nfa localNfa = localNfaBuilder.buildNFA(postfix, "return 7;", 0);
    const dfa localDfa =
        localMinimizer.minimizeDFA(localDfaBuilder.subsetConstruct(localNfa));
    for (const Expectation& localCase : localCases) {
      const int actual = evaluateDFA(localDfa, localCase.lexeme);
      std::cout << "[regex-test] " << rawRegex << " / " << std::quoted(localCase.lexeme)
                << " => " << actual << " (expected " << localCase.expected << ")\n";
      if (actual != localCase.expected) {
        allPassed = false;
      }
    }
  };

  auto expectExpandFailure = [&](const std::string& rawRegex,
                                 const std::map<std::string, std::string>& definitions = {}) {
    resetGlobalTables();
    idreTable = definitions;
    REExpander localExpander;
    bool failed = false;
    try {
      static_cast<void>(localExpander.expandRE(rawRegex));
    } catch (const std::exception&) {
      failed = true;
    }
    std::cout << "[regex-error] " << rawRegex << " => " << (failed ? "failed" : "unexpected-pass")
              << '\n';
    if (!failed) {
      allPassed = false;
    }
  };

  checkRegex(".", {{"a", 7}, {"\n", -1}});
  checkRegex("[^a-c]", {{"z", 7}, {"b", -1}});
  checkRegex("a?", {{"", 7}, {"a", 7}, {"aa", -1}});
  checkRegex("a{2,4}", {{"a", -1}, {"aa", 7}, {"aaa", 7}, {"aaaa", 7}, {"aaaaa", -1}});
  checkRegex("\"\"", {{"", 7}, {"x", -1}});
  checkRegex("\\\"", {{"\"", 7}, {"a", -1}});
  checkRegex("{DIGIT}{DIGIT}", {{"42", 7}, {"4", -1}}, {{"DIGIT", "[0-9]"}});  
  expectExpandFailure("|a");
  expectExpandFailure("a|");
  expectExpandFailure("a||b");
  expectExpandFailure("{MISSING}");

  {
    resetGlobalTables();
    DFABuilder localDfaBuilder;
    const dfa emptyDfa = localDfaBuilder.subsetConstruct(nfa{});
    const bool emptyOk = emptyDfa.start == nullptr && emptyDfa.nodeVec.empty();
    std::cout << "[dfa-empty] " << (emptyOk ? "ok" : "failed") << '\n';
    if (!emptyOk) {
      allPassed = false;
    }
  }

  const auto expectLexParseSuccess = [&](const std::string& fileName, const std::string& specText) {
    const std::string casePath = joinPath(tempDir, fileName);
    {
      std::ofstream caseFile(casePath);
      caseFile << specText;
    }
    resetGlobalTables();
    bool parsed = false;
    try {
      const LexSpecification parsedSpec = parser.parseLexFile(casePath);
      parsed = !parsedSpec.rules.empty();
    } catch (const std::exception&) {
      parsed = false;
    }
    std::cout << "[parser-test] " << fileName << " => " << (parsed ? "ok" : "failed") << '\n';
    if (!parsed) {
      allPassed = false;
    }
  };

  expectLexParseSuccess("action_delimiter.l", actionDelimiterSpec);
  expectLexParseSuccess("comment_brace.l", commentBraceSpec);

  const std::string generatedMinic = joinPath(tempDir, "generated_minic_lexer.cpp");
  generate(joinPath(repoRoot, "resources/minic.l"),
           generatedMinic,
           joinPath(tempDir, "minic_dot"));
  const std::string generatedC99 = joinPath(tempDir, "generated_c99_lexer.cpp");
  generate(joinPath(repoRoot, "resources/c99.l"),
           generatedC99,
           joinPath(tempDir, "c99_dot"));

  std::cout << "[self-test] generated lexer: " << outPath << '\n';
  std::cout << "[self-test] generated minic lexer: " << generatedMinic << '\n';
  std::cout << "[self-test] generated c99 lexer: " << generatedC99 << '\n';
  return allPassed;
}

}  // namespace seu_lex
