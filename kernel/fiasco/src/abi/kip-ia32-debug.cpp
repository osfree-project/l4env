IMPLEMENTATION [{ia32,ux}-debug]:

#include <cstring>

#include "types.h"

IMPLEMENT
void Kip::print() const
{
  unsigned kips = kip_sys_calls;
  static char const* const KIPS[] = {"No KIP syscalls supported",
				     "KIP syscalls via KIP relative stubs",
				     "KIP syscalls via absolute stubs",
				     "KIP syscalls ERROR: bad value"};

  if(kips>3)
    kips = 3;

  printf("magic: %.4s  version: 0x%lx\n",(char*)&magic,version);
  printf("root memory:     [" L4_PTR_FMT "; " L4_PTR_FMT ")\n"
	 "main memory:     [" L4_PTR_FMT "; " L4_PTR_FMT ")\n"
	 "reserved[0..1]:  [" L4_PTR_FMT "; " L4_PTR_FMT ") ["
	 L4_PTR_FMT "; " L4_PTR_FMT ")\n"
	 "semi reserved:   [" L4_PTR_FMT "; " L4_PTR_FMT ")\n"
	 "dedicated[0..3]: [" L4_PTR_FMT "; " L4_PTR_FMT ") ["
	 L4_PTR_FMT "; " L4_PTR_FMT ")\n"
	 "                 [" L4_PTR_FMT "; " L4_PTR_FMT ") ["
	 L4_PTR_FMT "; " L4_PTR_FMT ")\n",
	 L4_PTR_ARG(root_memory.low), L4_PTR_ARG(root_memory.high),
	 L4_PTR_ARG(main_memory.low), L4_PTR_ARG(main_memory.high),
	 L4_PTR_ARG(reserved0.low), L4_PTR_ARG(reserved0.high),
	 L4_PTR_ARG(reserved1.low), L4_PTR_ARG(reserved1.high),
	 L4_PTR_ARG(semi_reserved.low), L4_PTR_ARG(semi_reserved.high),
	 L4_PTR_ARG(dedicated[0].low), L4_PTR_ARG(dedicated[0].high),
	 L4_PTR_ARG(dedicated[1].low), L4_PTR_ARG(dedicated[1].high),
	 L4_PTR_ARG(dedicated[2].low), L4_PTR_ARG(dedicated[2].high),
	 L4_PTR_ARG(dedicated[3].low), L4_PTR_ARG(dedicated[3].high));

  printf("clock: " L4_X64_FMT " (%lld)\n", clock, clock);
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

  char const *s = version_string();

  printf("Kernel features:");
  for (s += strlen(s) + 1; *s; s += strlen(s) + 1)
    printf(" %s", s);
  puts("");

}
