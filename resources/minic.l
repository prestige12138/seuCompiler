/*
 * MiniC-Plus lexical specification
 * Phase 1 initialization only.
 *
 * This file is a resource scaffold for the `minic-plus` branch.
 * It is intentionally not wired into the current build/runtime yet.
 */

D           [0-9]
L           [A-Za-z_]
ALNUM       [A-Za-z0-9_]

%{
int yywrap(void) { return 1; }
%}

%%
"/*"                    { comment(); }
"//"[^\n]*              { /* skip line comment */ }

"int"                   { return(INT); }
"void"                  { return(VOID); }
"if"                    { return(IF); }
"else"                  { return(ELSE); }
"while"                 { return(WHILE); }
"for"                   { return(FOR); }
"break"                 { return(BREAK); }
"continue"              { return(CONTINUE); }
"return"                { return(RETURN); }

"&&"                    { return(AND_OP); }
"||"                    { return(OR_OP); }
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
"!"                     { return('!'); }
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
