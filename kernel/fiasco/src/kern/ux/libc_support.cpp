/*
 * Fiasco-UX
 * Generic printf-like kern-stack-safe output functionality
 */

IMPLEMENTATION:

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>
#include "config.h"
#include "cpu_lock.h"
#include "globals.h"
#include "kmem.h"
#include "lock_guard.h"

/*
 * Generic Fiasco-UX output function.
 * Because kernel stacks are too small for glibc's varargs we have to
 * temporarily switch to the Linux process stack and then back
 */
extern "C"
int
printf (const char *format, ...)
{
  // Caution! No stack variables because we're changing the stack pointer.
  static int ret;
  static unsigned esp;
  static va_list pvar;
  static const char *fmt;

  unsigned curr_esp;

  // use a local variable for this first test, several threads
  // might contend
  asm volatile ("movl %%esp, %0":"=m"(curr_esp));

  // If we're running on a kernel stack then switch to Linux stack
  if (Kmem::mem_tcbs <= curr_esp &&
      curr_esp < Kmem::mem_tcbs + (Config::thread_block_size << 18)) {

    Lock_guard<Cpu_lock> guard (&cpu_lock);

    // globally visible variables (esp, pvar, fmt) must be protected
    // by the cpu_lock when used by non-signal stack activities

    esp = curr_esp;
    va_start (pvar, format);
    fmt = format;

    asm volatile ("movl %%esp, %0; movl %1, %%esp"
                  : "=m" (esp)
                  :  "m" (boot_stack));

    // no local variables, we run on a different stack
    ret = vprintf (fmt, pvar);

    // and now switch back
    asm volatile ("movl %0, %%esp" : : "m" (esp));

    va_end (pvar);

  } else {

    va_start (pvar, format);
    ret = vprintf (format, pvar);
    va_end (pvar);
  }

  return ret;
}

/*
 * Our own version of the assertion failure output function according
 * to Linux Standard Base Specification.
 * We need it since the standard glibc function calls printf, and doing
 * that on a Fiasco kernel stack blows everything up.     
 */
extern "C"
void
__assert_fail (const char *assertion, const char *file,
               unsigned int line, const char *function) {

  printf ("ASSERTION_FAILED (%s) in function %s in file %s:%d\n",
          assertion, function, file, line);
  
  _exit (EXIT_FAILURE);         // Fatal! No destructors
}
