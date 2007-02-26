INTERFACE [arm]:

extern "C" void kern_kdebug_entry( char const *error ) 
__attribute__((long_call));

IMPLEMENTATION [arm]:

#include <cstdio>

inline NEEDS [<cstdio>]
bool kdb_ke(const char *msg)
{
  kern_kdebug_entry(msg);
  return true;
}

