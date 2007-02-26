#include <stdio.h>
#include <l4/crtx/ctor.h>
#include <l4/sys/syscalls.h>

extern void bar_in_binary(void);

void          foo_in_library_1(void);
l4_threadid_t l4_myself_in_library_1(void);

void
foo_in_library_1(void)
{
  printf("This is output from foo_in_library_1()\n");
  bar_in_binary();
}

l4_threadid_t
l4_myself_in_library_1(void)
{
  return l4_myself();
}

static void __attribute__((constructor))
initializer1(void)
{
  printf("This is output from initializer1 in library1\n");
}

static void
initializer2(void)
{
  printf("This is output from initializer2 in library1\n");
}

L4C_CTOR(initializer2, L4CTOR_BACKEND);
