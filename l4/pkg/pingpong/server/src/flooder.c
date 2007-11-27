#include <l4/sys/ipc.h>
#include "global.h"

#if SCRATCH_MEM_SIZE < 16*1024*1024
#define FLOODER_MEM_SIZE  SCRATCH_MEM_SIZE
#else
#define FLOODER_MEM_SIZE  16*1024*1024
#endif

void
flooder(void)
{
  l4_umword_t dummy;

#ifdef BENCH_x86
  asm volatile (".fill 8192,2,0x0075	\n\t"
	        ".align 8		\n\t"
		"1:			\n\t"
		"testl %%eax,(%%edi)	\n\t"
		"decl  %%ecx		\n\t"
		"leal  32(%%edi), %%edi	\n\t"
		"jnz   1b		\n\t"
		: "=D"(dummy), "=c"(dummy)
		: "D"(scratch_mem), "c"(FLOODER_MEM_SIZE/32));
#else
  for (dummy = 0; dummy < FLOODER_MEM_SIZE; dummy += 32)
    *(unsigned long *)((unsigned long)scratch_mem + dummy) = 3344556;
#endif
}
