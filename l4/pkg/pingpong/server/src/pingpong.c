/* $Id$ */
/*****************************************************************************/
/**
 * \file   pingpong/server/src/pingpong.c
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \brief  Pingpong IPC benchmark.
 */
/*****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdlib.h>

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kernel.h>
#include <l4/sys/ktrace.h>
#include <l4/sigma0/kip.h>
#include <l4/rmgr/librmgr.h>
#include <l4/util/util.h>
#include <l4/util/rdtsc.h>
#include <l4/util/parse_cmd.h>
#include <l4/util/bitops.h>
#include <l4/util/reboot.h>
#include <l4/util/l4_macros.h>
#include <l4/util/port_io.h>
#include <l4/util/irq.h>
#include <l4/sigma0/sigma0.h>
#ifdef USE_DIETLIBC
#include <l4/env_support/getchar.h>
#endif

#include "global.h"
#include "pingpong.h"
#include "worker.h"
#include "helper.h"

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
l4_umword_t global_rounds = 12500;
l4_uint32_t rdpmc0;
l4_uint32_t mhz;
l4_uint32_t timer_hz;
l4_uint32_t timer_resolution;
l4_kernel_info_t *kip;
int use_superpages;
int callmode;
int deceit;
int cold;
int ux_running;
int fiasco_running;

static int dont_do_sysenter;
static int dont_do_kipcalls;
static int dont_do_deceit;
static int dont_do_superpages;
static int dont_do_multiple_fpages;
static int dont_do_ipc_asm;
       int dont_do_cold;
static int smas_pos[2] = {0,0};

static unsigned char pager_stack[STACKSIZE] __attribute__((aligned(4096)));
static unsigned char ping_stack[STACKSIZE] __attribute__((aligned(4096)));
       unsigned char pong_stack[STACKSIZE] __attribute__((aligned(2048)));

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
		:"b" (&kip->clock)
		:"edx");
}

/** Guess kernel by looking at kernel info page version number. */
static void
detect_kernel(void)
{
  if (!(kip = l4sigma0_kip_map(L4_INVALID_ID)))
    {
      printf("failed to map kernel info page\n");
      return;
    }

  printf("Kernel version %08x: ", kip->version);
  switch (kip->version)
    {
    case 21000:
      puts("Jochen LN -- disabling 4MB mappings");
      dont_do_superpages = 1;
      dont_do_sysenter = 1;
      dont_do_kipcalls = 1;
      dont_do_deceit = 1;
      break;
    case 0x0004711:
      puts("L4Ka Hazelnut -- disabling multiple flexpages");
      dont_do_multiple_fpages = 1;
      dont_do_ipc_asm = 1;
      dont_do_deceit = 1;
      dont_do_kipcalls = 1;
      break;
    case L4SIGMA0_KIP_VERSION_FIASCO:
      if (strstr(l4sigma0_kip_version_string(), "(ux)"))
	{
	  puts("Fiasco-UX");
	  ux_running = 1;
	  global_rounds /= 10;
	}
      else
	puts("Fiasco");
      fiasco_running = 1;
      break;
    default:
      puts("Unknown");
      break;
    }
  if (ux_running)
    {
      dont_do_sysenter = 1;
      dont_do_cold = 1;
    }
  else
    {
      test_timer_frequency();
    }
}

/** Determine if cpu does not support sysenter. */
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

/** Determine if kernel supports KIP calls. */
static void
detect_kipcalls(void)
{
  if (!dont_do_kipcalls)
    dont_do_kipcalls = !l4sigma0_kip_kernel_has_feature("kip_syscalls");
}

void do_sleep(void);
extern void asm_do_sleep(void);

/** "shutdown" thread. */
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

/* keymap without shift */
static const unsigned char keymap[] =
{    0,  27, '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '-', '=',   8,
  '\t', 'q', 'w', 'e', 'r', 't', 'y', 'u', 'i', 'o', 'p', '[', ']', '\r',
     0, 'a', 's', 'd', 'f', 'g', 'h', 'j', 'k', 'l', ';', '\'', '`',
    0, '\\', 'z', 'x', 'c', 'v', 'b', 'n', 'm', ',', '.', '/',
};

/* getchar for both kdb and keyboard */
static int getchar_multi(void)
{
  int ch;
  unsigned long flags;
  l4util_flags_save(&flags);

  while (1)
    {
      if (!ux_running && l4util_in8(0x64) & 0x01)
        {
          /* keyboard character available */
          int scancode = l4util_in8(0x60);
          if (scancode & 0x80) /* key release - drop */
            continue;
          scancode &= 0x7f;
          ch = 0;
          if (scancode >= sizeof(keymap)/sizeof(keymap[0]))
            goto out;
          ch = keymap[scancode];
          goto out;
        }

      if ((ch = l4kd_inchar()) != -1)
        goto out;

      l4_sleep(50);
    }

out:
  l4util_flags_restore(&flags);
  return ch;
}

/** print name of test. */
static void
print_testname(const char *test, int nr, enum test_type type, int show_sysenter)
{
  if (ux_running)
    show_sysenter = 0;

  switch (type)
    {
    case INTER:
      printf(">> %c: %s: "l4util_idfmt" <==> "l4util_idfmt"%s\n",
	     nr, test, l4util_idstr(inter_ping), l4util_idstr(inter_pong),
	     show_sysenter ? (callmode == 1) ? " (sysenter)" : 
	                     (callmode == 2) ? " (kipcalls)" : " (int30)"
	                   : "");
      break;
    case INTRA:
      printf(">> %c: %s: "l4util_idfmt" <==> "l4util_idfmt"%s\n",
             nr, test, l4util_idstr(intra_ping), l4util_idstr(intra_pong),
             show_sysenter ? (callmode == 1) ? " (sysenter)" : 
	                     (callmode == 2) ? " (kipcalls)" : " (int30)"
                           : "");
      break;
    case SINGLE:
      printf(">> %c: %s: "l4util_idfmt"\n", nr, test, l4util_idstr(intra_ping));
      break;
    }
}

