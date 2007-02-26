/*
 * Fiasco-UX
 * Generic printf-like kern-stack-safe output functionality
 */

INTERFACE:

#include <cstdio>

IMPLEMENTATION:

#include <cstdarg>
#include <cstdlib>
#include <unistd.h>
#include "config.h"
#include "cpu_lock.h"
#include "globals.h"
#include "kmem.h"
#include "lock_guard.h"

#define xprintf_macro(stream) 						\
	unsigned esp;							\
	static va_list va;						\
	static unsigned save_esp;					\
	static const char *fmt = format;				\
									\
	asm volatile ("movl %%esp, %0" : "=m" (esp));			\
									\
	if (esp >= Kmem::mem_tcbs &&					\
	    esp <  Kmem::mem_tcbs + (Config::thread_block_size << 18)) {\
									\
	  Lock_guard <Cpu_lock> guard (&cpu_lock);			\
									\
	  va_start (va, format);					\
	  fmt = format;							\
									\
	  asm volatile ("movl %%esp, %0; movl %1, %%esp" 		\
	                : "=m" (save_esp) : "m" (boot_stack));		\
	  vfprintf (stream, fmt, va);					\
	  asm volatile ("movl %0, %%esp" : : "m" (save_esp));		\
									\
	  va_end (va);							\
									\
	} else {							\
									\
	  /*								\
	   * No locking while on the signal stack. We are protected by	\
	   * the fact that all signals are disabled in a signal context.\
	   * Also invoking cli from here leads to endless recursion	\
	   * with printf's in segv_handler.				\
	   */								\
									\
	  va_start (va, format);					\
	  vfprintf (stream, format, va);				\
	  va_end (va);							\
	}								\
									\
	return 0;

extern "C"
int
printf (const char *format, ...)
{
  xprintf_macro(stdout);
}

extern "C"
int
fprintf (FILE *stream, const char *format, ...)
{
  xprintf_macro(stream);
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
