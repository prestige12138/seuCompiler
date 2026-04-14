%{
static int base_value = 10;
%}
ID [A-Za-z_][A-Za-z0-9_]*
%%
"alpha" {
  const char* marker = "%%";
  return helper() + base_value;
}
[ \t\n]+ ;
{ID} {
  /* } should not break parser */
  return 20;
}
%%
int helper() { return 1; }