static void
move_to_small_space(l4_threadid_t id, int nr)
{
  l4_threadid_t foo;
  l4_sched_param_t sched;

  foo = L4_INVALID_ID;
  l4_thread_schedule(id, L4_INVALID_SCHED_PARAM, &foo, &foo, &sched);
  sched.sp.small = L4_SMALL_SPACE(SMAS_SIZE, nr);
  foo = L4_INVALID_ID;
  l4_thread_schedule(id, sched, &foo, &foo, &sched);
}


/** Create thread.
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

  /* get our preempter/pager */
  preempter = pager = L4_INVALID_ID;
  l4_thread_ex_regs_flags(main_id,(l4_umword_t)-1,(l4_umword_t)-1,&preempter,
                          &pager,&dummy,&dummy,&dummy,
			  L4_THREAD_EX_REGS_NO_CANCEL);

  /* create thread */
  l4_thread_ex_regs(id,eip,esp,&preempter,&new_pager,
		    &dummy,&dummy,&dummy);

}


/** create ping and pong threads. */
static void
create_ping_thread(void (*ping_thread)(void))
{
  create_thread(ping_id, (l4_umword_t)ping_thread,
			 (l4_umword_t)ping_stack + STACKSIZE, pager_id);
}

static void
create_pong_thread(void (*pong_thread)(void))
{
  create_thread(pong_id, (l4_umword_t)pong_thread,
			 (l4_umword_t)pong_stack + STACKSIZE, pager_id);
}

static void
create_pingpong_threads(void (*ping_thread)(void),
                        void (*pong_thread)(void))
{
  create_ping_thread(ping_thread);
  create_pong_thread(pong_thread);
}

/** create ping and pong tasks. */
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
      printf("failed to create pong task "l4util_idtskfmt"\n",
	  l4util_idtskstr(pong_id));
      return;
    }

  t = rmgr_task_new(ping_id,255,(l4_umword_t)ping_stack + STACKSIZE,
		    (l4_umword_t)ping_thread,pager_id);
  if (l4_is_nil_id(t) || l4_is_invalid_id(t))
    {
      printf("failed to create ping task "l4util_idtskfmt"\n",
	  l4util_idtskstr(ping_id));
      rmgr_task_new(pong_id,rmgr_id.lh.low,0,0,L4_NIL_ID);
      return;
    }

  move_to_small_space(ping_id, smas_pos[0]);
  move_to_small_space(pong_id, smas_pos[1]);

  /* shake hands with pong to ensure pong that ping is already created */
  recv(pong_id);
  send(pong_id);
}

/** Kill pong thread. */
static void
kill_pong_thread(void)
{
  create_thread(pong_id,(l4_umword_t)asm_do_sleep,
                (l4_umword_t)pong_stack + STACKSIZE, pager_id);
}

/** kill ping and pong tasks. */
static void
kill_pingpong_tasks(void)
{
  /* delete ping and pong tasks */
  rmgr_task_new(ping_id,rmgr_id.lh.low,0,0,L4_NIL_ID);
  rmgr_task_new(pong_id,rmgr_id.lh.low,0,0,L4_NIL_ID);
}

/** map 4k page from "from" to addr. */
static void
map_4k_page(l4_threadid_t from, l4_umword_t addr)
{
  switch (l4sigma0_map_mem(from, addr, addr, L4_PAGESIZE))
    {
    case -2:
      printf("IPC error mapping 4KB page at %08lx from "l4util_idfmt"\n",
	     addr, l4util_idstr(from));
      enter_kdebug("map_4k_page");
      break;
    case -3:
      printf("Can't map 4KB page at %08lx from "l4util_idfmt"\n",
	     addr, l4util_idstr(from));
      rmgr_dump_mem();
      enter_kdebug("map_4k_page");
      break;
    }
}

/** map 4M page from "from" to addr. */
static void
map_4m_page(l4_threadid_t from, l4_umword_t addr)
{
  switch (l4sigma0_map_mem(from, addr, addr, L4_SUPERPAGESIZE))
    {
    case -2:
      printf("IPC error mapping 4MB page at %08lx from "l4util_idfmt"\n",
	     addr, l4util_idstr(from));
      enter_kdebug("map_4M_page");
      break;
    case -3:
      printf("Can't map 4MB page at %08lx from "l4util_idfmt"\n",
	     addr, l4util_idstr(from));
      rmgr_dump_mem();
      enter_kdebug("map_4M_page");
      break;
    }
}

/** Map scratch memory from RMGR as 4M pages. */
static void
map_scratch_mem_from_rmgr(void)
{
  l4_umword_t a;

  for (a=scratch_mem; a<scratch_mem+SCRATCH_MEM_SIZE; a+=L4_SUPERPAGESIZE)
    map_4m_page(rmgr_pager_id, a);
}

/** map scratch memory from page into current task. */
void
map_scratch_mem_from_pager(void)
{
  l4_umword_t a;
  l4_umword_t inc_size = use_superpages ? L4_SUPERPAGESIZE : L4_PAGESIZE;

  for (a = scratch_mem; a < scratch_mem + SCRATCH_MEM_SIZE; a += inc_size)
    {
      if (use_superpages)
	map_4m_page(pager_id, a);
      else
	map_4k_page(pager_id, a);
    }
}

