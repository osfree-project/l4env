/*
 * IA-32 Kernel-Info Page
 */

INTERFACE:

#include "types.h"

EXTENSION class Kernel_info
{
public:

  /* 00 */
  Mword magic;
  Mword version;
  Unsigned8 offset_version_strings;
  Unsigned8 reserved[3];
  Unsigned8 kip_sys_calls;
  Unsigned8 reserved01[3];

  /* the following stuff is undocumented; we assume that the kernel
     info page is located at offset 0x1000 into the L4 kernel boot
     image so that these declarations are consistent with section 2.9
     of the L4 Reference Manual */

  /* 10 */
  Mword init_default_kdebug, default_kdebug_exception, 
    __unknown, default_kdebug_end;

  /* 20 */
  Mword sigma0_esp, sigma0_eip;
  l4_low_high_t sigma0_memory;

  /* 30 */
  Mword sigma1_esp, sigma1_eip;
  l4_low_high_t sigma1_memory;
  
  /* 40 */
  Mword root_esp, root_eip;
  l4_low_high_t root_memory;

  /* 50 */
  Mword l4_config;
  Mword reserved2;
  Mword kdebug_config;
  Mword kdebug_permission;

  /* 60 */
  l4_low_high_t main_memory;
  l4_low_high_t reserved0;

  /* 70 */
  l4_low_high_t reserved1;
  l4_low_high_t semi_reserved;

  /* 80 */
  l4_low_high_t dedicated[4];

  /* A0 */
  volatile Cpu_time clock;
  Mword unused_4[2];

  /* B0 */
  Mword frequencies[2];
  Mword unused_5[2];

  /* C0 */
  Mword sys_ipc;
  Mword sys_id_nearest;
  Mword sys_fpage_unmap;
  Mword sys_thread_switch;

  /* D0 */
  Mword sys_thread_schedule;
  Mword sys_lthread_ex_regs;
  Mword sys_task_new;
  Mword unused_6;

  /* E0 */
  char  version_strings[256];
  char  sys_calls[];
};




IMPLEMENTATION[ia32]:

#include <cstdio>
#include "l4_types.h"

IMPLEMENT
void Kernel_info::print() const
{
  
  unsigned kips = kip_sys_calls;
  char const* const KIPS[] = {"No KIP syscalls supported",
			      "KIP syscalls via KIP relative stubs",
			      "KIP syscalls via absolute stubs",
			      "KIP syscalls ERROR: bad value"};

  if(kips>3) 
    kips = 3;

  printf("magic: %.4s  version: 0x%x\n",(char*)&magic,version);
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

  printf("clock: " L4_X64_FMT "\n", clock);
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
      printf("The following addresses are offsest in the KIP\n");
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


};
