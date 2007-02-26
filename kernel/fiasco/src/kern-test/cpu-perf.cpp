INTERFACE:

#define PERF_CNT_RETIRED_INST 0xc0

#define RDPMC(cnt,val) asm volatile("rdpmc": "=a"(val): "c"(cnt): "edx"); 

IMPLEMENTATION[perf]:

#include <assert.h>
#include <stdio.h>

#include "node.h"


PUBLIC unsigned
cpu_t::read_perf_cnt(unsigned cnt)
{
  assert(node::test_exec_cpu());
  assert(node::cpu_to_id(this) == node::id());
  assert((cnt == 0) || (cnt == 1));

#if 0
  asm __volatile__ ("cpuid"
		    : "=a"(tmp), "=b"(tmp), "=c"(tmp), "=d"(tmp)
		    : "0"(0));
#endif

  unsigned eax, edx, msr;

  msr = 0xc1 + cnt;

  asm volatile ("rdmsr"
		: "=a"(eax), "=d"(edx)
		: "c" (msr));
  return eax;
}


PUBLIC void
cpu_t::init_perf_cnt(unsigned cnt, unsigned event, unsigned unit)
{
  unsigned tmp;

  assert(cnt == 0);

  asm volatile ("cpuid" : "=d"(tmp), "=a"(tmp));
  disable_perf_cnt();

  reset_perf_cnt(cnt);

  set_perf_event(cnt, unit, event);

  enable_perf_cnt();
  asm volatile ("cpuid" : "=d"(tmp), "=a"(tmp));
}

PUBLIC void
cpu_t::dump_perf_cnt_settings()
{
  unsigned eax, edx;

  assert(node::test_exec_cpu());
  assert(node::cpu_to_id(this) == node::id());

  unsigned regs[4] = {0xc1, 0xc2, 0x186,0x187};

  for(int i=0; i<4; i++)
    {
      asm volatile("rdmsr": "=a"(eax), "=d"(edx): "c"(regs[i]));

      printf("cpu %d, msr reg %08x %08x:%08x\n", 
	     node::id(), regs[i], edx,eax);
    }

  for(int i=0; i<2; i++)
    {
      printf("cpu %d, cnt %d : %08x\n", node::id(), i, read_perf_cnt(i));
    }
}

PROTECTED void
cpu_t::set_perf_event(unsigned cnt, unsigned unit, unsigned event)
{
  unsigned eax, edx, tmp;

  assert((cnt == 0) || (cnt == 1));
  asm __volatile__("rdmsr" : "=a"(eax), "=d"(edx): "c"(0x186+cnt));

  eax &= 0xff1cffff;         // clear out 
  eax |= 0x30000;            // OS and USR events
  //  eax |= 0x1000000;          // counter mask = 1
  eax |= (unit << 8 ) | event;

  asm __volatile__("wrmsr"
		   : "=a"(tmp), "=d"(tmp)
		   : "0"(eax),  "1" (edx), "c"(0x186 +cnt));
}

PROTECTED void
cpu_t::reset_perf_cnt(unsigned cnt)
{
  unsigned tmp;

  assert((cnt == 0) || (cnt == 1));
  asm __volatile__("wrmsr"
		   : "=a"(tmp), "=d"(tmp)
		   : "0"(0), "1"(0), "c"(0xc1 + cnt));
}

PUBLIC void
cpu_t::disable_perf_cnt()
{
  unsigned eax, edx, tmp;

  assert(node::test_exec_cpu());
  assert(node::cpu_to_id(this) == node::id());

  asm __volatile__("rdmsr" : "=a"(eax), "=d"(edx): "c"(0x186));

  eax &= ~ 0x400000;  // clear bit 22

  asm __volatile__("wrmsr" : "=a"(tmp), "=d"(tmp):
		   "0"(eax), "1"(edx), "c"(0x186));
}

PUBLIC void
cpu_t::enable_perf_cnt()
{
  unsigned eax, edx, tmp;

  assert(node::test_exec_cpu());
  assert(node::cpu_to_id(this) == node::id());

  asm __volatile__("rdmsr" : "=a"(eax), "=d"(edx): "c"(0x186));

  eax |= 0x400000;  // clear bit 22

  asm __volatile__("wrmsr" 
		   : "=a"(tmp), "=d"(tmp)
		   : "0"(eax), "1"(edx), "c"(0x186));
}
