%token X
%start S
%%
S : A
  | B
  ;
A : X ;
B : X ;
%%
