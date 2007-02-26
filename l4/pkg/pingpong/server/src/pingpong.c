/* $Id$ */
/*****************************************************************************/
/**
 * \file   pingpong/server/src/pingpong.c
 * \brief  Pingpong IPC benchmark
 */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/rmgr/librmgr.h>
#include <l4/util/util.h>
#include <l4/util/rdtsc.h>
#include <l4/util/parse_cmd.h>
#include <l4/util/bitops.h>

#include "global.h"
#include "idt.h"
#include "pingpong.h"
#include "worker.h"
#include "helper.h"

#define NR_MSG        8
#define NR_DWORDS     4093  /* whole message descriptor occupies 4 pages */
#define NR_STRINGS    4

#define SMAS_SIZE     16

/* threads */
l4_threadid_t main_id;		/* main thread */
l4_threadid_t pager_id;		/* local pager */
l4_threadid_t ping_id;		/* ping thread */
l4_threadid_t pong_id;		/* pong thread */
l4_threadid_t intra_ping, intra_pong;
l4_threadid_t inter_ping, inter_pong;
l4_threadid_t memcpy_id;
l4_umword_t scratch_mem;		/* memory for mapping */
l4_umword_t fpagesize;
l4_umword_t strsize;
l4_umword_t strnum;
l4_umword_t rounds;
l4_uint32_t mhz;
l4_uint32_t timer_hz;
l4_uint32_t timer_resolution;
l4_uint64_t flooder_costs;
int use_superpages;
int sysenter;
int cold; 

static int dont_do_sysenter;
static int dont_do_superpages;
static int dont_do_multiple_fpages;
static int dont_do_ipc_asm;
       int dont_do_cold;
static int smas_pos[2] = {0,0};


/* stacks */
static unsigned char kip[L4_PAGESIZE] __attribute__((aligned(4096)));
static unsigned char pager_stack[STACKSIZE] __attribute__((aligned(4096)));
static unsigned char ping_stack[STACKSIZE] __attribute__((aligned(4096)));
static unsigned char pong_stack[STACKSIZE] __attribute__((aligned(4096)));

extern void pc_reset(void);

#ifndef FIASCO_UX
static void
test_timer_frequency(void)
{
  asm volatile ("movl  (%%ebx), %%ecx	\n\t"
		"1:			\n\t"
		"cmpl  (%%ebx), %%ecx	\n\t"
		"je    1b		\n\t"
		"movl  (%%ebx), %%ecx	\n\t"
		"rdtsc			\n\t"
		"movl  %%eax, %%esi	\n\t"
		"movl  %%edx, %%edi	\n\t"
		"1:			\n\t"
		"cmpl  (%%ebx), %%ecx	\n\t"
		"je    1b		\n\t"
		"rdtsc			\n\t"
		"subl  %%esi, %%eax	\n\t"
		"subl  (%%ebx), %%ecx	\n\t"
		"negl  %%ecx		\n\t"
		:"=a"(timer_hz), "=c"(timer_resolution)
		:"b" (&kip[0xa0])
		:"edx");
}
#endif

/** guess kernel by looking at kernel info page version number */
static void
detect_kernel(void)
{
  l4_umword_t dummy;
  l4_msgdope_t result;

  l4_fpage_unmap(l4_fpage((l4_umword_t)kip, L4_LOG2_PAGESIZE, 0, 0), 
		 L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);
  l4_ipc_call(rmgr_pager_id, 
		   L4_IPC_SHORT_MSG, 1, 1,
		   L4_IPC_MAPMSG((l4_umword_t)kip, L4_LOG2_PAGESIZE), 
		     &dummy, &dummy,
		   L4_IPC_NEVER, &result);
  if (L4_IPC_IS_ERROR(result) || !l4_ipc_fpage_received(result))
    {
      printf("failed to map kernel info page (%02x)\n",L4_IPC_ERROR(result));
      return;
    }

  printf("Kernel version %08x: ", ((l4_uint32_t*)kip)[1]);
  switch (((l4_uint32_t*)kip)[1])
    {
    case 21000:
      puts("Jochen LN -- disabling 4MB mappings");
      dont_do_superpages = 1;
      dont_do_sysenter = 1;
      break;
    case 0x0004711:
      puts("L4Ka Hazelnut -- disabling multiple flexpages");
      dont_do_multiple_fpages = 1;
      dont_do_ipc_asm = 1;
      break;
    case 0x01004444:
      puts("Fiasco");
      break;
    default:
      puts("Unknown");
      break;
    }
#ifdef FIASCO_UX
  dont_do_sysenter = 1;
  dont_do_cold = 1;
#else
  test_timer_frequency();
#endif
  l4_fpage_unmap(l4_fpage((l4_umword_t)kip, L4_LOG2_PAGESIZE, 0, 0), 
		 L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);
}

