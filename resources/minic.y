/*
 * MiniC-Plus grammar specification
 *
 * The `minic-plus` branch deliberately shrinks the original draft to the
 * subset already exercised by the current executable pipeline.
 */

%token IDENTIFIER CONSTANT
%token INT RETURN IF ELSE WHILE
%token LE_OP GE_OP EQ_OP NE_OP

%start translation_unit

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
%left EQ_OP NE_OP '<' '>' LE_OP GE_OP
%left '+' '-'
%left '*' '/' '%'

%%

translation_unit
    : function_list
    ;

function_list
    : function_definition
    | function_list function_definition
    ;

function_definition
    : INT IDENTIFIER '(' parameter_list_opt ')' compound_statement
    ;

parameter_list_opt
    :
    | parameter_list
    ;

parameter_list
    : parameter_declaration
    | parameter_list ',' parameter_declaration
    ;

parameter_declaration
    : INT IDENTIFIER
    ;

compound_statement
    : '{' declaration_list_opt statement_list_opt '}'
    ;

declaration_list_opt
    :
    | declaration_list
    ;

declaration_list
    : declaration
    | declaration_list declaration
    ;

declaration
    : INT IDENTIFIER ';'
    ;

statement_list_opt
    :
    | statement_list
    ;

statement_list
    : statement
    | statement_list statement
    ;

statement
    : IDENTIFIER '=' expression ';'
    | RETURN expression ';'
    | selection_statement
    | iteration_statement
    | compound_statement
    ;

selection_statement
    : IF '(' expression ')' statement %prec LOWER_THAN_ELSE
    | IF '(' expression ')' statement ELSE statement
    ;

iteration_statement
    : WHILE '(' expression ')' statement
    ;

expression
    : IDENTIFIER
    | CONSTANT
    | '(' expression ')'
    | IDENTIFIER '(' argument_expression_list_opt ')'
    | expression '+' expression
    | expression '-' expression
    | expression '*' expression
    | expression '/' expression
    | expression '%' expression
    | expression '<' expression
    | expression '>' expression
    | expression LE_OP expression
    | expression GE_OP expression
    | expression EQ_OP expression
    | expression NE_OP expression
    ;

argument_expression_list_opt
    :
    | argument_expression_list
    ;

argument_expression_list
    : expression
    | argument_expression_list ',' expression
    ;

%%
