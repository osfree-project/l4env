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
  const Mword mask 
    = ~((Config::thread_block_size * Kmem::info()->max_threads())-1)
    |   (Config::thread_block_size -1);
  return reinterpret_cast<Thread *>
    (id.is_invalid()
     ? 0 // invalid
     : Kmem::mem_tcbs | ((id.gthread()*Config::thread_block_size) & ~mask));
}

