/*
 * Fiasco-UX
 * Generic printf-like kern-stack-safe output functionality
 */

INTERFACE:

#include <cstdarg>
#include <cstdio>

void register_libc_atexit(void (*f)(void));


IMPLEMENTATION:

#include <cstdlib>
#include <unistd.h>
#include "config.h"
#include "console.h"
#include "cpu_lock.h"
#include "globals.h"
#include "kernel_console.h"
#include "kmem.h"
#include "lock_guard.h"
#include "panic.h"
#include "static_init.h"


class Stdout_console : public Console
{
private:
  static int _active;
};

static void (*libc_atexit)(void);

// see linker flags in  Makerules.KERNEL.ux
extern "C" int __real_vsnprintf(char *str, size_t size, 
				const char *format, va_list ap);
extern "C" int __real_puts(const char *s);
extern "C" int __real_putchar(int c);


#define GET_ESP								\
  ({ unsigned esp; asm volatile ("movl %%esp, %0" : "=r" (esp)); esp; })

// note that gcc may pass arguments by "movl arg, xx(%esp)"
#define SWITCH_TO_BOOT_STACK(esp)					\
     asm volatile ("movl %%esp, %0; movl %1, %%esp"			\
		 : "=m" (esp) : "r" (boot_stack-64)			\
		 : "memory")						\

#define RESTORE_STACK(esp)						\
  asm volatile ("movl %0, %%esp" : : "m" (save_esp) : "memory")

// Note for all wrapper functions: No locking while on the signal stack
// (Kmem::is_tcb_page_fault() is false). We are protected by the fact that
// all signals are disabled in a signal context. Also invoking cli from
// here leads to endless recursion with printf's in segv_handler.

extern "C"
int
__wrap_printf (const char *format, ...)
{
  if (!Stdout_console::active())
    return 0;

  unsigned esp = GET_ESP;

  if (Kmem::is_tcb_page_fault(esp, 0))
    {
      static va_list    va;
      static const char *_format;
      static unsigned   save_esp;
      static int        ret;

      Lock_guard <Cpu_lock> guard (&cpu_lock);

      _format = format;
      va_start (va, format);
      SWITCH_TO_BOOT_STACK(save_esp);
      ret = vprintf(_format, va);
      RESTORE_STACK(save_esp);
      va_end (va);

      return ret;
    }
  else
    {
      va_list va;
      int     ret;

      va_start(va, format);
      ret = vprintf(format, va);
      va_end(va);

      return ret;
    }
}


// Unfortunately, we also need a wrapper for snprintf althought snprintf
// calls vsnprintf. But glibc-snprintf calls __real_vsnprintf ...

extern "C"
int
__wrap_snprintf (char *str, size_t size, const char *format, ...)
{
  unsigned esp = GET_ESP;

  if (Kmem::is_tcb_page_fault(esp, 0))
    {
      static va_list    va;
      static const char *_format;
      static char       *_str;
      static size_t     _size;
      static unsigned   save_esp;
      static int        ret;

      Lock_guard <Cpu_lock> guard (&cpu_lock);

      _format = format;
      _str    = str;
      _size   = size;
      va_start (va, format);
      SWITCH_TO_BOOT_STACK(save_esp);
      ret = __real_vsnprintf(_str, _size, _format, va);
      RESTORE_STACK(save_esp);
      va_end (va);

      return ret;
    }
  else
    {
      va_list va;
      int     ret;

      va_start(va, format);
      ret = __real_vsnprintf(str, size, format, va);
      va_end(va);

      return ret;
    }
}

extern "C"
int
__wrap_vsnprintf (char *str, size_t size, const char *format, va_list va)
{
  if (!Stdout_console::active())
    return 0;

  unsigned esp = GET_ESP;

  if (Kmem::is_tcb_page_fault(esp, 0))
    {
      static const char *_format;
      static char       *_str;
      static size_t     _size;
      static va_list    _va;
      static unsigned   save_esp;
      static int        ret;

      Lock_guard <Cpu_lock> guard (&cpu_lock);

      _format = format;
      _str    = str;
      _size   = size;
      _va     = va;

      SWITCH_TO_BOOT_STACK(save_esp);
      ret = __real_vsnprintf(_str, _size, _format, _va);
      RESTORE_STACK(save_esp);

      return ret;
    }
  else
    {
      return __real_vsnprintf(str, size, format, va);
    }
}

extern "C"
int
__wrap_puts (const char *s)
{
  if (!Stdout_console::active())
    return 0;

  return __real_puts(s);
}

extern "C"
int
__wrap_putchar (int c)
{
  if (!Stdout_console::active())
    return 0;

  return __real_putchar(c);
}

void
register_libc_atexit(void (*f)(void))
{
  if (libc_atexit)
    panic("libc_atexit already registered");
  libc_atexit = f;
}

// Our own version of the assertion failure output function according
// to Linux Standard Base Specification.
// We need it since the standard glibc function calls printf, and doing
// that on a Fiasco kernel stack blows everything up.
extern "C"
void
__assert_fail (const char *assertion, const char *file,
               unsigned int line, const char *function)
{
  Kconsole::console()->change_state(0, 0, ~0U, Console::OUTENABLED);

  printf ("ASSERTION_FAILED (%s)\n"
          "  in function %s\n"
	  "  in file %s:%d\n",
          assertion, function, file, line);
  
  if (libc_atexit)
    libc_atexit();
  _exit (EXIT_FAILURE);         // Fatal! No destructors
}


int Stdout_console::_active = 1;

PUBLIC
int
Stdout_console::char_avail() const
{
  return -1; // unknown
}

PUBLIC
int
Stdout_console::write(char const * /*str*/, size_t len)
{
  return len;
}

PUBLIC
Mword
Stdout_console::get_attributes() const
{
  return OUT;
}

PUBLIC static inline
int
Stdout_console::active()
{
  return _active;
}

PUBLIC
void
Stdout_console::state(Mword new_state)
{
  if ((_state ^ new_state) & OUTENABLED)
    {
      if (new_state & OUTENABLED)
	_active = 1;
      else
	_active = 0;
    }

  _state = new_state;
}

static
void
stdout_console_init()
{
  static Stdout_console c;
  Kconsole::console()->register_console(&c);
}

STATIC_INITIALIZER_P(stdout_console_init, MAX_INIT_PRIO);