/** determine if cpu does not support sysenter */
static void
detect_sysenter(void)
{
  if (!dont_do_sysenter)
    {
      asm volatile ("xorl  %%eax,%%eax	\n\t"
		    "cpuid		\n\t"	// cpuid(0)
		    "movl  $1,%%ecx	\n\t"
		    "test  %%eax,%%eax	\n\t"	// max # cpuid < 1?
		    "je    1f		\n\t"	// yes => no sysenter
		    "movl  $1,%%eax	\n\t"
		    "cpuid		\n\t"	// cpuid(1)
		    "movl  $1,%%ecx	\n\t"
		    "testl $0x800,%%edx	\n\t"	// feature SEP?
		    "je    1f		\n\t"	// no => no sysenter
		    "andl  $0xfff,%%eax	\n\t"
		    "cmpl  $0x633,%%eax	\n\t"	// cpu version < 0x633
		    "jb    1f		\n\t"	// yes => sysenter broken
		    "xorl  %%ecx,%%ecx	\n\t"
		    "1:			\n\t"
		    : "=c"(dont_do_sysenter)
		    :
		    : "eax","ebx","edx","memory");
    }
}

#if DO_DEBUG
void
hello(const char *name)
{
  printf("%s: %x.%x\n",name,l4_myself().id.task,l4_myself().id.lthread);
}
#endif

void do_sleep(void);
extern void asm_do_sleep(void);

/** "shutdown" thread */
void __attribute__((noreturn))
do_sleep(void)
{
  /* just sleep forever */
  for (;;)
    {
      /* the asm label allows to go sleep without touching the stack */
      asm ("asm_do_sleep:\n\t"
	   "movl $-1,%%eax    \n\t"
	   "sub  %%ecx,%%ecx  \n\t"
	   "sub  %%esi,%%esi  \n\t"
	   "sub  %%edi,%%edi  \n\t"
	   "sub  %%ebp,%%ebp  \n\t"
	   "int  $0x30        \n\t"
	   : : : "memory");
    }
}

/** print name of test */
static void
print_testname(const char *test, int nr, int inter, int show_sysenter)
{
#ifdef FIASCO_UX
  show_sysenter = 0;
#endif
  printf(">> %d: %s: %x.%x <==> %x.%x"
#ifndef FIASCO_UX
         ", CPU %dMHz, L4 Timer %dHz"
#endif
	 "%s\n",
         nr, test,
	 inter == INTER ? inter_ping.id.task    : intra_ping.id.task, 
	 inter == INTER ? inter_ping.id.lthread : intra_ping.id.lthread,
	 inter == INTER ? inter_pong.id.task    : intra_pong.id.task, 
	 inter == INTER ? inter_pong.id.lthread : intra_pong.id.lthread,
#ifndef FIASCO_UX
	 mhz, timer_hz,
#endif
	 show_sysenter ? sysenter ? " (sysenter)" : " (int30)"
		       : "");
}

static void move_to_small_space(l4_threadid_t id, int nr)
{
  l4_threadid_t foo;
  l4_sched_param_t sched;

  foo = L4_INVALID_ID;
  l4_thread_schedule(id, L4_INVALID_SCHED_PARAM,
                     &foo, &foo,
                     &sched);
  sched.sp.small = L4_SMALL_SPACE( SMAS_SIZE, nr);          
  foo = L4_INVALID_ID;
  l4_thread_schedule(id, sched,
                     &foo, &foo,
                     &sched);

#if DEBUG
  printf("thread %x.%x move to small space %u\n",
             id.id.task,id.id.lthread, sched.sp.small);
#endif
}


/** Create thread
 * \param  id            Thread Id
 * \param  eip           EIP of thread function
 * \param  esp           Stack pointer
 * \param  pager         Pager thread */
void
create_thread(l4_threadid_t id, l4_umword_t eip, l4_umword_t esp, 
	      l4_threadid_t new_pager)
{
  l4_threadid_t pager,preempter;
  l4_umword_t dummy;

#if DO_DEBUG
  printf("creating thread %x.%x, eip 0x%08x, esp 0x%08x, pager %x.%x\n",
	 id.id.task,id.id.lthread,eip,esp,
	 new_pager.id.task,new_pager.id.lthread);
#endif

  /* get our preempter/pager */
  preempter = pager = L4_INVALID_ID;
  l4_thread_ex_regs(l4_myself(),(l4_umword_t)-1,(l4_umword_t)-1,&preempter,
                    &pager,&dummy,&dummy,&dummy);

  /* create thread */
  l4_thread_ex_regs(id,eip,esp,&preempter,&new_pager,
		    &dummy,&dummy,&dummy);

}

