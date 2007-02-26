
#include <cstdio>

int
putstr(const char *const s)
{
  return printf("%s", s);
}

int
putnstr(const char *const s, int len)
{
  return printf("%*.*s", len, len, s);
}

