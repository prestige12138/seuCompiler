#pragma once

#include <map>
#include <set>
#include <string>
#include <vector>

#include "node.h"

/**
 * @file nfa.h
 * @brief 中期报告规定的 NFA 结构与共享 NFA 阶段表。
 */

namespace seu_lex {

/**
 * @brief 中期报告规定的 NFA 聚合结构。
 */
typedef struct nfa {
  node* start;
  std::vector<node*> terminal;
} nfa;

/**
 * @brief 非确定有限自动机构造阶段收集到的输入字符集合。
 *
 * 该集合在 Thompson 片段构造时填充，后续由 DFA 确定化和最小化重复使用。
 */
extern std::set<char> char_set;

/**
 * @brief 词法定义段中的命名正规定义表。
 */
extern std::map<std::string, std::string> idreTable;

/**
 * @brief 每条 Lex 规则对应的一个 NFA 片段表，直到全局重置前一直保留。
 */
extern std::vector<nfa> nfaTable;

/**
 * @brief 汤普森构造阶段生成的终态到动作代码映射表。
 */
extern std::map<int, std::string> nfaterstatetoaction;

}  // 命名空间 seu_lex
