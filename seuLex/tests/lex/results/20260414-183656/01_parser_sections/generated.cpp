#include <array>
#include <cstddef>
#include <iostream>
#include <string>
#include <vector>


static int base_value = 10;


static std::string yytext_storage;
static char* yytext = nullptr;
static std::string yy_source;
static std::size_t yy_cursor = 0;
#ifndef ECHO
#define ECHO do { std::cout << yytext; } while (0)
#endif
#ifdef YY_USER_INIT
#define SEU_LEX_CALL_USER_INIT() do { YY_USER_INIT; } while (false)
#else
#define SEU_LEX_CALL_USER_INIT() do {} while (false)
#endif

int input() {
  if (yy_cursor >= yy_source.size()) {
    return 0;
  }
  return static_cast<unsigned char>(yy_source[yy_cursor++]);
}

static const int kStartState = 0;
static const std::vector<std::array<int, 128>> kTransitions = {
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, 2, -1, 3, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, -1},
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, 1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1},
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, -1, -1, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, 2, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, -1},
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, -1, -1, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, 2, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, -1},
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, -1, -1, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, 2, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 5, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, -1},
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, -1, -1, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, 2, -1, 2, 2, 2, 2, 2, 2, 2, 6, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, -1},
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, -1, -1, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, 2, -1, 7, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, -1},
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, -1, -1, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, 2, -1, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, -1, -1, -1, -1, -1}
};

static const std::vector<int> kAcceptStates = {0, 1, 1, 1, 1, 1, 1, 1};


int helper() { return 1; }

static int dispatch_action(int state) {
  switch (state) {
    case 1:
      return 0;
    case 2:
      do {
        {
  /* } should not break parser */
  return 20;
}
      } while (false);
      return 0;
    case 3:
      do {
        {
  /* } should not break parser */
  return 20;
}
      } while (false);
      return 0;
    case 4:
      do {
        {
  /* } should not break parser */
  return 20;
}
      } while (false);
      return 0;
    case 5:
      do {
        {
  /* } should not break parser */
  return 20;
}
      } while (false);
      return 0;
    case 6:
      do {
        {
  /* } should not break parser */
  return 20;
}
      } while (false);
      return 0;
    case 7:
      do {
        {
  const char* marker = "%%";
  return helper() + base_value;
}
      } while (false);
      return 0;
    default:
      return -1;
  }
}

static void reset_source(const std::string& source) {
  yy_source = source;
  yy_cursor = 0;
  SEU_LEX_CALL_USER_INIT();
}

int analysis(std::string yytext) {
  yytext_storage = yytext;
  ::yytext = yytext_storage.data();
  int state = kStartState;
  for (unsigned char ch : yytext_storage) {
    if (ch >= 128) {
      return -1;
    }
    state = kTransitions[state][ch];
    if (state < 0) {
      return -1;
    }
  }
  return dispatch_action(state);
}

int next_token() {
  if (yy_cursor >= yy_source.size()) {
    return 0;
  }
  const std::size_t start = yy_cursor;
  std::size_t pos = yy_cursor;
  int state = kStartState;
  int last_accept_state = kAcceptStates[state] != 0 ? state : -1;
  std::size_t last_accept_pos = start;
  while (pos < yy_source.size()) {
    const unsigned char ch = static_cast<unsigned char>(yy_source[pos]);
    if (ch >= 128) {
      break;
    }
    const int next = kTransitions[state][ch];
    if (next < 0) {
      break;
    }
    state = next;
    ++pos;
    if (kAcceptStates[state] != 0) {
      last_accept_state = state;
      last_accept_pos = pos;
    }
  }
  if (last_accept_state < 0) {
    yytext_storage = yy_source.substr(start, 1);
    yytext = yytext_storage.data();
    ++yy_cursor;
    return -1;
  }
  yytext_storage = yy_source.substr(start, last_accept_pos - start);
  yytext = yytext_storage.data();
  if (last_accept_pos == start && yy_cursor < yy_source.size()) {
    ++yy_cursor;
  } else {
    yy_cursor = last_accept_pos;
  }
  return dispatch_action(last_accept_state);
}

std::vector<int> tokenize(const std::string& source) {
  reset_source(source);
  std::vector<int> tokens;
  while (yy_cursor < yy_source.size()) {
    const int token = next_token();
    if (token == 0) {
      continue;
    }
    tokens.push_back(token);
  }
  return tokens;
}
