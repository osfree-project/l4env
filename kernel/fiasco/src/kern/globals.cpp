INTERFACE:

#include <cassert>

#include "mem_layout.h"
#include "panic.h"
#include "types.h"

class Context;
class Mem_space;
class Task;
class Thread;
class Timeout;

extern Task *sigma0_task;
extern Mem_space *sigma0_space;
extern Thread *sigma0_thread;
extern Timeout *timeslice_timeout;
extern bool running;
extern unsigned boot_stack;

/* the check macro is like assert(), but it evaluates its argument
   even if NDEBUG is defined */
#ifndef check
#ifdef NDEBUG
# define check(expression) ((void)(expression))
#else /* ! NDEBUG */
# ifdef ASSERT_KDB_KE
#  define check(expression) assert(expression)
# else
#  define check(expression) \
          ((void)((expression) ? 0 : \
                 (panic(__FILE__":%u: failed check `"#expression"'", \
                         __LINE__), 0)))
# endif
#endif /* ! NDEBUG */
#endif /* check */

// nil_thread and kernel_thread might have different values on a SMP system
static Thread * const nil_thread    = 
  reinterpret_cast<Thread*>(Mem_layout::Tcbs);
static Thread * const kernel_thread = 
  reinterpret_cast<Thread*>(Mem_layout::Tcbs);

//---------------------------------------------------------------------------
INTERFACE[utcb]:

extern Address *global_utcb_ptr;

//---------------------------------------------------------------------------
IMPLEMENTATION:

#include "config.h"

Task  *sigma0_task;
Mem_space *sigma0_space;
Thread *sigma0_thread;
Timeout *timeslice_timeout;
bool running = true;
unsigned boot_stack;

inline NEEDS ["config.h"]
Context *context_of(const void *ptr)
{
  return reinterpret_cast<Context *>
    (reinterpret_cast<unsigned long>(ptr) & ~(Config::thread_block_size - 1));
}

//---------------------------------------------------------------------------
IMPLEMENTATION[utcb]:

Address *global_utcb_ptr;

