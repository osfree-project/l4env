INTERFACE:

#include "l4_types.h"

class Thread;

IMPLEMENTATION:

#include "config.h"
#include "kmem.h"

inline NEEDS["config.h", "kmem.h"]
Thread*
lookup_thread(L4_uid id)
{
  return reinterpret_cast<Thread *>
    (id.is_invalid()
     ? 0 // invalid
     : Kmem::mem_tcbs | ((id.gthread() 
			  * Config::thread_block_size)
			 & ~Config::thread_block_mask));
}

inline NEEDS[lookup_thread]
Thread*
lookup_first_thread(unsigned space)
{
  return lookup_thread( L4_uid( space, 0 ) );
}
