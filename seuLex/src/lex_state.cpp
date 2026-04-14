#include "nfa_constructor.h"

#include <memory>
#include <vector>

#include "internal/lex_state.h"

namespace seu_lex {

std::set<char> char_set;
std::map<std::string, std::string> idreTable;
std::vector<nfa> nfaTable;
std::vector<node*> dfaterminals;
std::map<int, std::string> nfaterstatetoaction;
std::map<int, std::string> TerStateActionTable;
std::map<int, std::string> mindfareturn;
std::map<int, std::size_t> nfaPriorityTableInternal;

namespace {

int g_nextStateLabel = 1;
std::vector<std::unique_ptr<node>> g_nodeArena;

}  // namespace

node* createState(bool accepted) {
  g_nodeArena.push_back(std::make_unique<node>(g_nextStateLabel++, accepted));
  return g_nodeArena.back().get();
}

void resetGlobalTables() {
  char_set.clear();
  idreTable.clear();
  nfaTable.clear();
  dfaterminals.clear();
  nfaterstatetoaction.clear();
  TerStateActionTable.clear();
  mindfareturn.clear();
  nfaPriorityTableInternal.clear();
  g_nodeArena.clear();
  g_nextStateLabel = 1;
}

}  // namespace seu_lex
