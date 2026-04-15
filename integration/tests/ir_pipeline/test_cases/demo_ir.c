int inc(int v)
{
  int t;

  t = v + 1;
  return t;
}

int main()
{
  int x;
  int y;
  int limit;
  int result;

  x = 1;
  y = 2;
  limit = 20;
  result = 0;

  while (x < 8)
  {
    y = inc(y);
    result = x + y * 2;

    if (result > limit)
    {
      x = result - 3;
    }
    else
    {
      x = result + 1;
    }
  }

  return x;
}
