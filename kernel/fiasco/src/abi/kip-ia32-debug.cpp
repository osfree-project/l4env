IMPLEMENTATION [{ia32,ux}-debug]:

#include <cstring>
#include "types.h"

IMPLEMENT
void
Kip::debug_print_syscalls() const
{
  unsigned kips = kip_sys_calls;
  static char const* const KIPS[] = {"No KIP syscalls supported",
				     "KIP syscalls via KIP relative stubs",
				     "KIP syscalls via absolute stubs",
				     "KIP syscalls ERROR: bad value"};

  if(kips>3)
    kips = 3;
  
  printf("%s\n",KIPS[kips]);

  Mword
    s_ipc = sys_ipc,
    s_id_n= sys_id_nearest,
    s_fpu = sys_fpage_unmap,
    s_tsw = sys_thread_switch,
    s_tsc = sys_thread_schedule,
    s_lex = sys_lthread_ex_regs,
    s_tn  = sys_task_new;

  switch(kips)
    {
    default:
      break;
    case 1:
      printf("The following addresses are offsets in the KIP\n");
    case 2:
      printf("  ipc:             " L4_PTR_FMT "\n"
	     "  id nearest:      " L4_PTR_FMT "\n"
	     "  fpage unmap:     " L4_PTR_FMT "\n"
	     "  thread switch:   " L4_PTR_FMT "\n"
	     "  thread schedule: " L4_PTR_FMT "\n"
	     "  lthread ex regs: " L4_PTR_FMT "\n"
	     "  task new:        " L4_PTR_FMT "\n",
	     L4_PTR_ARG(s_ipc), L4_PTR_ARG(s_id_n),
	     L4_PTR_ARG(s_fpu), L4_PTR_ARG(s_tsw),
	     L4_PTR_ARG(s_tsc), L4_PTR_ARG(s_lex),
	     L4_PTR_ARG(s_tn) );

      break;
    }

}