/** create ping and pong tasks */
static void
create_pingpong_tasks(void (*ping_thread)(void),
		      void (*pong_thread)(void))
{
  l4_threadid_t t;

  ping_id = inter_ping;
  pong_id = inter_pong;

  /* first create pong task */
  t = rmgr_task_new(pong_id,255,(l4_umword_t)pong_stack + STACKSIZE,
		    (l4_umword_t)pong_thread,pager_id);
  if (l4_is_nil_id(t) || l4_is_invalid_id(t))
    {
      printf("failed to create pong task (%x)\n",pong_id.id.task);
      return;
    }

  t = rmgr_task_new(ping_id,255,(l4_umword_t)ping_stack + STACKSIZE,
		    (l4_umword_t)ping_thread,pager_id);
  if (l4_is_nil_id(t) || l4_is_invalid_id(t))
    {
      printf("failed to create ping task (%x)\n",ping_id.id.task);
      rmgr_task_new(pong_id,rmgr_id.lh.low,0,0,L4_NIL_ID);
      return;
    }

  move_to_small_space(ping_id, smas_pos[0]);
  move_to_small_space(pong_id, smas_pos[1]);

  /* shake hands with pong to ensure pong that ping is already created */
  recv(pong_id);
  send(pong_id);
}

/** kill ping and pong tasks */
static void
kill_pingpong_tasks(void)
{
  /* delete ping and pong tasks */
  rmgr_task_new(ping_id,rmgr_id.lh.low,0,0,L4_NIL_ID);
  rmgr_task_new(pong_id,rmgr_id.lh.low,0,0,L4_NIL_ID);
}

/** map 4k page from "from" to addr */
static void
map_4k_page(l4_threadid_t from, l4_umword_t addr)
{
  l4_snd_fpage_t fpage;
  l4_msgdope_t result;

  l4_ipc_call(from,
		   L4_IPC_SHORT_MSG, addr, L4_LOG2_PAGESIZE,
		   L4_IPC_MAPMSG(addr, L4_LOG2_PAGESIZE),
		     &fpage.snd_base, &fpage.fpage.fpage,
		   L4_IPC_NEVER, &result);
  if (L4_IPC_IS_ERROR(result))
    {
      printf("IPC error %02x mapping 4kB page at %08x from %x.%02x\n", 
	     L4_IPC_ERROR(result), addr, from.id.task, from.id.lthread);
      enter_kdebug("map_4k_page");
    }
  else if (fpage.fpage.fp.size != L4_LOG2_PAGESIZE)
    {
      printf("Can't map 4kB page at %08x from %x.%02x\n", 
	     addr, from.id.task, from.id.lthread);
      enter_kdebug("map_4k_page");
    }
}

/** map 4M page from "from" to addr */
static void
map_4m_page(l4_threadid_t from, l4_umword_t addr)
{
  l4_snd_fpage_t fpage;
  l4_msgdope_t result;

  l4_ipc_call(from,
		   L4_IPC_SHORT_MSG, addr|1, L4_LOG2_SUPERPAGESIZE << 2,
		   L4_IPC_MAPMSG(addr, L4_LOG2_SUPERPAGESIZE),
		     &fpage.snd_base, &fpage.fpage.fpage,
		   L4_IPC_NEVER, &result);
  if (L4_IPC_IS_ERROR(result))
    {
      printf("IPC error %02x mapping 4MB page at %08x from %x.%x\n", 
	     L4_IPC_ERROR(result), addr, from.id.task, from.id.lthread);
      enter_kdebug("map_4M_page");
    }
  else if (fpage.fpage.fp.size != L4_LOG2_SUPERPAGESIZE)
    {
      printf("Can't map 4MB page at %08x from %x.%02x\n", 
	     addr, from.id.task, from.id.lthread);
      enter_kdebug("map_4M_page");
    }
}

/** Map scratch memory from RMGR as 4M pages */
static void
map_scratch_mem_from_rmgr(void)
{
  l4_umword_t a;

  for (a=scratch_mem; a<scratch_mem+SCRATCH_MEM_SIZE; a+=L4_SUPERPAGESIZE)
    map_4m_page(rmgr_pager_id, a);
}

/** map scratch memory from page into current task */
void
map_scratch_mem_from_pager(void)
{
  l4_umword_t a;

  for (a=scratch_mem; a<scratch_mem+SCRATCH_MEM_SIZE; a+=L4_PAGESIZE)
    {
      if (use_superpages)
	map_4m_page(pager_id, a);
      else
	map_4k_page(pager_id, a);
    }
}

