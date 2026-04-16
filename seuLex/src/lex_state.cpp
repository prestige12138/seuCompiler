/**
 * @file lex_state.cpp
 * @brief 实现 seuLex 生成流程中共享的全局构造状态。
 */

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

}  // 匿名命名空间

/**
 * @brief 从共享状态池中分配一个新的自动机结点。
 */
node* createState(bool accepted) {
  g_nodeArena.push_back(std::make_unique<node>(g_nextStateLabel++, accepted));
  return g_nodeArena.back().get();
}

/**
 * @brief 清空一次生成过程中积累的所有全局表和临时状态。
 */
void resetGlobalTables() {
  // 每次重新生成都从空状态池开始，避免状态编号、规则表和中间自动机
  // 在不同测试或命令行调用之间互相污染。
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

}  // 命名空间 seu_lex
