DIGIT [0-9]
ID [A-Za-z_][A-Za-z0-9_]*
%%
"if" { return 1; }
"==" { return 2; }
"=" { return 3; }
{ID} { return 4; }
{DIGIT}+ { return 5; }
[ \t\n]+ ;
. { return -1; }
%%
