#pragma once

#include <vector>

#include "lr1_pda.h"

/**
 * @file lalr_converter.h
 * @brief 定义 LR(1) 自动机向 LALR(1) 自动机压缩时所需的状态合并接口。
 */

namespace seu_yacc {

/**
 * @brief 保存 LR(1) 自动机压缩为 LALR(1) 自动机后的结果。
 */
struct LALRResult {
  LRPDA automaton;
  std::vector<int> old_to_new;
};

/**
 * @brief 负责将 LR(0) 核相同的 LR(1) 状态合并为 LALR(1) 状态。
 */
class LALRConverter {
 public:
  /**
   * @brief 将规范 LR(1) 下推自动机转换为 LALR(1) 下推自动机。
   *
   * 时间复杂度：O(S * I log I)，其中 S 为状态数，I 为项目数。
   */
  LALRResult convert(const LRPDA& canonical) const;
};

}  // 命名空间 seu_yacc
