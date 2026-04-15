1: t1 = v + 1
2: t = t1
3: return t
4: x = 1
5: y = 2
6: limit = 20
7: result = 0
8: if x < 8 goto 10
9: goto 23
10: t2 = call inc(y)
11: y = t2
12: t3 = y * 2
13: t4 = x + t3
14: result = t4
15: if result > limit goto 17
16: goto 20
17: t5 = result - 3
18: x = t5
19: goto 22
20: t6 = result + 1
21: x = t6
22: goto 8
23: return x