/** Pingpong thread pager */
static void __attribute__((noreturn))
pager(void)
{
  l4_threadid_t src;
  l4_umword_t pfa,eip,snd_base,fp;
  l4_msgdope_t result;
  extern char _start;
  extern char _end;

#if DO_DEBUG
  hello("pager");
#endif

  while (1)
    {
      l4_ipc_wait(&src,L4_IPC_SHORT_MSG,&pfa,&eip,L4_IPC_NEVER,&result);
      while (1)
	{
	  if (use_superpages &&
	      pfa >= scratch_mem &&
	      pfa <= scratch_mem + SCRATCH_MEM_SIZE)
	    {
	      snd_base = pfa & L4_SUPERPAGEMASK;
	      fp = snd_base | (L4_LOG2_SUPERPAGESIZE << 2);
	    }
	  else
	    {
	      snd_base = pfa & L4_PAGEMASK;
	      fp = snd_base | (L4_LOG2_PAGESIZE << 2);
	    }

	  /* sanity check */
      	  if (   (pfa < (unsigned)&_start || pfa > (unsigned)&_end)
	      && (pfa < scratch_mem || pfa > scratch_mem+SCRATCH_MEM_SIZE))
	      
		{
		  printf("Pager: pfa=%08x eip=%08x from %x.%02x received\n", 
		      pfa, eip, src.id.task, src.id.lthread);
		  enter_kdebug("stop");
		}
	  if (pfa & 2)
    	    fp |= 2;

	  l4_ipc_reply_and_wait(src,L4_IPC_SHORT_FPAGE,snd_base,fp,
				     &src,L4_IPC_SHORT_MSG,&pfa,&eip,
			    	     L4_IPC_NEVER,&result);
	  if (L4_IPC_IS_ERROR(result))
	    {
	      printf("pager: IPC error (dope=0x%08x) "
		     "serving pfa=%08x eip=%08x from %x.%02x\n",
		     result.msgdope, pfa, eip, src.id.task, src.id.lthread);
	      break;
	    }
	}
    }
}

/** IPC benchmark - short INTRA-AS */
static void
benchmark_short_intraAS(int nr)
{
  /* start ping and pong thread */
  print_testname("short intra IPC", 0, INTRA, 0);

  ping_id = intra_ping;
  pong_id = intra_pong;

  for (cold=0; cold<2-dont_do_cold; cold++)
    {
      for (sysenter=0; sysenter<2-dont_do_sysenter; sysenter++)
	{
	  create_thread(ping_id, 
		    (l4_umword_t)(sysenter 
				    ? cold ? sysenter_ping_short_cold_thread
				           : sysenter_ping_short_thread
				    : cold ? int30_ping_short_cold_thread
				           : int30_ping_short_thread),
		    (l4_umword_t)ping_stack + STACKSIZE,pager_id);

	  create_thread(pong_id,
		    (l4_umword_t)(sysenter ? sysenter_pong_short_thread
					   : int30_pong_short_thread),
		    (l4_umword_t)pong_stack + STACKSIZE,pager_id);

	  move_to_small_space(ping_id, smas_pos[0]);

	  recv(pong_id);
	  send(pong_id);

	  /* wait for measurement to be finished */
	  recv_ping_timeout(timeout_10s);

	  /* shutdown pong thread, ping thread already sleeps */
	  create_thread(pong_id,(l4_umword_t)asm_do_sleep,
		    (l4_umword_t)pong_stack + STACKSIZE,pager_id);

	  /* give ping thread time to go to bed */
	  send(ping_id);

	  /* give pong thread time to go to bed */
	  l4_thread_switch(L4_NIL_ID);

	  move_to_small_space(ping_id, 0);
	}
    }
}

/** IPC benchmark - short INTER-AS */
static void
benchmark_short_interAS(int nr)
{
  print_testname("short inter IPC", nr, INTER, 0);

  for (cold=0; cold<2-dont_do_cold; cold++)
    {
      for (sysenter=0; sysenter<2-dont_do_sysenter; sysenter++)
	{
	  create_pingpong_tasks(sysenter 
				  ? cold ? sysenter_ping_short_cold_thread
					 : sysenter_ping_short_thread
				  : cold ? int30_ping_short_cold_thread
					 : int30_ping_short_thread,
				 sysenter 
				   ? sysenter_pong_short_thread
				   : int30_pong_short_thread);

	  /* wait for measurement to be finished, then kill tasks */
	  recv_ping_timeout(timeout_10s);
	  kill_pingpong_tasks();
	}
    }
}

/** IPC benchmark - long INTER-AS */
static void
benchmark_long_interAS(int nr)
{
  static struct
    {
      l4_umword_t sz;
      l4_umword_t rounds;
    } strsizes[]
  =
    {
	{.sz=   4,.rounds=12500}, {.sz=   8,.rounds=12500},
	{.sz=  16,.rounds=12500}, {.sz=  64,.rounds=12500},
	{.sz= 256,.rounds=12500}, {.sz=1024,.rounds=12500},
	{.sz=2048,.rounds= 6250}, {.sz=4093,.rounds= 6250}
    };
  int i;

  sysenter = 1-dont_do_sysenter;
  print_testname("long inter IPC", nr, INTER, 1);

  for (cold=0; cold<2-dont_do_cold; cold++)
    {
      for (i=0; i<sizeof(strsizes)/sizeof(strsizes[0]); i++)
	{
	  strsize = strsizes[i].sz;
	  rounds  = strsizes[i].rounds;
#ifdef FIASCO_UX
	  rounds /= 10;
#endif
	  create_pingpong_tasks(sysenter 
				   ? cold ? sysenter_ping_long_cold_thread
					  : sysenter_ping_long_thread
				   : cold ? int30_ping_long_cold_thread
					  : int30_ping_long_thread,
				sysenter ? sysenter_pong_long_thread
					 : int30_pong_long_thread);

    	  /* wait for measurement to be finished, then kill tasks */
	  recv_ping_timeout(timeout_20s);
	  kill_pingpong_tasks();
	}
    }
}

