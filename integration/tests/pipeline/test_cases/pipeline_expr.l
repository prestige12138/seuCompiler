%{
#define SEU_LEX_FINALIZE_PARSER_TOKEN(token_ref, lex_token_ref) \
  do { \
    (void)(lex_token_ref); \
    if ((token_ref).type == SEU_YACC_TOKEN_NAMESPACE::IDENTIFIER) { \
      (token_ref).semantic.str = (token_ref).lexeme.c_str(); \
    } \
  } while (false)
%}

%%
"return"                  { return SEU_YACC_TOKEN_NAMESPACE::RETURN; }
"answer"                  { yylval.ival = 42; return SEU_YACC_TOKEN_NAMESPACE::MAGIC; }
[0-9]+                    { yylval.ival = std::stoi(yytext); return SEU_YACC_TOKEN_NAMESPACE::NUMBER; }
[A-Za-z_][A-Za-z0-9_]*    { return SEU_YACC_TOKEN_NAMESPACE::IDENTIFIER; }
"+"                       { return '+'; }
"*"                       { return '*'; }
"="                       { return '='; }
";"                       { return ';'; }
"("                       { return '('; }
")"                       { return ')'; }
[ \t\r\n]+                ;
.                         { return -1; }
%%
