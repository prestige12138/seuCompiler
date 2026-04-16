/**
 * @file lex_state.h
 * @brief 词法分析模块实现文件共享的状态分配辅助接口与全局表声明。
 */

#pragma once

#include <cstddef>
#include <map>

#include "node.h"

namespace seu_lex {

/**
 * @brief 非确定有限自动机转确定有限自动机阶段使用的内部规则优先级表。
 *
 * 该符号只应在实现层内部使用，不应暴露到公共头文件中。
 */
extern std::map<int, std::size_t> nfaPriorityTableInternal;

/**
 * @brief 从共享构造池中分配一个 NFA 状态结点。
 *
 * 返回指针在调用 `resetGlobalTables()` 之前始终有效。
 */
node* createState(bool accepted = false);

}  // 命名空间 seu_lex
