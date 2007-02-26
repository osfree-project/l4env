INTERFACE:

#include <cassert>		// panic()

#include "types.h"

class Context;
class Thread;
class Space;


extern Thread *sigma0_thread, *kernel_thread, *nil_thread;
extern Space *sigma0;
extern bool running;
extern unsigned boot_stack;

// Helpers

/* the check macro is like assert(), but it evaluates its argument
   even if NDEBUG is defined */
#ifndef check
#ifdef NDEBUG
# define check(expression) ((void)(expression))
#else /* ! NDEBUG */
# define check(expression) \
         ((void)((expression) ? 0 : \
                (panic(__FILE__":%u: failed check `"#expression"'", \
                        __LINE__), 0)))
#endif /* ! NDEBUG */
#endif /* check */

IMPLEMENTATION:

// 
// Global variables
// 

Thread *sigma0_thread, *kernel_thread, *nil_thread;
Space *sigma0;
bool running = true;
unsigned boot_stack;
// 
// Helpers
// 

#include "config.h"

inline NEEDS ["config.h"]
Context *context_of(const void *ptr)
{
  return reinterpret_cast<Context *>
    (reinterpret_cast<unsigned long>(ptr) & ~(Config::thread_block_size - 1));
}