/** Pingpong thread pager. */
static void __attribute__((noreturn))
pager(void)
{
  l4_threadid_t src;
  l4_umword_t pfa,eip,snd_base,fp;
  l4_msgdope_t result;
  extern char _start;
  extern char _end;

  while (1)
    {
      l4_ipc_wait(&src, L4_IPC_SHORT_MSG, &pfa, &eip, L4_IPC_NEVER, &result);
      while (1)
	{
          int fn = pfa & SIGMA0_REQ_ID_MASK;

	  if (SIGMA0_IS_MAGIC_REQ(pfa) && fn == SIGMA0_REQ_ID_FPAGE_RAM)
	    {
              l4_fpage_t fpage = { .raw = eip };
              pfa = fpage.fp.page << L4_PAGESHIFT;

	      if (use_superpages &&
                  pfa >= scratch_mem &&
                  pfa <= scratch_mem + SCRATCH_MEM_SIZE)
                {
                  snd_base = (fpage.fp.page << L4_PAGESHIFT) & L4_SUPERPAGEMASK;
	          fp = snd_base | (L4_LOG2_SUPERPAGESIZE << 2);
                }
              else
                {
                  snd_base = fpage.fp.page << L4_PAGESHIFT;
	          fp = snd_base | (L4_LOG2_PAGESIZE << 2);
                }
              fp |= 2;
	    }
          else
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
            }

	  /* sanity check */
          if (   (pfa < (unsigned)&_start || pfa > (unsigned)&_end)
	      && (pfa < scratch_mem || pfa > scratch_mem+SCRATCH_MEM_SIZE))
		{
		  printf("Pager: pfa=%08lx eip=%08lx from "
			  l4util_idfmt" received\n",
			  pfa, eip, l4util_idstr(src));
		  enter_kdebug("stop");
		}
	  if (pfa & 2)
	    fp |= 2;

	  l4_ipc_reply_and_wait(src,L4_IPC_SHORT_FPAGE,snd_base,fp,
				     &src,L4_IPC_SHORT_MSG,&pfa,&eip,
			    	     L4_IPC_NEVER,&result);
	  if (L4_IPC_IS_ERROR(result))
	    {
	      printf("pager: IPC error (dope=0x%08lx) "
		     "serving pfa=%08lx eip=%08lx from "l4util_idfmt"\n",
		     result.msgdope, pfa, eip, l4util_idstr(src));
	      break;
	    }
	}
    }
}

/** IPC benchmark - short INTRA-AS. */
static void
bench_short_intraAS(int nr)
{
  /* start ping and pong thread */
  print_testname("short intra IPC", nr, INTRA, 0);

  ping_id = intra_ping;
  pong_id = intra_pong;

  printf("  === Shortcut (Client=Timeout(NEVER), Server=Timeout(0)) ===\n");
  for (cold=0; cold<2-dont_do_cold; cold++)
    {
      for (callmode=0; callmode<3; callmode++)
	{
	  if (callmode == 1 && dont_do_sysenter)
	      continue;
	  if (callmode == 2 && dont_do_kipcalls)
	      continue;

	  BENCH_BEGIN;

	  if (callmode == 0)
	      create_pingpong_threads(cold ?
		  int30_ping_short_cold_thread :
		  int30_ping_short_thread,
		  int30_pong_short_thread);
	  else if (callmode == 1)
	      create_pingpong_threads(cold ?
		  sysenter_ping_short_cold_thread :
		  sysenter_ping_short_thread,
		  sysenter_pong_short_thread);
	  else if (callmode == 2)
	      create_pingpong_threads(cold ? 
		  kipcalls_ping_short_cold_thread :
		  kipcalls_ping_short_thread, 
		  kipcalls_pong_short_thread);

	  move_to_small_space(ping_id, smas_pos[0]);

	  recv(pong_id);
	  send(pong_id);

	  /* wait for measurement to be finished */
	  recv_ping_timeout(timeout_50s);

	  /* shutdown pong thread, ping thread already sleeps */
          kill_pong_thread();

	  BENCH_END;

	  /* give ping thread time to go to bed */
	  send(ping_id);

	  /* give pong thread time to go to bed */
	  l4_thread_switch(L4_NIL_ID);

	  move_to_small_space(ping_id, 0);
	}
    }

  printf("  === No Shortcut (Receive Timeout 1s) ===\n");
  for (cold=0; cold<2-dont_do_cold; cold++)
    {
      for (callmode=0; callmode<3; callmode++)
	{
	  if (callmode == 1 && dont_do_sysenter)
	      continue;
	  if (callmode == 2 && dont_do_kipcalls)
	      continue;

	  BENCH_BEGIN;

	  if (callmode == 0)
	      create_pingpong_threads(cold ? 
		  int30_ping_short_to_cold_thread :
		  int30_ping_short_to_thread, 
		  int30_pong_short_to_thread);
	  else if (callmode == 1)
	      create_pingpong_threads(cold ? 
		  sysenter_ping_short_to_cold_thread :
		  sysenter_ping_short_to_thread, 
		  sysenter_pong_short_to_thread);
	  else if (callmode == 2)
	      create_pingpong_threads(cold ? 
		  kipcalls_ping_short_to_cold_thread :
		  kipcalls_ping_short_to_thread, 
		  kipcalls_pong_short_to_thread);

	  move_to_small_space(ping_id, smas_pos[0]);

	  recv(pong_id);
	  send(pong_id);
    
	  /* wait for measurement to be finished */
	  recv_ping_timeout(timeout_50s);

	  /* shutdown pong thread, ping thread already sleeps */
          kill_pong_thread();

	  BENCH_END;

	  /* give ping thread time to go to bed */
	  send(ping_id);

	  /* give pong thread time to go to bed */
	  l4_thread_switch(L4_NIL_ID);

	  move_to_small_space(ping_id, 0);
	}
    }
}

