/*
 * MiniC-Plus lexical specification
 *
 * The `minic-plus` branch keeps only the stable executable subset already
 * supported by the current Lex -> Yacc -> AST -> IR chain.
 */

D           [0-9]
L           [A-Za-z_]
ALNUM       [A-Za-z0-9_]

%{
int yywrap(void) { return 1; }
void comment(void);
%}

%%
"/*"                    { comment(); }
"//"[^\n]*              { /* skip line comment */ }

"int"                   { return(INT); }
"if"                    { return(IF); }
"else"                  { return(ELSE); }
"while"                 { return(WHILE); }
"return"                { return(RETURN); }

"<="                    { return(LE_OP); }
">="                    { return(GE_OP); }
"=="                    { return(EQ_OP); }
"!="                    { return(NE_OP); }

{L}{ALNUM}*             { return(IDENTIFIER); }
{D}+                    { return(CONSTANT); }

";"                     { return(';'); }
","                     { return(','); }
"("                     { return('('); }
")"                     { return(')'); }
"{"                     { return('{'); }
"}"                     { return('}'); }
"="                     { return('='); }
"+"                     { return('+'); }
"-"                     { return('-'); }
"*"                     { return('*'); }
"/"                     { return('/'); }
"%"                     { return('%'); }
"<"                     { return('<'); }
">"                     { return('>'); }

[ \t\r\n\f\v]+          { /* skip whitespace */ }
.                       { return(-1); }
%%

void comment(void)
{
    int c;
    int prev = 0;
    while ((c = input()) != 0)
    {
        if (prev == '*' && c == '/')
        {
            return;
        }
        prev = c;
    }
}
