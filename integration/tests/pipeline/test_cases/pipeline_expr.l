%{
#include "pipeline_tokens.h"
using namespace pipeline_parser_generated;
%}

%%
"return"                  { return RETURN; }
[0-9]+                    { return NUMBER; }
[A-Za-z_][A-Za-z0-9_]*    { return IDENTIFIER; }
"+"                       { return '+'; }
"*"                       { return '*'; }
"="                       { return '='; }
";"                       { return ';'; }
"("                       { return '('; }
")"                       { return ')'; }
[ \t\r\n]+                ;
.                         { return -1; }
%%
