INTERFACE:

#include "space_index.h"

class Space_index_util
{
};

IMPLEMENTATION:

#include "lock_guard.h"
#include "cpu_lock.h"
#include "space.h"

PUBLIC static inline NEEDS["space.h", "lock_guard.h", "cpu_lock.h"]
Space_index 
Space_index_util::chief(Space_index task)
{
  Lock_guard<Cpu_lock> guard (&cpu_lock);

  Space* s = task.lookup();

  if (s)
    return s->chief();

  return task.chief();
}
