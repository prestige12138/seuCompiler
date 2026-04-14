%token NUM
%nonassoc '<'
%start S
%%
S : E ;
E : E '<' E
  | NUM
  ;
%%
