#pragma once

#include <string>

/**
 * @file regex_expander.h
 * @brief Extended Lex regular-expression expansion interface.
 */

namespace seu_lex {

/**
 * @brief Expand extended Lex regular expressions into ordinary RE tokens.
 */
class REExpander {
 public:
  /**
   * @brief Expand one extended RE using `idreTable`.
   *
   * Complexity: O(M + K), where M is input length and K is normalized output.
   */
  std::string expandRE(const std::string& raw) const;
};

}  // namespace seu_lex