/** IPC benchmark - short INTER-AS. */
static void
bench_short_interAS(int nr)
{
  print_testname("short inter IPC", nr, INTER, 0);

  printf("  === Shortcut (Client=Timeout(NEVER), Server=Timeout(0)) ===\n");
  for (cold=0; cold<2-dont_do_cold; cold++)
    {
      for (callmode=0; callmode<3; callmode++)
	{
	  if (callmode == 1 && dont_do_sysenter)
	      continue;
	  if (callmode == 2 && dont_do_kipcalls)
	      continue;

	  BENCH_BEGIN;
	  if (callmode == 0)
	      create_pingpong_tasks(cold ? 
		  int30_ping_short_cold_thread :
		  int30_ping_short_thread, 
		  int30_pong_short_thread);
	  else if (callmode == 1)
	      create_pingpong_tasks(cold ? 
		  sysenter_ping_short_cold_thread :
		  sysenter_ping_short_thread, 
		  sysenter_pong_short_thread);
	  else if (callmode == 2)
	      create_pingpong_tasks(cold ? 
		  kipcalls_ping_short_cold_thread :
		  kipcalls_ping_short_thread, 
		  kipcalls_pong_short_thread);
	  recv_ping_timeout(timeout_50s);
	  kill_pingpong_tasks();
	  BENCH_END;
	}
    }

  printf("  === No Shortcut (Receive Timeout 1s) ===\n");
  for (cold=0; cold<2-dont_do_cold; cold++)
    {
      for (callmode=0; callmode<3; callmode++)
	{
	  if (callmode == 1 && dont_do_sysenter)
	      continue;
	  if (callmode == 2 && dont_do_kipcalls)
	      continue;

	  BENCH_BEGIN;
	  if (callmode == 0)
	      create_pingpong_tasks(cold ? 
		  int30_ping_short_cold_thread :
		  int30_ping_short_thread, 
		  int30_pong_short_thread);
	  else if (callmode == 1)
	      create_pingpong_tasks(cold ? 
		  sysenter_ping_short_cold_thread :
		  sysenter_ping_short_thread, 
		  sysenter_pong_short_thread);
	  else if (callmode == 2)
	      create_pingpong_tasks(cold ? 
		  kipcalls_ping_short_cold_thread :
		  kipcalls_ping_short_thread, 
		  kipcalls_pong_short_thread);
	  recv_ping_timeout(timeout_50s);
	  kill_pingpong_tasks();
	  BENCH_END;
	}
    }
}

/** IPC benchmark - short INTER-AS. */
static void
test_short_interAS_flood(int nr)
{
  print_testname("short inter IPC", nr, INTER, 0);

  printf("  === Shortcut (Client=Timeout(NEVER), Server=Timeout(0)) ===\n");
  BENCH_BEGIN;
  create_pingpong_tasks(sysenter_ping_short_cold_thread,
                        sysenter_pong_short_thread);

  /* wait for measurement to be finished, then kill tasks */
  recv_ping_timeout(timeout_50s);
  kill_pingpong_tasks();
  BENCH_END;

  printf("  === No Shortcut (Receive Timeout 1s) ===\n");
  BENCH_BEGIN;
  create_pingpong_tasks(sysenter_ping_short_to_cold_thread,
			sysenter_pong_short_to_thread);

  /* wait for measurement to be finished, then kill tasks */
  recv_ping_timeout(timeout_50s);
  kill_pingpong_tasks();
  BENCH_END;
}

/** IPC benchmark - short INTER-AS. */
static void
bench_short_dc_interAS(int nr)
{
  print_testname("short inter IPC / don't switch to receiver", nr, INTER, 0);
  printf("  Make sure to have enough kernel memory available, test will fail otherwise.\n");

  if (!l4sigma0_kip_kernel_has_feature("deceit_bit_disables_switch"))
    {
      printf("Kernel does not support deceit-bit-disables-switch, test skipped.\n");
      return;
    }

  ping_id = inter_ping;
  pong_id = inter_pong;

  for (deceit=0; deceit<2; deceit++)
    {
      for (callmode=0; callmode<3; callmode++)
	{
	  if (callmode == 1 && dont_do_sysenter)
	      continue;
	  if (callmode == 2 && dont_do_kipcalls)
	      continue;

	  l4_threadid_t t;
	  l4_threadid_t p;
	  void (*pong_thread)(void) = 0;
	  void (*ping_thread)(void) = 0;
	  if (deceit)
	  {
	      if (callmode == 0)
	      {
		  pong_thread = int30_pong_short_dc_thread;
		  ping_thread = int30_ping_short_dc_thread;
	      }
	      else if (callmode == 1)
	      {
		  pong_thread = sysenter_pong_short_dc_thread;
		  ping_thread = sysenter_ping_short_dc_thread;
	      }
	      else if (callmode == 2)
	      {
		  pong_thread = kipcalls_pong_short_dc_thread;
		  ping_thread = kipcalls_ping_short_dc_thread;
	      }
	  } else {
	      if (callmode == 0)
	      {
		  pong_thread = int30_pong_short_ndc_thread;
		  ping_thread = int30_ping_short_ndc_thread;
	      }
	      else if (callmode == 1)
	      {
		  pong_thread = sysenter_pong_short_ndc_thread;
		  ping_thread = sysenter_ping_short_ndc_thread;
	      }
	      else if (callmode == 2)
	      {
		  pong_thread = kipcalls_pong_short_ndc_thread;
		  ping_thread = kipcalls_ping_short_ndc_thread;
	      }
	  }
	  int ping_tasks=0, pong_tasks=0;
	  int i;

	  /* first create pong tasks */
	  for (i=0; i<200; i++)
	    {
	      p = pong_id;
	      p.id.task += i;
	      t = rmgr_task_new(p, 255,
				(l4_umword_t)scratch_mem + (i+1)*STACKSIZE,
				(l4_umword_t)pong_thread, pager_id);
	      if (l4_is_nil_id(t) || l4_is_invalid_id(t))
		{
		  printf("failed to create pong task "l4util_idtskfmt"\n",
		      l4util_idtskstr(p));
		  pong_tasks = i;
		  goto error;
		}
	      recv(p);
	    }
	  pong_tasks = i;

	  /* create ping task */
	  t = rmgr_task_new(ping_id, 255, (l4_umword_t)ping_stack + STACKSIZE,
	      (l4_umword_t)ping_thread, pager_id);
	  if (l4_is_nil_id(t) || l4_is_invalid_id(t))
	    {
	      printf("failed to create ping task "l4util_idtskfmt"\n",
		  l4util_idtskstr(ping_id));
	      goto error;
	    }
	  ping_tasks = 1;

	  recv(ping_id);
	  send(ping_id);

	  /* wait for measurement to be finished, then kill tasks */
	  recv_ping_timeout(timeout_50s);

	  /* ensure that all pong tasks got an IPC */
	  for (i=0; i<pong_tasks; i++)
	    {
	      p = pong_id;
	      p.id.task += i;
	      recv(p);
	    }
error:
	  for (i=0; i<pong_tasks; i++)
	    {
	      p = pong_id;
	      p.id.task += i;
	      rmgr_task_new(p, rmgr_id.lh.low, 0, 0, L4_NIL_ID);
	    }
	  for (i=0; i<ping_tasks; i++)
	    {
	      p = ping_id;
	      p.id.task += i;
	      rmgr_task_new(p, rmgr_id.lh.low, 0, 0, L4_NIL_ID);
	    }
	}
    }
}

