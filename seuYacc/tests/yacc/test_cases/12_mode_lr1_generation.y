%token ID
%token ASSIGN
%start S
%%
S : assignment ;
assignment : ID ASSIGN ID ;
%%