/** IPC benchmark - long INTER-AS */
static void
benchmark_indirect_interAS(int nr)
{
  static struct 
    { 
      l4_umword_t sz;
      l4_umword_t num;
      l4_umword_t rounds;
    } strsizes[] 
  = 
    {
	{.sz=   16,.num=4,.rounds=10000},{.sz=   64, .num=1,.rounds=10000},
	{.sz=  256,.num=4,.rounds= 4000},{.sz= 1024, .num=1,.rounds= 4000},
	{.sz=  512,.num=4,.rounds= 3000},{.sz= 2048, .num=1,.rounds= 3000},
	{.sz= 1024,.num=4,.rounds= 2000},{.sz= 4096, .num=1,.rounds= 2000},
	{.sz= 2048,.num=4,.rounds=  500},{.sz= 8192, .num=1,.rounds=  500},
	{.sz= 4096,.num=4,.rounds=  400},{.sz=16384, .num=1,.rounds=  400}
    };
  int i;

  sysenter = 1-dont_do_sysenter;
  print_testname("indirect inter IPC", nr, INTER, 1);

  for (cold=0; cold<2-dont_do_cold; cold++)
    {
      for (i=0; i<sizeof(strsizes)/sizeof(strsizes[0]); i++)
	{
	  strsize = strsizes[i].sz;
	  strnum  = strsizes[i].num;
	  rounds  = strsizes[i].rounds;
#ifdef FIASCO_UX
	  rounds /= 10;
#endif
	  create_pingpong_tasks(sysenter 
				   ? cold ? sysenter_ping_indirect_cold_thread
					  : sysenter_ping_indirect_thread
				   : cold ? int30_ping_indirect_cold_thread
					  : int30_ping_indirect_thread,
				sysenter ? sysenter_pong_indirect_thread
					 : int30_pong_indirect_thread);
	  /* wait for measurement to be finished, then kill tasks */
	  recv_ping_timeout(timeout_20s);
	  kill_pingpong_tasks();
	}
    }
}

/** IPC benchmark - Mapping test */
static void
benchmark_shortMap_test(void)
{
#ifdef LOW_MEM
  static int fpagesizes[] = { 1, 2, 4, 16, 64, 256, 512 };
#else
  static int fpagesizes[] = { 1, 2, 4, 16, 64, 256, 1024 };
#endif
  int i;

  sysenter = 1-dont_do_sysenter;
  for (i=0; i<sizeof(fpagesizes)/sizeof(fpagesizes[0]); i++)
    {
      fpagesize = fpagesizes[i]*L4_PAGESIZE;
      rounds    = SCRATCH_MEM_SIZE/(fpagesize*8);

      create_pingpong_tasks(sysenter ? sysenter_ping_fpage_thread
				     : int30_ping_fpage_thread, 
			    sysenter ? sysenter_pong_fpage_thread
				     : int30_pong_fpage_thread);

      /* wait for measurement to be finished, then kill tasks */
      recv_ping_timeout(timeout_10s);
      kill_pingpong_tasks();
    }
}

/** IPC benchmark - Mapping */
static void
benchmark_shortMap_interAS(int nr)
{
  use_superpages = 0;
  sysenter = 1-dont_do_sysenter;
  print_testname("short fpage map from 4k", nr, INTER, 1);

  benchmark_shortMap_test();

  if (!dont_do_superpages)
    {
      use_superpages = 1;
      print_testname("short fpage map from 4M", nr, INTER, 1);
      benchmark_shortMap_test();
    }
  else
    puts("INTER-AS-SHORT-FP disabled from 4M because Jochen LN detected");
}

/** IPC benchmark - long fpages */
static void
benchmark_longMap_interAS(int nr)
{
  rounds = SCRATCH_MEM_SIZE/(1024*L4_PAGESIZE);
  if (rounds > NR_MSG)
    rounds = NR_MSG;

  sysenter = 1-dont_do_sysenter;
  print_testname("long fpage map", nr, INTER, 1);
  for (cold=0; cold<2-dont_do_cold; cold++)
    {
      create_pingpong_tasks(sysenter 
				? cold ? sysenter_ping_long_fpage_cold_thread
				       : sysenter_ping_long_fpage_thread
				: cold ? int30_ping_long_fpage_cold_thread
				       : int30_ping_long_fpage_thread,
			    sysenter ? sysenter_pong_long_fpage_thread
				     : int30_pong_long_fpage_thread);

      /* wait for measurement to be finished, then kill tasks */
      recv_ping_timeout(timeout_10s);
      kill_pingpong_tasks();
    }
}

