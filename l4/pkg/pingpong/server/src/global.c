
#include "global.h"

void*
memchr(const void *s, int c, unsigned n) 
{
  register const char* t=s;
  int i;
  for (i=n; i; --i)
    {
      if (*t==c)
	return (char*)t;
      ++t;
    }
  return 0;
}