/** IPC benchmark - long INTER-AS. */
static void
bench_long_interAS(int nr)
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

  /* don't care about kip-calls, because callmode directly is fastest */
  callmode = 1-dont_do_sysenter;
  print_testname("long inter IPC", nr, INTER, 1);

  for (cold=0; cold<2-dont_do_cold; cold++)
    {
      for (i=0; i<sizeof(strsizes)/sizeof(strsizes[0]); i++)
	{
	  strsize = strsizes[i].sz;
	  rounds  = strsizes[i].rounds;
	  if (ux_running)
	    rounds /= 10;

	  create_pingpong_tasks(callmode 
				   ? cold ? sysenter_ping_long_cold_thread
					  : sysenter_ping_long_thread
				   : cold ? int30_ping_long_cold_thread
					  : int30_ping_long_thread,
				callmode ? sysenter_pong_long_thread
					 : int30_pong_long_thread);

    	  /* wait for measurement to be finished, then kill tasks */
	  recv_ping_timeout(timeout_50s);
	  kill_pingpong_tasks();
	}
    }
}

/** IPC benchmark - long INTER-AS. */
static void
bench_indirect_interAS(int nr)
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

  /* don't care about kip-calls, because sysenter directly is fastest */
  callmode = 1-dont_do_sysenter;
  print_testname("indirect inter IPC", nr, INTER, 1);

  for (cold=0; cold<2-dont_do_cold; cold++)
    {
      for (i=0; i<sizeof(strsizes)/sizeof(strsizes[0]); i++)
	{
	  strsize = strsizes[i].sz;
	  strnum  = strsizes[i].num;
	  rounds  = strsizes[i].rounds;
	  if (ux_running)
	    rounds /= 10;

	  create_pingpong_tasks(callmode 
				   ? cold ? sysenter_ping_indirect_cold_thread
					  : sysenter_ping_indirect_thread
				   : cold ? int30_ping_indirect_cold_thread
					  : int30_ping_indirect_thread,
				callmode ? sysenter_pong_indirect_thread
					 : int30_pong_indirect_thread);
	  /* wait for measurement to be finished, then kill tasks */
	  recv_ping_timeout(timeout_50s);
	  kill_pingpong_tasks();
	}
    }
}

/** IPC benchmark - Mapping test. */
static void
bench_shortMap_test(void)
{
#ifdef LOW_MEM
  static int fpagesizes[] = { 1, 2, 4, 16, 64, 256, 512 };
#else
  static int fpagesizes[] = { 1, 2, 4, 16, 64, 256, 1024 };
#endif
  int i;

  /* don't care about kip-calls, because sysenter directly is fastest */
  callmode = 1-dont_do_sysenter;
  for (i=0; i<sizeof(fpagesizes)/sizeof(fpagesizes[0]); i++)
    {
      fpagesize = fpagesizes[i]*L4_PAGESIZE;
      rounds    = SCRATCH_MEM_SIZE/(fpagesize*8);

      BENCH_BEGIN;
      create_pingpong_tasks(callmode ? sysenter_ping_fpage_thread
				     : int30_ping_fpage_thread, 
			    callmode ? sysenter_pong_fpage_thread
				     : int30_pong_fpage_thread);

      /* wait for measurement to be finished, then kill tasks */
      recv_ping_timeout(timeout_50s);
      kill_pingpong_tasks();
      BENCH_END;
    }
}

/** IPC benchmark - Mapping. */
static void
bench_shortMap_interAS(int nr)
{
  use_superpages = 0;
  /* don't care about kip-calls, because sysenter directly is fastest */
  callmode = 1-dont_do_sysenter;
  print_testname("short fpage map from 4k", nr, INTER, 1);

  bench_shortMap_test();

  if (!dont_do_superpages)
    {
      use_superpages = 1;
      print_testname("short fpage map from 4M", nr, INTER, 1);
      bench_shortMap_test();
    }
  else
    puts("INTER-AS-SHORT-FP disabled from 4M because Jochen LN detected");
}

/** IPC benchmark - long fpages. */
static void
bench_longMap_interAS(int nr)
{
  rounds = SCRATCH_MEM_SIZE/(1024*L4_PAGESIZE);
  if (rounds > NR_MSG)
    rounds = NR_MSG;

  /* don't care about kip-calls, because sysenter directly is fastest */
  callmode = 1-dont_do_sysenter;
  print_testname("long fpage map", nr, INTER, 1);
  for (cold=0; cold<2-dont_do_cold; cold++)
    {
      create_pingpong_tasks(callmode 
				? cold ? sysenter_ping_long_fpage_cold_thread
				       : sysenter_ping_long_fpage_thread
				: cold ? int30_ping_long_fpage_cold_thread
				       : int30_ping_long_fpage_thread,
			    callmode ? sysenter_pong_long_fpage_thread
				     : int30_pong_long_fpage_thread);

      /* wait for measurement to be finished, then kill tasks */
      recv_ping_timeout(timeout_50s);
      kill_pingpong_tasks();
    }
}

/** IPC benchmark - pagefault test. */
static void
bench_pagefault_test(void)
{
  rounds = SCRATCH_MEM_SIZE/(fpagesize*8);
  /* don't care about kip-calls, because sysenter directly is fastest */
  callmode = 1-dont_do_sysenter;
  BENCH_BEGIN;
  create_pingpong_tasks(callmode ? sysenter_ping_pagefault_thread
				 : int30_ping_pagefault_thread,
			callmode ? sysenter_pong_pagefault_thread
				 : int30_pong_pagefault_thread);
  recv_ping_timeout(timeout_50s);
  kill_pingpong_tasks();
  BENCH_END;
}

