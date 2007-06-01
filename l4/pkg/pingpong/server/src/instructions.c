
#include <stdio.h>
#include <l4/sys/ipc.h>
#include <l4/sys/ktrace.h>
#include "global.h"

#define LOOPS		1000000
#define LOCK

#define __STR(x) #x
#define STR(x) __STR(x)

#define cmpxchg8b_asm     "1:				\n\t" \
			  "movl   (%%esi),%%eax		\n\t" \
                          "movl   4(%%esi),%%edx	\n\t" \
                          "movl   %%eax,%%ebx		\n\t" \
                          "addl   $1,%%ebx		\n\t" \
	                  "movl   %%edx,%%ecx		\n\t" \
                          LOCK " cmpxchg8b  %1		\n\t" \
			  "jne    1b			\n\t"

#define cmpxchg_asm       "1:				\n\t" \
			  "movl   (%%esi),%%eax		\n\t" \
                          "movl   %%eax,%%ebx		\n\t" \
                          "addl   $1,%%ebx		\n\t" \
                          LOCK " cmpxchgl %%ebx,%1	\n\t" \
	                  "jne    1b			\n\t"

#define or_asm            LOCK " orl $1,(%%esi)		\n\t"

#define xchg_asm          "movl   %%eax,%%ebx		\n\t" \
                          "addl   $1,%%ebx		\n\t" \
                          LOCK " xchgl  %%ebx,(%%esi)	\n\t"

#define add_asm           LOCK " addl $1,(%%esi)	\n\t"

#define xadd_asm          "movl  $1,%%eax		\n\t" \
                          LOCK " xadd %%eax,%1		\n\t"

#define btc_asm           LOCK " btc %%eax, %1		\n\t"

#define bts_asm           LOCK " bts %%eax, %1		\n\t"

#define cli_asm           "cli				\n\t"

#define sti_asm           "sti				\n\t"

#define clisti_asm        "cli; sti			\n\t"

#define pushf_pop_asm     "pushf; pop %%eax		\n\t"

#define cpuid_asm         "xorl %%eax,%%eax; cpuid	\n\t"

#define rdtsc_asm         "rdtsc			\n\t"


#define test(ins) 					\
  do							\
    {							\
      BENCH_BEGIN;					\
      asm volatile ("pushl %%ebp		\n\t"	\
		    "movl  $"STR(LOOPS)",%%edi	\n\t"	\
		    "leal  %1, %%esi		\n\t"	\
		    "rdtsc			\n\t"	\
		    "movl  %%eax, %%ebp		\n\t"	\
		    "# for btc ...		\n\t"	\
		    "movl  $1, %%eax		\n\t"	\
		    ".align 16			\n\t"	\
		    "9:				\n\t"	\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    ins##_asm				\
		    "decl  %%edi		\n\t"	\
		    "jne   9b			\n\t"	\
		    "rdtsc			\n\t"	\
		    "subl  %%ebp, %%eax		\n\t"	\
		    "popl  %%ebp		\n\t"	\
		    : "=a" (cycles)			\
		    : "m" (atomic_ll)			\
		    : "ebx","ecx","edx",		\
		      "esi","edi","memory");		\
      BENCH_END;					\
      printf("   %-10s takes about %3u cycles\n",	\
	  STR(ins), (cycles+10*LOOPS)/(20*LOOPS));	\
    } while (0)

static inline int get_iopl(void)
{
  unsigned long e;
  asm volatile("pushf; pop %0" : "=rm"(e));
  return (e & 0x3000) >> 12;
}

long long atomic_ll __attribute__((aligned(512))) = 0;

void
test_instruction_cycles(int nr)
{
  unsigned cycles;

  printf(">> %c: Testing instruction cycles (CPU %dMhz):\n", nr, mhz);
  test(bts);
  test(btc);
  test(add);
  test(xadd);
  test(or);
  test(xchg);
  test(cmpxchg);
  test(cmpxchg8b);
  test(pushf_pop);
  if (!ux_running)
    {
      if (get_iopl() == 3)
        {
	  test(cli);
	  test(sti);
	  test(clisti);
	}
      else
	printf("   Skipping CLI/STI tests because not running with IOPL3\n");
    }
  test(cpuid);
  test(rdtsc);
}
