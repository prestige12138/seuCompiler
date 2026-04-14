%{
#include "ast_builder.h"
#include "intermediate_code.h"

#include <string>

static seu_icg::ASTBuilder g_ast_builder;
%}

%union {
  void* node;
  int ival;
  const char* str;
}

%token <ival> NUMBER
%token <ival> MAGIC
%token <str> IDENTIFIER
%token RETURN
%type <node> program stmt_list stmt expr
%start program

%left '+'
%left '*'

%%
program
  : stmt_list
    {
      $$ = $1;
      seu_icg::setParseRoot(static_cast<seu_icg::ASTNode*>($1));
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
  | expr '*' expr
    {
      $$ = g_ast_builder.makeBinary(seu_icg::NODE_ARITH,
                                    static_cast<seu_icg::ASTNode*>($1),
                                    static_cast<seu_icg::ASTNode*>($3),
                                    "*",
                                    "int");
    }
  | IDENTIFIER
    {
      $$ = g_ast_builder.makeIdentifier(std::string($1), "int");
    }
  | NUMBER
    {
      $$ = g_ast_builder.makeConstant(std::to_string($1), "int");
    }
  | MAGIC
    {
      $$ = g_ast_builder.makeConstant(std::to_string($1), "int");
    }
  | '(' expr ')'
    {
      $$ = $2;
    }
  ;
%%
