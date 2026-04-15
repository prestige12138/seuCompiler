%{
#include "ast_builder.h"
#include "intermediate_code.h"

#include <string>
#include <vector>

static seu_icg::ASTBuilder g_ast_builder;

static std::vector<seu_icg::ASTNode*> take_children(void* raw_node) {
  auto* node = static_cast<seu_icg::ASTNode*>(raw_node);
  if (node == nullptr) {
    return {};
  }
  std::vector<seu_icg::ASTNode*> children = node->children;
  node->children.clear();
  g_ast_builder.destroyTree(node);
  return children;
}

static seu_icg::ASTNode* merge_program_nodes(void* lhs_raw, void* rhs_raw) {
  auto* lhs = static_cast<seu_icg::ASTNode*>(lhs_raw);
  auto* rhs = static_cast<seu_icg::ASTNode*>(rhs_raw);
  if (lhs == nullptr) {
    return rhs;
  }
  if (rhs == nullptr) {
    return lhs;
  }
  for (auto* child : rhs->children) {
    g_ast_builder.appendChild(lhs, child);
  }
  rhs->children.clear();
  g_ast_builder.destroyTree(rhs);
  return lhs;
}
%}

%union {
  void* node;
  int ival;
  const char* str;
}

%token <str> IDENTIFIER
%token <ival> NUMBER
%token INT RETURN IF ELSE WHILE
%token LE GE EQ NE
%type <node> program function_list function parameter_list_opt parameter_list parameter
%type <node> decl_list_opt decl_list declaration stmt_list_opt stmt_list stmt compound_stmt
%type <node> expr arg_list_opt arg_list
%start program

%nonassoc LOWER_THAN_ELSE
%nonassoc ELSE
%left EQ NE '<' '>' LE GE
%left '+' '-'
%left '*' '/' '%'

%%
program
  : function_list
    {
      $$ = $1;
      seu_icg::setParseRoot(static_cast<seu_icg::ASTNode*>($1));
    }
  ;

function_list
  : function_list function
    {
      g_ast_builder.appendChild(static_cast<seu_icg::ASTNode*>($1),
                                static_cast<seu_icg::ASTNode*>($2));
      $$ = $1;
    }
  | function
    {
      $$ = g_ast_builder.makeProgram({static_cast<seu_icg::ASTNode*>($1)});
    }
  ;

function
  : INT IDENTIFIER '(' parameter_list_opt ')' compound_stmt
    {
      $$ = g_ast_builder.makeFunction(
          std::string($2),
          "int",
          take_children($4),
          static_cast<seu_icg::ASTNode*>($6));
    }
  ;

parameter_list_opt
  : parameter_list
    {
      $$ = $1;
    }
  |
    {
      $$ = g_ast_builder.makeProgram({});
    }
  ;

parameter_list
  : parameter_list ',' parameter
    {
      g_ast_builder.appendChild(static_cast<seu_icg::ASTNode*>($1),
                                static_cast<seu_icg::ASTNode*>($3));
      $$ = $1;
    }
  | parameter
    {
      $$ = g_ast_builder.makeProgram({static_cast<seu_icg::ASTNode*>($1)});
    }
  ;

parameter
  : INT IDENTIFIER
    {
      $$ = g_ast_builder.makeVarDecl(std::string($2), "int");
    }
  ;

compound_stmt
  : '{' decl_list_opt stmt_list_opt '}'
    {
      $$ = merge_program_nodes($2, $3);
    }
  ;

decl_list_opt
  : decl_list
    {
      $$ = $1;
    }
  |
    {
      $$ = g_ast_builder.makeProgram({});
    }
  ;

decl_list
  : decl_list declaration
    {
      g_ast_builder.appendChild(static_cast<seu_icg::ASTNode*>($1),
                                static_cast<seu_icg::ASTNode*>($2));
      $$ = $1;
    }
  | declaration
    {
      $$ = g_ast_builder.makeProgram({static_cast<seu_icg::ASTNode*>($1)});
    }
  ;

declaration
  : INT IDENTIFIER ';'
    {
      $$ = g_ast_builder.makeVarDecl(std::string($2), "int");
    }
  ;

stmt_list_opt
  : stmt_list
    {
      $$ = $1;
    }
  |
    {
      $$ = g_ast_builder.makeProgram({});
    }
  ;

stmt_list
  : stmt_list stmt
    {
      g_ast_builder.appendChild(static_cast<seu_icg::ASTNode*>($1),
                                static_cast<seu_icg::ASTNode*>($2));
      $$ = $1;
    }
  | stmt
    {
      $$ = g_ast_builder.makeProgram({static_cast<seu_icg::ASTNode*>($1)});
    }
  ;

