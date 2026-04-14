#pragma once

#include <string>
#include <vector>

namespace generated_parser_generated {

struct YYSTYPE {
  std::string lexeme;
};

enum TokenKind {
  YYEOF_TOKEN = 0,
  IDENTIFIER = 256,
  CONSTANT = 257,
  STRING_LITERAL = 258,
  SIZEOF = 259,
  PTR_OP = 260,
  INC_OP = 261,
  DEC_OP = 262,
  LEFT_OP = 263,
  RIGHT_OP = 264,
  LE_OP = 265,
  GE_OP = 266,
  EQ_OP = 267,
  NE_OP = 268,
  AND_OP = 269,
  OR_OP = 270,
  MUL_ASSIGN = 271,
  DIV_ASSIGN = 272,
  MOD_ASSIGN = 273,
  ADD_ASSIGN = 274,
  SUB_ASSIGN = 275,
  LEFT_ASSIGN = 276,
  RIGHT_ASSIGN = 277,
  AND_ASSIGN = 278,
  XOR_ASSIGN = 279,
  OR_ASSIGN = 280,
  TYPE_NAME = 281,
  TYPEDEF = 282,
  EXTERN = 283,
  STATIC = 284,
  AUTO = 285,
  REGISTER = 286,
  INLINE = 287,
  RESTRICT = 288,
  CHAR = 289,
  SHORT = 290,
  INT = 291,
  LONG = 292,
  SIGNED = 293,
  UNSIGNED = 294,
  FLOAT = 295,
  DOUBLE = 296,
  CONST = 297,
  VOLATILE = 298,
  VOID = 299,
  BOOL = 300,
  COMPLEX = 301,
  IMAGINARY = 302,
  STRUCT = 303,
  UNION = 304,
  ENUM = 305,
  ELLIPSIS = 306,
  CASE = 307,
  DEFAULT = 308,
  IF = 309,
  ELSE = 310,
  SWITCH = 311,
  WHILE = 312,
  DO = 313,
  FOR = 314,
  GOTO = 315,
  CONTINUE = 316,
  BREAK = 317,
  RETURN = 318,
};

struct Token {
  int type = 0;
  std::string lexeme;
  int line = 0;
  int column = 0;
  YYSTYPE semantic{};
};

bool yyparse(const std::vector<Token>& tokens);

}  // namespace generated_parser_generated
