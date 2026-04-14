%{
int seeded_value = 41;
%}
%union {
  int ival;
}
%token <ival> NUM
%type <ival> expr
%left '+'
%right '^'
%start expr
%%
expr : expr '+' { /* midrule action */ int hold = 0; hold += 1; } expr
         { $$ = $1 + $4 + seeded_value - seeded_value; }
     | expr '^' expr
         { $$ = $1; }
     | NUM
         { $$ = $1; }
     ;
%%
int union_helper() { return seeded_value; }
