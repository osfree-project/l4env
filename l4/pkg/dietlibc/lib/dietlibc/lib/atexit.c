/* by aw11 
 * derived from dietlibc's atexit.c
 */

#include <stdlib.h>
#include <unistd.h>

typedef void (*function)(void);

#define NUM_ATEXIT      64

struct exititem
{
  void (*f)(void*);
  void *arg;
  void *dso_handle;
};

static struct exititem __atexitlist[NUM_ATEXIT];
static int atexit_counter;

extern void *__dso_handle __attribute__ ((__weak__));

int __cxa_atexit(void (*f)(void*), void *arg, void *dso_handle)
{
  int c = atexit_counter++;
  if (c >= NUM_ATEXIT)
    return -1;
  
  struct exititem *h = &__atexitlist[c];
  h->f = f;
  h->arg = arg;
  h->dso_handle = dso_handle;

  return 0;
}

int atexit(function t) 
{
  return __cxa_atexit((void (*)(void*))t, NULL,
      &__dso_handle == NULL ? NULL : __dso_handle);
}

void __cxa_finalize(void *dso_handle)
{
  register int i = atexit_counter;

  if (i>NUM_ATEXIT)
    i = NUM_ATEXIT;

  while(i) 
    {
      struct exititem *h = &__atexitlist[--i];
      if (h->f && (dso_handle == 0 || h->dso_handle == dso_handle)) 
	{
	  h->f(h->arg);
	  h->f = 0;
	}
    }
}

extern void __thread_doexit(int doexit);

void __libc_exit(int code);
void __libc_exit(int code) {
  __thread_doexit(code);
  __cxa_finalize(0);
  _exit(code);
}
void exit(int code) __attribute__((alias("__libc_exit")));

