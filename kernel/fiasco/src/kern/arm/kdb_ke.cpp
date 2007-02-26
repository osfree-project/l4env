INTERFACE:

IMPLEMENTATION:

#include <cstdio>
#include "jdb.h"

inline NEEDS [<cstdio>,"jdb.h"]
bool kdb_ke(const char *msg)
{
  printf("\nKDB: %s\n",msg);
  return Jdb::enter_jdb();
}
