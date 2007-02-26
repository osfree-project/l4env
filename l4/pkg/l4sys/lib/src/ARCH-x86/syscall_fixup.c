#include <l4/sys/syscalls.h>

// make sure that unused symbols are not discarded
#if (__GNUC__ == 3 && __GNUC_MINOR__ >= 3) || __GNUC__ >= 4
#define SECTION(x)	__attribute__((used, section( x )))
#else
#define SECTION(x)	__attribute__((section( x )))
#endif

static char beg[0] SECTION(".mark_beg_l4syscalls");
static char end[0] SECTION(".mark_end_l4syscalls");

// This function has to be called per shared library before any L4 syscall
// .hidden prevents exporting this function from a shared library. We use
// assembler to tag the function as hidden since gcc-2.95 (which is still
// supported by L4env) does not support the visibility attribute.
asm (".hidden l4sys_fixup_abs_syscalls");
void l4sys_fixup_abs_syscalls(void);
void
l4sys_fixup_abs_syscalls(void)
{
  l4_syscall_patch_t *p;
  static int done;

  if (done)
    return;

  for (p=(l4_syscall_patch_t*)beg; p<(l4_syscall_patch_t*)end; p++)
    p->address += L4_ABS_syscall_page - (unsigned long)p->nop;

  done = 1;
}
