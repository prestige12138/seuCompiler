%token ID
%left '+'
%left '*'
%start S
%%
S : E ;
E : E '+' E
  | E '*' E
  | '(' E ')'
  | ID
  ;
%%