/** IPC benchmark - pagefault test */
static void
benchmark_pagefault_test(void)
{
  rounds = SCRATCH_MEM_SIZE/(fpagesize*8);
  sysenter = 1-dont_do_sysenter;
  create_pingpong_tasks(sysenter ? sysenter_ping_pagefault_thread
				 : int30_ping_pagefault_thread,
			sysenter ? sysenter_pong_pagefault_thread
				 : int30_pong_pagefault_thread);

  /* wait for measurement to be finished, then kill tasks */
  recv_ping_timeout(timeout_10s);
  kill_pingpong_tasks();
}

/** IPC benchmark - pagefaults */
static void
benchmark_pagefault_interAS(int nr)
{
  sysenter = 1-dont_do_sysenter;
  print_testname("pagefault inter address space", nr, INTER, 1);

  fpagesize = L4_PAGESIZE;
  use_superpages = 0;
  benchmark_pagefault_test();

  if (!dont_do_superpages)
    {
      fpagesize = L4_PAGESIZE;
      use_superpages = 1;
      benchmark_pagefault_test();

      /* We need at least 32 MB for this test */
      if (SCRATCH_MEM_SIZE >= (32*1024*1024))
        {
          fpagesize = L4_SUPERPAGESIZE;
          use_superpages = 1;
          benchmark_pagefault_test();
	}

    }
  else
    puts("INTER-AS-PF disabled from 4M because Jochen LN detected");
}

static void
benchmark_exception_intraAS(int nr)
{
  /* start ping and pong thread */
  sysenter = 0;
  print_testname("intra exceptions", 7, INTRA, 0);

  ping_id = intra_ping;

  move_to_small_space(ping_id, smas_pos[0]);

  create_thread(ping_id,(l4_umword_t)(ping_exception_thread),
		(l4_umword_t)ping_stack + STACKSIZE,pager_id);

  /* wait for measurement to be finished */
  recv_ping_timeout(timeout_10s);
  /* give ping thread time to go to bed */
  send(ping_id);

  move_to_small_space(ping_id, 0);
}

/** IPC benchmark - short INTER-AS with c-bindings */
static void
benchmark_short_c_interAS(int nr)
{
  print_testname("short inter IPC (c-inline-bindings)", nr, INTER, 0);
  for (cold=0; cold<2-dont_do_cold; cold++)
    {
      for (sysenter=0; sysenter<2-dont_do_sysenter; sysenter++)
	{
	  create_pingpong_tasks(sysenter 
				   ? cold ? sysenter_ping_short_c_cold_thread
					  : sysenter_ping_short_c_thread
				   : cold ? int30_ping_short_c_cold_thread
					  : int30_ping_short_c_thread,
				sysenter ? sysenter_pong_short_c_thread
					 : int30_pong_short_c_thread);

	  /* wait for measurement to be finished, then kill tasks */
	  recv_ping_timeout(timeout_10s);
	  kill_pingpong_tasks();
	}
    }
}

/** IPC benchmark - short INTER-AS with extern assembler functions */
static void
benchmark_short_asm_interAS(int nr)
{
  print_testname("short inter IPC (external assembler)", nr, INTER, 0);
  for (cold=0; cold<2-dont_do_cold; cold++)
    {
      for (sysenter=0; sysenter<2-dont_do_sysenter; sysenter++)
	{
	  create_pingpong_tasks(sysenter
				  ? cold ? sysenter_ping_short_asm_cold_thread
				         : sysenter_ping_short_asm_thread
				  : cold ? int30_ping_short_asm_cold_thread
					 : int30_ping_short_asm_thread,
				sysenter ? sysenter_pong_short_asm_thread
					 : int30_pong_short_asm_thread);

	  /* wait for measurement to be finished, then kill tasks */
	  recv_ping_timeout(timeout_10s);
	  kill_pingpong_tasks();
	}
    }
}

