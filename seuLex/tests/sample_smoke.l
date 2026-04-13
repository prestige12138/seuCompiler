%{
#include <string>
%}

DIGIT [0-9]
ALPHA [A-Za-z_]

%%
"if" { return 1; }
"else" { return 2; }
{ALPHA}({ALPHA}|{DIGIT})* { return 3; }
{DIGIT}+ { return 4; }
[ \t\n]+ ;
%%

int yywrap() { return 1; }
