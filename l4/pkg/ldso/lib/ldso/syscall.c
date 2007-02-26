#include "dl-string.h"
#include "syscall.h"
#include <l4/sys/syscalls.h>

typedef struct
{
  const char *syscall_name;
  l4_addr_t   syscall_addr;
} fixup_t;

static fixup_t fixup[] =
{
  { "ipc_direct",             L4_ABS_syscall_page + L4_ABS_ipc             },
  { "id_nearest_direct",      L4_ABS_syscall_page + L4_ABS_id_nearest      },
  { "fpage_unmap_direct",     L4_ABS_syscall_page + L4_ABS_fpage_unmap     },
  { "thread_switch_direct",   L4_ABS_syscall_page + L4_ABS_thread_switch   },
  { "thread_schedule_direct", L4_ABS_syscall_page + L4_ABS_thread_schedule },
  { "lthread_ex_regs_direct", L4_ABS_syscall_page + L4_ABS_lthread_ex_regs },
  { "task_new_direct",        L4_ABS_syscall_page + L4_ABS_task_new        },
  { "privctrl_direct",        L4_ABS_syscall_page + L4_ABS_privctrl        },
};

/* This is necessary: If a shared library contains non-PIC code then L4
 * syscalls generate external references to absolute addresses. If the
 * binary provides these symbols, everything would be fine since the Linker
 * resolves the relocation entries using the symbols of the binary. If the
 * binary does _not_ provide entries for these symbols, the linker resolves
 * these relocating entries using the entries of the shared library itself.
 * But these addresses are still relocate ... */
int
_dl_syscall_fixup(const char *symname, unsigned long *reloc_addr)
{
  if (!_dl_strncmp(symname, "__l4sys_", 8))
    {
      int i;

      for (i=0; i<sizeof(fixup)/sizeof(fixup[0]); i++)
	if (!_dl_strcmp(symname+8, fixup[i].syscall_name))
	  {
	    *reloc_addr = fixup[i].syscall_addr
	                - (unsigned long)reloc_addr - 4;
	    return 1;
	 }
    }
  return 0;
}
