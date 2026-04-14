%{
int assoc_value = 0;
%}
%union {
  int ival;
}
%token <ival> NUM
%type <ival> S E
%left '-'
%start S
%%
S : E { assoc_value = $1; $$ = $1; } ;
E : E '-' E { $$ = $1 - $3; }
  | NUM { $$ = $1; }
  ;
%%
int get_assoc_value() { return assoc_value; }