static void
test_rdtsc(void)
{
#define TEST_RDTSC_NUM 10000
  int i;
  l4_cpu_time_t start,end,mid;
  l4_uint32_t diff,min1,min2,min3,max1,max2,max3;
  l4_cpu_time_t total1,total2,total3;

  total1 = total2 = total3 = 0;
  min1 = min2 = min3 = -1;
  max1 = max2 = max3 = 0;

  for (i = 0; i < TEST_RDTSC_NUM; i++)
    {
      start = l4_rdtsc();
      mid   = l4_rdtsc();
      end   = l4_rdtsc();
      
      diff = (l4_uint32_t)(mid - start);
      total1 += diff;

      if (diff < min1)
	min1 = diff;
      if (diff > max1)
	max1 = diff;

      diff = (l4_uint32_t)(end - mid);
      total2 += diff;

      if (diff < min2)
	min2 = diff;
      if (diff > max2)
	max2 = diff;

      diff = (l4_uint32_t)(end - start);
      total3 += diff;

      if (diff < min3)
	min3 = diff;
      if (diff > max3)
	max3 = diff;
      
    }
  
  printf("Rdtsc impact: 1-2: %u/%u/%u 2-3: %u/%u/%u, 1-3: %u/%u/%u\n",
       min1,(l4_uint32_t)(total1 / TEST_RDTSC_NUM),max1,
       min2,(l4_uint32_t)(total2 / TEST_RDTSC_NUM),max2,
       min3,(l4_uint32_t)(total3 / TEST_RDTSC_NUM),max3);
}

#ifndef FIASCO_UX
// determine cpu frequency
static void
test_cpu(void)
{
  l4_uint32_t hz;

  l4_calibrate_tsc();
  hz       = l4_get_hz();
  mhz      = hz / 1000000;
  timer_hz = hz / timer_hz;

  printf("CPU frequency is %dMhz, L4 Timer frequency is %dHz\n", mhz, timer_hz);
}
#endif

/** Show measurement explanations */
static void
help(void)
{
  printf(
"\n"
" Test description\n"
"  0: One thread (client) calls another thread (server) in the same address\n"
"     space with ipc timeout NEVER. The server thread replies a short message\n"
"     with send timeout zero. This test usually examines the fastest ipc for\n"
"     L4 v2/x0 kernels. We measure the round-trip time.\n"
"  1: Same as test 0 except that both threads reside in different address\n"
"     spaces. Therefore the test shows additional costs induced by context\n"
"     switches (two per round-trip) and TLB reloading overhead.\n"
"  2: The client thread calls the server thread which resides in a different\n"
"     address space with ipc timeout NEVER. The server replies a long\n"
"     message which is copied to the client (direct string copy). The test\n"
"     is performed for different string lengths.\n"
"  3: Same as test 2 except that the server replies indirect strings.\n"
"  4: Same as test 1 except that the server replies one short flexpage.\n"
"  5: Same as test 2 except that the server replies multiple flexpages\n"
"     (1024 per message, each fpage has a size of L4_PAGESIZE). Note that\n"
"     transferring multiple flexpages is not implemented in L4Ka/Hazelnut.\n"
"  6: The client raises pagefaults which are handled by the server residing\n"
"     in a different address space. This test differs from test 3 because\n"
"     the client enters the kernel raising exception 0xe (pagefault) -- and\n"
"     not through the system call interface.\n"
"  7: A thread raises exceptions 0x0d. The user level exception handler\n"
"     returns immediately behind the faulting instruction.\n"
"  8: Same as test 1 except we use the standard C-bindings.\n"
#ifndef FIASCO_UX
"\n"
"  Press any key to continue ..."
#endif
      );
#ifndef FIASCO_UX
  getchar();
#endif
  printf(
"\n\n"
"  9: Same as test 1 except we use static functions implemented in Assembler\n"
"     for performing ipc.\n"
      );
}

extern void unmap_test(void);