/** IPC benchmark - pagefaults. */
static void
bench_pagefault_interAS(int nr)
{
  /* don't care about kip-calls, because sysenter directly is fastest */
  callmode = 1-dont_do_sysenter;
  print_testname("pagefault inter address space", nr, INTER, 1);

  fpagesize = L4_PAGESIZE;
  use_superpages = 0;
  bench_pagefault_test();

  if (!dont_do_superpages)
    {
      fpagesize = L4_PAGESIZE;
      use_superpages = 1;
      bench_pagefault_test();

      /* We need at least 32 MB for this test */
      if (SCRATCH_MEM_SIZE >= (32*1024*1024))
        {
          fpagesize = L4_SUPERPAGESIZE;
          use_superpages = 1;
          bench_pagefault_test();
	}

    }
  else
    puts("INTER-AS-PF disabled from 4M because Jochen LN detected");
}

static void
bench_exceptions(int nr)
{
  /* start ping and pong thread */
  callmode = 0;
  print_testname("intra reflection exceptions", nr, SINGLE, 0);

  ping_id = intra_ping;

  move_to_small_space(ping_id, smas_pos[0]);

  BENCH_BEGIN;
  create_ping_thread(ping_exception_intraAS_idt_thread);

  send(ping_id);
  recv(ping_id);

  /* wait for measurement to be finished */
  recv_ping_timeout(timeout_50s);
  /* give ping thread time to go to bed */
  send(ping_id);

  move_to_small_space(ping_id, 0);

  /* ----- */
  print_testname("inter reflection exceptions", nr, INTER, 0);
  ping_id = inter_ping;
  pong_id = inter_pong;

  create_pingpong_tasks(ping_exception_interAS_idt_thread,
                        exception_reflection_pong_handler);

  send(ping_id);
  recv(ping_id);

  recv_ping_timeout(timeout_50s);
  kill_pingpong_tasks();
  send(ping_id);
  BENCH_END;

  if (!l4sigma0_kip_kernel_has_feature("exception_ipc"))
    {
      printf("Kernel does not support exception IPC, "
             "remaining tests skipped.\n");
      return;
    }

  /* ------------------------------------------------------ */
  print_testname("intra IPC exception", nr, INTRA, 0);
  ping_id = intra_ping;
  pong_id = intra_pong;

  BENCH_BEGIN;

  create_thread(ping_id, (l4_umword_t)ping_exception_IPC_thread,
                (l4_umword_t)ping_stack + STACKSIZE, pong_id);
  create_pong_thread(exception_IPC_pong_handler);

  /* Hand-shake with thread to start */
  recv(pong_id);
  send(pong_id);
  recv(ping_id);
  send(ping_id);
  /* Wait to finish */
  recv_ping_timeout(timeout_50s);
  send(ping_id);
  kill_pong_thread();

  BENCH_END;

  /* ----- */
  print_testname("inter IPC exception", nr, INTER, 0);
  ping_id = inter_ping;
  pong_id = inter_pong;

  create_pingpong_tasks(ping_exception_IPC_thread,
                        exception_IPC_pong_handler);

  recv(ping_id);
  send(ping_id);

  recv_ping_timeout(timeout_50s);
  kill_pingpong_tasks();
  send(ping_id);
  BENCH_END;
}

/** IPC benchmark - short INTER-AS with c-bindings. */
static void
bench_short_casm_interAS(int nr)
{
  print_testname("short inter IPC (c-inline-bindings)", nr, INTER, 0);
  for (cold=0; cold<2-dont_do_cold; cold++)
    {
      for (callmode=0; callmode<3; callmode++)
	{
	  if (callmode == 1 && dont_do_sysenter)
	      continue;
	  if (callmode == 2 && dont_do_kipcalls)
	      continue;

	  BENCH_BEGIN;
	  if (callmode == 0)
	  {
	      create_pingpong_tasks(cold ?
		  int30_ping_short_c_cold_thread :
		  int30_ping_short_c_thread,
		  int30_pong_short_c_thread);
	  }
	  else if (callmode == 1)
	  {
	      create_pingpong_tasks(cold ?
		  sysenter_ping_short_c_cold_thread :
		  sysenter_ping_short_c_thread,
		  sysenter_pong_short_c_thread);
	  }
	  else if (callmode == 2)
	  {
	      create_pingpong_tasks(cold ?
		  kipcalls_ping_short_c_cold_thread :
		  kipcalls_ping_short_c_thread,
		  kipcalls_pong_short_c_thread);
	  }

	  /* wait for measurement to be finished, then kill tasks */
	  recv_ping_timeout(timeout_50s);
	  kill_pingpong_tasks();
	  BENCH_END;
	}
    }

  print_testname("short inter IPC (external assembler)", nr, INTER, 0);
  for (cold=0; cold<2-dont_do_cold; cold++)
    {
      for (callmode=0; callmode<3; callmode++)
	{
	  if (callmode == 1 && dont_do_sysenter)
	      continue;
	  if (callmode == 2 && dont_do_kipcalls)
	      continue;

	  BENCH_BEGIN;
	  if (callmode == 0)
	  {
	      create_pingpong_tasks(cold ?
		  int30_ping_short_asm_cold_thread :
		  int30_ping_short_asm_thread,
		  int30_pong_short_asm_thread);
	  }
	  else if (callmode == 1)
	  {
	      create_pingpong_tasks(cold ?
		  sysenter_ping_short_asm_cold_thread :
		  sysenter_ping_short_asm_thread,
		  sysenter_pong_short_asm_thread);
	  }
	  else if (callmode == 2)
	  {
	      create_pingpong_tasks(cold ?
		  kipcalls_ping_short_asm_cold_thread :
		  kipcalls_ping_short_asm_thread,
		  kipcalls_pong_short_asm_thread);
	  }

	  /* wait for measurement to be finished, then kill tasks */
	  recv_ping_timeout(timeout_50s);
	  kill_pingpong_tasks();
	  BENCH_END;
	}
    }
}

