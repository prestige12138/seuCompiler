%{
int semantic_total = 0;
%}
%union {
  int ival;
}
%token <ival> NUM
%type <ival> S E
%left '+'
%start S
%%
S : E { semantic_total = $<ival>1; $<ival>$ = $<ival>1; } ;
E : NUM { $<ival>$ = $<ival>1; }
  | E '+' NUM { $<ival>$ = $<ival>1 + $<ival>3; }
  ;
%%
int read_semantic_total() { return semantic_total; }
