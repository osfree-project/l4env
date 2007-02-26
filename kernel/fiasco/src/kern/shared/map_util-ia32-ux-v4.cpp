INTERFACE:

//-

IMPLEMENTATION[ia32-ux-v4]:

#include "kdb_ke.h"

inline NEEDS ["config.h","kdb_ke.h"]
L4_msgdope fpage_map(Space *from, L4_fpage fp_from, 
		     Space *to, L4_fpage fp_to, 
		     Address offs)
{
  // v4 specific implementation of fpage_map() using mem_map() and io_map()
  // will be placed here

  kdb_ke ("v4 fpage_map called");
  
  L4_msgdope result(0);
  return result;
}