/** IPC benchmark - short INTER-AS with extern assembler functions. */
static void
bench_short_compare(int nr)
{
  callmode = 1;

  print_testname("short IPC intra-AS shortcut/no-shortcut", nr, INTRA, 1);
  ping_id = intra_ping;
  pong_id = intra_pong;

  BENCH_BEGIN;
  create_pingpong_threads(sysenter_ping_short_thread,
                          sysenter_pong_short_thread);
  recv(pong_id);
  send(pong_id);
  recv_ping_timeout(timeout_50s);
  kill_pong_thread();
  BENCH_END;

  send(ping_id);
  l4_thread_switch(L4_NIL_ID);

  BENCH_BEGIN;
  create_pingpong_threads(sysenter_ping_short_to_thread,
                          sysenter_pong_short_to_thread);
  recv(pong_id);
  send(pong_id);
  recv_ping_timeout(timeout_50s);
  kill_pong_thread();
  BENCH_END;

  send(ping_id);
  l4_thread_switch(L4_NIL_ID);

  print_testname("short IPC inter-AS shortcut/no-shortcut", nr, INTER, 1);

  BENCH_BEGIN;
  create_pingpong_tasks(sysenter_ping_short_thread,
			sysenter_pong_short_thread);
  recv_ping_timeout(timeout_50s);
  kill_pingpong_tasks();

  create_pingpong_tasks(sysenter_ping_short_to_thread,
			sysenter_pong_short_to_thread);
  recv_ping_timeout(timeout_50s);
  kill_pingpong_tasks();
  BENCH_END;
}

/* Simple intra and inter IPC using different UTCB sizes,
 * should probably integrated into other IPC tests with sysenter/int30
 * hot/old etc. */
static void
bench_utcb_ipc(int nr)
{
  print_testname("IPC with UTCB data transfer", nr, INTRA, 1);
  ping_id = intra_ping;
  pong_id = intra_pong;

  BENCH_BEGIN;
  create_pingpong_threads(ping_utcb_ipc_thread,
                          pong_utcb_ipc_thread);

  recv(pong_id);
  send(pong_id);
  recv(ping_id);
  send(ping_id);
  recv_ping_timeout(timeout_50s);

  kill_pong_thread();

  BENCH_END;

  print_testname("IPC with UTCB data transfer", nr, INTER, 1);
  ping_id = inter_ping;
  pong_id = inter_pong;

  BENCH_BEGIN;
  create_pingpong_tasks(ping_utcb_ipc_thread,
                        pong_utcb_ipc_thread);
  recv(ping_id);
  send(ping_id);
  recv_ping_timeout(timeout_50s);
  kill_pingpong_tasks();
  BENCH_END;
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

  BENCH_BEGIN;
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
  BENCH_END;

  printf("Rdtsc impact: 1-2: %u/%u/%u 2-3: %u/%u/%u, 1-3: %u/%u/%u\n",
       min1,(l4_uint32_t)(total1 / TEST_RDTSC_NUM),max1,
       min2,(l4_uint32_t)(total2 / TEST_RDTSC_NUM),max2,
       min3,(l4_uint32_t)(total3 / TEST_RDTSC_NUM),max3);
}

// determine cpu frequency
static void
test_cpu(void)
{
  l4_uint32_t hz;

  l4_tsc_init(0);
  hz       = l4_get_hz();
  mhz      = hz / 1000000;
  if (!ux_running)
    timer_hz = hz / timer_hz;

  printf("CPU frequency is %dMhz", mhz);
  if (!ux_running)
    printf(", L4 Timer frequency is %dHz", timer_hz);

  putchar('\n');
}

/** Show measurement explanations. */
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
"     A variation of this test is executed on Fiasco. Normal implementations\n"
"     of L4 contain an implicit switch to the ipc-receiver. This is not\n"
"     volitional in all cases. A special extension of Fiasco allows to send\n"
"     an ipc without that implicit switch. This feature can be enabled by\n"
"     setting the DECEIT bit of the send descriptor. In an appropriate\n"
"     scenario a sender would wake up several receivers using send-only ipcs.\n"
"     The scheduler has to deceide then which receiver runs next.\n"
"\n"
"  1: Same as test 0 except that both threads reside in different address\n"
"     spaces. Therefore the test shows additional costs induced by context\n"
"     switches (two per round-trip) and TLB reloading overhead.\n"
"\n"
"  2: The client thread calls the server thread which resides in a different\n"
"     address space with ipc timeout NEVER. The server replies a long\n"
"     message which is copied to the client (direct string copy). The test\n"
"     is performed for different string lengths.\n"
"\n"
"  3: Same as test 2 except that the server replies indirect strings.\n"
"\n"
"  Press any key to continue ...");
  fflush(NULL);
  getchar_multi();
  printf("\r"
"  4: Same as test 1 except that the server replies one short flexpage.\n"
"\n"
"  5: Same as test 2 except that the server replies multiple flexpages\n"
"     (1024 per message, each fpage has a size of L4_PAGESIZE). Note that\n"
"     transferring multiple flexpages is not implemented in L4Ka/Hazelnut.\n"
"\n"
"  6: The client raises pagefaults which are handled by the server residing\n"
"     in a different address space. This test differs from test 3 because\n"
"     the client enters the kernel raising exception 0xe (pagefault) -- and\n"
"     not through the system call interface.\n"
"\n"
"  7: A thread raises exceptions 0x0d. The user level exception handler\n"
"     returns immediately behind the faulting instruction.\n"
"\n"
"  8: Same as test 1 except we use the standard C-bindings or static\n"
"     functions implemented in Assembler\n"
"\n"
"  9: Compare sysenter IPC\n"
"\n"
"  d: Send once to 200 different Tasks without implicit switching to the\n"
"     receiver. Compared to call/receive+send.\n"
"  Press any key to continue ...");
  fflush(NULL);
  getchar_multi();
  printf("\r%40s", "");
}

extern void unmap_test(void);

static void __attribute__((noreturn))
my_exit(int errno)
{
  if (ux_running)
    l4util_reboot();

  exit(errno);
}