stmt
  : IDENTIFIER '=' expr ';'
    {
      $$ = g_ast_builder.makeAssignment(
          g_ast_builder.makeIdentifier(std::string($1), "int"),
          static_cast<seu_icg::ASTNode*>($3));
    }
  | RETURN expr ';'
    {
      $$ = g_ast_builder.makeReturn(static_cast<seu_icg::ASTNode*>($2));
    }
  | IF '(' expr ')' stmt %prec LOWER_THAN_ELSE
    {
      $$ = g_ast_builder.makeIf(static_cast<seu_icg::ASTNode*>($3),
                                static_cast<seu_icg::ASTNode*>($5));
    }
  | IF '(' expr ')' stmt ELSE stmt
    {
      $$ = g_ast_builder.makeIf(static_cast<seu_icg::ASTNode*>($3),
                                static_cast<seu_icg::ASTNode*>($5),
                                static_cast<seu_icg::ASTNode*>($7));
    }
  | WHILE '(' expr ')' stmt
    {
      $$ = g_ast_builder.makeWhile(static_cast<seu_icg::ASTNode*>($3),
                                   static_cast<seu_icg::ASTNode*>($5));
    }
  | compound_stmt
    {
      $$ = $1;
    }
  ;

expr
  : expr '+' expr
    {
      $$ = g_ast_builder.makeBinary(seu_icg::NODE_ARITH,
                                    static_cast<seu_icg::ASTNode*>($1),
                                    static_cast<seu_icg::ASTNode*>($3),
                                    "+",
                                    "int");
    }
  | expr '-' expr
    {
      $$ = g_ast_builder.makeBinary(seu_icg::NODE_ARITH,
                                    static_cast<seu_icg::ASTNode*>($1),
                                    static_cast<seu_icg::ASTNode*>($3),
                                    "-",
                                    "int");
    }
  | expr '*' expr
    {
      $$ = g_ast_builder.makeBinary(seu_icg::NODE_ARITH,
                                    static_cast<seu_icg::ASTNode*>($1),
                                    static_cast<seu_icg::ASTNode*>($3),
                                    "*",
                                    "int");
    }
  | expr '/' expr
    {
      $$ = g_ast_builder.makeBinary(seu_icg::NODE_ARITH,
                                    static_cast<seu_icg::ASTNode*>($1),
                                    static_cast<seu_icg::ASTNode*>($3),
                                    "/",
                                    "int");
    }
  | expr '%' expr
    {
      $$ = g_ast_builder.makeBinary(seu_icg::NODE_ARITH,
                                    static_cast<seu_icg::ASTNode*>($1),
                                    static_cast<seu_icg::ASTNode*>($3),
                                    "%",
                                    "int");
    }
  | expr '<' expr
    {
      $$ = g_ast_builder.makeBinary(seu_icg::NODE_ARITH,
                                    static_cast<seu_icg::ASTNode*>($1),
                                    static_cast<seu_icg::ASTNode*>($3),
                                    "<",
                                    "int");
    }
  | expr '>' expr
    {
      $$ = g_ast_builder.makeBinary(seu_icg::NODE_ARITH,
                                    static_cast<seu_icg::ASTNode*>($1),
                                    static_cast<seu_icg::ASTNode*>($3),
                                    ">",
                                    "int");
    }
  | expr LE expr
    {
      $$ = g_ast_builder.makeBinary(seu_icg::NODE_ARITH,
                                    static_cast<seu_icg::ASTNode*>($1),
                                    static_cast<seu_icg::ASTNode*>($3),
                                    "<=",
                                    "int");
    }
  | expr GE expr
    {
      $$ = g_ast_builder.makeBinary(seu_icg::NODE_ARITH,
                                    static_cast<seu_icg::ASTNode*>($1),
                                    static_cast<seu_icg::ASTNode*>($3),
                                    ">=",
                                    "int");
    }
  | expr EQ expr
    {
      $$ = g_ast_builder.makeBinary(seu_icg::NODE_ARITH,
                                    static_cast<seu_icg::ASTNode*>($1),
                                    static_cast<seu_icg::ASTNode*>($3),
                                    "==",
                                    "int");
    }
  | expr NE expr
    {
      $$ = g_ast_builder.makeBinary(seu_icg::NODE_ARITH,
                                    static_cast<seu_icg::ASTNode*>($1),
                                    static_cast<seu_icg::ASTNode*>($3),
                                    "!=",
                                    "int");
    }
  | IDENTIFIER '(' arg_list_opt ')'
    {
      $$ = g_ast_builder.makeCall(std::string($1), take_children($3), "int");
    }
  | IDENTIFIER
    {
      $$ = g_ast_builder.makeIdentifier(std::string($1), "int");
    }
  | NUMBER
    {
      $$ = g_ast_builder.makeConstant(std::to_string($1), "int");
    }
  | '(' expr ')'
    {
      $$ = $2;
    }
  ;

arg_list_opt
  : arg_list
    {
      $$ = $1;
    }
  |
    {
      $$ = g_ast_builder.makeProgram({});
    }
  ;

arg_list
  : arg_list ',' expr
    {
      g_ast_builder.appendChild(static_cast<seu_icg::ASTNode*>($1),
                                static_cast<seu_icg::ASTNode*>($3));
      $$ = $1;
    }
  | expr
    {
      $$ = g_ast_builder.makeProgram({static_cast<seu_icg::ASTNode*>($1)});
    }
  ;
%%
