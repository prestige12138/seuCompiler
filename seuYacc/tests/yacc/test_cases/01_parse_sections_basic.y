%{
int shared_counter = 7;
int header_helper() { return shared_counter; }
%}
%token ID NUM
%start translation_unit
%%
translation_unit : external_declaration ;
external_declaration : ID ;
%%
int trailer_helper() { return header_helper(); }