/** Main. */
int
main(int argc, const char **argv)
{
  int c, i, j, loop = 0, description = 0, test_nr = -1;
  typedef struct
    {
      const char *name;
      void (*func)(int nr);
      int enabled;
      char key;
    } test_t;
  static test_t test[] =
    {
      { "short IPC intra address space",    bench_short_intraAS,      1, '0' },
      { "short IPC inter address space",    bench_short_interAS,      1, '1' },
      { "long  IPC inter address space",    bench_long_interAS,       1, '2' },
      { "indrct IPC inter address space",   bench_indirect_interAS,   1, '3' },
      { "short fpage inter address space",  bench_shortMap_interAS,   1, '4' },
      { "long  fpage inter address space",  bench_longMap_interAS,    1, '5' },
      { "pagefault inter address space",    bench_pagefault_interAS,  1, '6' },
      { "exception handling methods",       bench_exceptions,         1, '7' },
      { "short IPC inter c-inl-/asm-bind",  bench_short_casm_interAS, 1, '8' },
      { "compare fast sysenter IPC",        bench_short_compare,      1, '9' },
      { "short IPC inter / don't switch",   bench_short_dc_interAS,   1, 'd' },
      { "IPC using UTCB",                   bench_utcb_ipc,           1, 'u' },
      { "measure specific instructions",    test_instruction_cycles,  1, 'c' },
//    { "measure costs of TLB/Cache",       test_cache_tlb,           1, 't' },
      { "memory bandwidth (memcpy)",        test_memory_bandwidth,    1, 'm' },
      { "short IPC inter AS, flooder",      test_short_interAS_flood, 1, 'i' },
    };

  const int mentries = sizeof(test)/sizeof(test[0]);

  i = parse_cmdline(&argc, &argv,
		    'c', "skipcold", "skip all cold tests",
		      PARSE_CMD_SWITCH, 1, &dont_do_cold,
		    'l', "loop", "repeat all tests forever",
		      PARSE_CMD_SWITCH, 1, &loop,
		    't', "test", "select specific test",
		      PARSE_CMD_INT, -1, &test_nr,
		    'd', "description", "give description about tests",
		      PARSE_CMD_SWITCH, 1, &description,
		    0);
  switch (i)
    {
    case -1: puts("parse_cmdline descriptor error");  my_exit(-1);
    case -2: puts("out of stack in parse_cmdline()"); my_exit(-2);
    case -3: puts("unknown command line option");     my_exit(-3);
    case -4:                                          my_exit(0);
    }

  if (description)
    {
      help();
      my_exit(0);
    }

  /* init librmgr */
  if (!rmgr_init())
    {
      puts("pingpong: RMGR not found!");
      return -1;
    }

  detect_kernel();
  detect_sysenter();
  detect_kipcalls();

  printf("Sysenter %s\n",
         dont_do_sysenter ? "disabled/not available" : "enabled");
  printf("Kipcall %s\n",
         dont_do_kipcalls ? "disabled/not available" : "enabled");

  test_rdtsc();
  test_cpu();

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  /* start pager */
  main_id = pager_id = memcpy_id = l4_myself();
  pager_id.id.lthread = PAGER_THREAD;
  memcpy_id.id.lthread = MEMCPY_THREAD;

  create_thread(pager_id,(l4_umword_t)pager,
		(l4_umword_t)pager_stack+STACKSIZE, rmgr_pager_id);

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
      printf("at %08lx-%08lx reserved\n",
	     scratch_mem, scratch_mem+SCRATCH_MEM_SIZE);
      map_scratch_mem_from_rmgr();
    }

  if (scratch_mem==-1)
    test[4].enabled = test[5].enabled = test[6].enabled = 0;
  if (dont_do_multiple_fpages)
    test[5].enabled = 0;
  if (dont_do_sysenter)
    test[9].enabled = 0;
  if (dont_do_deceit || scratch_mem == -1)
    test[10].enabled = 0;
  if (!l4sigma0_kip_kernel_has_feature("utcb"))
    test[11].enabled = 0;
  if (scratch_mem == -1)
    test[12].enabled = 0;

  for (;;)
    {
      for (i=0, j=0; i<mentries; i++)
	{
	  if (test[i].enabled)
	    {
	      if (j++ % 2 == 0)
		putchar('\n');
	      printf("   %c: %-32s", test[i].key, test[i].name);
	    }
	};
      if (!ux_running)
	{
	  if (j++ % 2 == 0)
    	    putchar('\n');
	  printf("   s: change small spaces (now: %u-%u)  ",
	      smas_pos[0], smas_pos[1] );
	}
      if (1)
	{
	  if (j++ % 2 == 0)
	    putchar('\n');
	  printf("   a: all  x: boot  h: help  k: kdbg  ");
	}
      putchar('\n');

invalid_key:
      if (test_nr == -1)
	c = loop ? 'a' : getchar_multi();
      else
	{
	  c = test_nr + '0';
	  test_nr = -1;
	}
      switch(c)
	{
	case 'x': 
	  puts("\nRebooting...");
	  l4util_reboot(); 
	  break;
	case 'h': 
	  help(); 
	  break;
	case 'a':
	  for (i=0; i<mentries; i++)
	    {
	      if (test[i].enabled)
		{
		  putchar('\n');
		  test[i].func(test[i].key);
		}
	    }
	  break;
	case 'H':
	  // undocumented ;-)
	  asm volatile ("cli; 1: jmp 1b");
	  break;
	case 'k':
	  putchar('\n');
	  enter_kdebug("stop");
	  break;
	case 's' :
	  if (!ux_running)
	    {
	      printf("\n"
	             ">> 0: (0-0)  1: (0-2)  2: (1-0)  3:(1-2)");
	      fflush(NULL);
	      switch (getchar_multi())
		{
		case '0': smas_pos[0] = 0; smas_pos[1] = 0; break;
		case '1': smas_pos[0] = 0; smas_pos[1] = 2; break;
		case '2': smas_pos[0] = 1; smas_pos[1] = 0; break;
		case '3': smas_pos[0] = 1; smas_pos[1] = 2; break;
		}
	      printf("\n"
		     "   set to %u-%u\n", smas_pos[0], smas_pos[1]);
	    }
	  break;
	default:
	  for (i=0; i<mentries; i++)
	    {
	      if (test[i].key == c && test[i].enabled)
		{
		  putchar('\n');
		  test[i].func(c);
		  break;
		}
	    }
	  if (i>=mentries)
	    goto invalid_key;
	}
    }
}
