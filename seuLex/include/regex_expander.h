#pragma once

#include <string>

/**
 * @file regex_expander.h
 * @brief 扩展 Lex 正则表达式展开接口。
 */

namespace seu_lex {

/**
 * @brief 将扩展 Lex 正则表达式展开为普通 RE 记号流。
 */
class REExpander {
 public:
  /**
   * @brief 借助 `idreTable` 展开一条扩展 RE。
   *
   * 复杂度：O(M + K)，其中 M 为输入长度，K 为规范化输出长度。
   */
  std::string expandRE(const std::string& raw) const;
};

}  // 命名空间 seu_lex
