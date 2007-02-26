INTERFACE:

extern "C" void kern_kdebug_entry(void) __attribute__((long_call));

IMPLEMENTATION:

#include <cstdio>

inline NEEDS [<cstdio>]
bool kdb_ke(const char *msg)
{
  printf("\nKDB: %s\n",msg);
  kern_kdebug_entry();
  return true;
}
