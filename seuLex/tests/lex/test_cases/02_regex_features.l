DIGIT [0-9]
%%
"if" { return 1; }
{DIGIT}+ { return 2; }
[a-cx-z] { return 3; }
e? { return 6; }
f+ { return 7; }
g{2} { return 8; }
h{2,4} { return 9; }
i{2,} { return 10; }
. { return 5; }
%%
