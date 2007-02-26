
#include <stdio.h>
#include <l4/sys/ipc.h>
#include <l4/util/rdtsc.h>

#include "global.h"

#if SCRATCH_MEM_SIZE < 16*1024*1024
#define FLOODER_MEM_SIZE  SCRATCH_MEM_SIZE
#else
#define FLOODER_MEM_SIZE  16*1024*1024
#endif

void
flooder(void)
{
  register int i;
  asm (".fill 8192,2,0x0075");
  for (i=0; i<FLOODER_MEM_SIZE; i+=32)
    asm volatile ("movl  %0,%%eax" : : "r"(scratch_mem + i) : "eax");
}

void
test_flooder(void)
{
  register int i;
  l4_cpu_time_t in,out;

  printf("Testing costs for flooder: ");
  in = l4_rdtsc();
  for (i=1000; i; i--)
    flooder();
  out = l4_rdtsc();

  flooder_costs = (l4_umword_t)((out-in)/1000);
  printf("%d cycles/turn\n", (l4_uint32_t)flooder_costs);
}

