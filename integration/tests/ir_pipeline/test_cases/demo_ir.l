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
"int"                     { return SEU_YACC_TOKEN_NAMESPACE::INT; }
"return"                  { return SEU_YACC_TOKEN_NAMESPACE::RETURN; }
"if"                      { return SEU_YACC_TOKEN_NAMESPACE::IF; }
"else"                    { return SEU_YACC_TOKEN_NAMESPACE::ELSE; }
"while"                   { return SEU_YACC_TOKEN_NAMESPACE::WHILE; }
"<="                      { return SEU_YACC_TOKEN_NAMESPACE::LE; }
">="                      { return SEU_YACC_TOKEN_NAMESPACE::GE; }
"=="                      { return SEU_YACC_TOKEN_NAMESPACE::EQ; }
"!="                      { return SEU_YACC_TOKEN_NAMESPACE::NE; }
[0-9]+                    { yylval.ival = std::stoi(yytext); return SEU_YACC_TOKEN_NAMESPACE::NUMBER; }
[A-Za-z_][A-Za-z0-9_]*    { return SEU_YACC_TOKEN_NAMESPACE::IDENTIFIER; }
"+"                       { return '+'; }
"-"                       { return '-'; }
"*"                       { return '*'; }
"/"                       { return '/'; }
"%"                       { return '%'; }
"="                       { return '='; }
";"                       { return ';'; }
","                       { return ','; }
"("                       { return '('; }
")"                       { return ')'; }
"{"                       { return '{'; }
"}"                       { return '}'; }
"<"                       { return '<'; }
">"                       { return '>'; }
[ \t\r\n]+                ;
.                         { return -1; }
%%
