/*
 * MiniC-Plus grammar specification
 * Phase 1 initialization only.
 *
 * This file defines the intended grammar boundary for the `minic-plus`
 * branch. Semantic actions and generator/runtime integration are deferred
 * to later stages.
 */

%token IDENTIFIER CONSTANT
%token INT VOID
%token IF ELSE WHILE FOR BREAK CONTINUE RETURN
%token LE_OP GE_OP EQ_OP NE_OP AND_OP OR_OP

%start translation_unit

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
%left OR_OP
%left AND_OP
%left EQ_OP NE_OP
%left '<' '>' LE_OP GE_OP
%left '+' '-'
%left '*' '/' '%'
%right '!'
%right UMINUS

%%

translation_unit
    : external_list
    ;

external_list
    : external
    | external_list external
    ;

external
    : function_definition
    | global_declaration
    ;

type_specifier
    : INT
    | VOID
    ;

global_declaration
    : INT init_declarator_list ';'
    ;

function_definition
    : type_specifier IDENTIFIER '(' parameter_list_opt ')' compound_statement
    ;

parameter_list_opt
    :
    | VOID
    | parameter_list
    ;

parameter_list
    : parameter_declaration
    | parameter_list ',' parameter_declaration
    ;

parameter_declaration
    : INT IDENTIFIER
    ;

init_declarator_list
    : init_declarator
    | init_declarator_list ',' init_declarator
    ;

init_declarator
    : IDENTIFIER
    | IDENTIFIER '=' expression
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
    : INT init_declarator_list ';'
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
    : assignment_statement
    | expression_statement
    | selection_statement
    | iteration_statement
    | jump_statement
    | compound_statement
    ;

assignment_statement
    : IDENTIFIER '=' expression ';'
    ;

expression_statement
    : ';'
    | expression ';'
    ;

selection_statement
    : IF '(' expression ')' statement %prec LOWER_THAN_ELSE
    | IF '(' expression ')' statement ELSE statement
    ;

iteration_statement
    : WHILE '(' expression ')' statement
    | FOR '(' for_init_opt ';' expression_opt ';' for_step_opt ')' statement
    ;

for_init_opt
    :
    | assignment_expression
    | for_declaration
    ;

for_declaration
    : INT init_declarator_list
    ;

for_step_opt
    :
    | assignment_expression
    ;

expression_opt
    :
    | expression
    ;

jump_statement
    : RETURN ';'
    | RETURN expression ';'
    | BREAK ';'
    | CONTINUE ';'
    ;

assignment_expression
    : IDENTIFIER '=' expression
    ;

expression
    : logical_or_expression
    ;

logical_or_expression
    : logical_and_expression
    | logical_or_expression OR_OP logical_and_expression
    ;

logical_and_expression
    : equality_expression
    | logical_and_expression AND_OP equality_expression
    ;

equality_expression
    : relational_expression
    | equality_expression EQ_OP relational_expression
    | equality_expression NE_OP relational_expression
    ;

relational_expression
    : additive_expression
    | relational_expression '<' additive_expression
    | relational_expression '>' additive_expression
    | relational_expression LE_OP additive_expression
    | relational_expression GE_OP additive_expression
    ;

additive_expression
    : multiplicative_expression
    | additive_expression '+' multiplicative_expression
    | additive_expression '-' multiplicative_expression
    ;

multiplicative_expression
    : unary_expression
    | multiplicative_expression '*' unary_expression
    | multiplicative_expression '/' unary_expression
    | multiplicative_expression '%' unary_expression
    ;

unary_expression
    : postfix_expression
    | '-' unary_expression %prec UMINUS
    | '!' unary_expression
    ;

postfix_expression
    : primary_expression
    | IDENTIFIER '(' argument_expression_list_opt ')'
    ;

argument_expression_list_opt
    :
    | argument_expression_list
    ;

argument_expression_list
    : expression
    | argument_expression_list ',' expression
    ;

primary_expression
    : IDENTIFIER
    | CONSTANT
    | '(' expression ')'
    ;

%%