/** Main */
int
main(int argc, const char **argv)
{
#ifndef FIASCO_UX
  int c;
#endif
  int i;
  int loop = 0;
  typedef struct
    {
      const char *name;
      void (*func)(int nr);
      int enabled;
    } test_t;
  static test_t test[] =
    {
	{ "short IPC intra address space  ", benchmark_short_intraAS,     1 },
	{ "short IPC inter address space  ", benchmark_short_interAS,     1 },
	{ "long  IPC inter address space  ", benchmark_long_interAS,      1 },
	{ "indrct IPC inter address space ", benchmark_indirect_interAS,  1 },
	{ "short fpage inter address space", benchmark_shortMap_interAS,  1 },
	{ "long  fpage inter address space", benchmark_longMap_interAS,   1 },
	{ "pagefault inter address space  ", benchmark_pagefault_interAS, 1 },
	{ "exception intra address space  ", benchmark_exception_intraAS, 1 },
	{ "short IPC inter (c-inline-bind)", benchmark_short_c_interAS,   1 },
	{ "short IPC inter (external asm) ", benchmark_short_asm_interAS, 1 }
    };
#define MENU_ENTRIES (sizeof(test)/sizeof(test[0]))
#ifndef FIASCO_UX
  static const char *mentry_disabled =
          "(disabled)                     ";
#endif

  i = parse_cmdline(&argc, &argv,
		    'l', "loop", "repeat all tests forever",
		      PARSE_CMD_SWITCH, 1, &loop,
		    0);
  switch (i)
    {
    case -1:
      puts("parse_cmdline descriptor error");
      exit(-1);
    case -2:
      puts("out of stack in parse_cmdline()");
      exit(-2);
    case -3:
      puts("unknown command line option");
      exit(-3);
    case -4:
      puts("help requested");
      exit(-4);
    }

  /* init librmgr */
  if (!rmgr_init())
    {
      puts("pingpong: RMGR not found!");
      return -1;
    }

  detect_kernel();
  detect_sysenter();

  printf("Sysenter %s\n",
        dont_do_sysenter ? "disabled/not available" : "enabled");

  test_rdtsc();

#ifndef FIASCO_UX
  test_cpu();
#endif

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  /* start pager */
  main_id = pager_id = memcpy_id = l4_myself();
  pager_id.id.lthread = PAGER_THREAD;
  memcpy_id.id.lthread = MEMCPY_THREAD;
  create_thread(pager_id,(l4_umword_t)pager,(l4_umword_t)pager_stack+STACKSIZE,
		rmgr_pager_id);

  intra_ping = intra_pong = inter_ping = inter_pong = main_id;
  intra_ping.id.lthread  = PING_THREAD;
  intra_pong.id.lthread += PONG_THREAD;
  inter_ping.id.task    += 1;
  inter_ping.id.lthread  = 0;
  inter_pong.id.task    += 2;
  inter_pong.id.lthread  = 0;

  /* allocate scratch memory for sending flexpages */
  scratch_mem = rmgr_reserve_mem(SCRATCH_MEM_SIZE, 
				 L4_LOG2_SUPERPAGESIZE, 0, 0, 0);
  printf("%d MB scratch memory ", SCRATCH_MEM_SIZE/(1024*1024));
  if (scratch_mem == -1)
    {
      puts("not reserved -- disabling fpage tests");
      dont_do_cold = 1;
    }
  else
    {
      printf("at %08x-%08x reserved\n",
	     scratch_mem, scratch_mem+SCRATCH_MEM_SIZE);
      map_scratch_mem_from_rmgr();
      if (!dont_do_cold)
	test_flooder();
    }

  if (scratch_mem==-1)
    test[4].enabled = test[5].enabled = test[6].enabled = 0;
  if (dont_do_multiple_fpages)
    test[5].enabled = 0;
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
  test[9].enabled = 0;
#endif

#ifdef FIASCO_UX

  help();
  for (i=0; i<MENU_ENTRIES; i++)
    {
      if (test[i].enabled)
	test[i].func(i);
    }

  putchar('\n');
  enter_kdebug("*#^Done");

#else

  for (;;)
    {
      putchar('\n');
      for (i=0; i<MENU_ENTRIES; i++)
	{
	  printf("   %d: %s", 
	      i, test[i].enabled ? test[i].name : mentry_disabled);
	  if (i % 2)
	    putchar('\n');
	};
      printf("   a: all  x: boot  h: help  k: kdbg ");
      if (scratch_mem != -1)
	{
	  if (i % 2)
	    putchar('\n');
	  i++;
	  printf("   m: memory test");
	}
      if (i % 2)
	putchar('\n');
      i++;
      printf("   s: change small spaces (now: %u-%u)", 
	         smas_pos[0], smas_pos[1] );
      putchar('\n');

invalid_key:
      switch(c = loop ? 'a' : getchar())
	{
	case 'x': 
	  puts("\nRebooting..."); pc_reset(); 
	  break;
	case 'h': 
	  help(); 
	  break;
	case 'a':
	  for (i=0; i<MENU_ENTRIES; i++)
	    {
	      if (test[i].enabled)
		{
		  putchar('\n');
		  test[i].func(i);
		}
	    }
	  break;
	case 'm':
	  test_mem_bandwidth();
	  break;
	case 'k':
	  putchar('\n');
	  enter_kdebug("stop");
	  break;
	case 's' :
	  printf("\n"
	         ">> 0: (0-0)  1: (0-2)  2: (1-0)  3:(1-2)");
	  switch (getchar())
	    {
	    case '0': smas_pos[0] = 0; smas_pos[1] = 0; break;
	    case '1': smas_pos[0] = 0; smas_pos[1] = 2; break;
	    case '2': smas_pos[0] = 1; smas_pos[1] = 0; break;
	    case '3': smas_pos[0] = 1; smas_pos[1] = 2; break;
	    }
	  printf("\n"
	         "   set to %u-%u\n", smas_pos[0], smas_pos[1]);
	  break;
	default:
	  if ((c-'0' < MENU_ENTRIES) && (test[c-'0'].enabled))
	    {
	      putchar('\n');
	      test[c-'0'].func(c-'0');
	      break;
	    }
	  else
	    goto invalid_key;
	}
    }

#endif

  /* done */
  return 0;
}

