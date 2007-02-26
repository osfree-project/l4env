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

#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/rmgr/librmgr.h>
#include <l4/util/util.h>
#include <l4/util/rdtsc.h>
#include <l4/util/parse_cmd.h>

#include "global.h"
#include "idt.h"

/*****************************************************************************
 *** configuration
 *****************************************************************************/

/* number of test rounds */
#ifdef FIASCO_UX
#define ROUNDS 1250
#else
#define ROUNDS 12500
#endif

/* which test */
#define CALL_ONLY    0

/* Don't enable this because it is unfair. We normally call the server
 * and the server replies by reply_and_wait using send timeout Zero. */
#if CALL_ONLY

/* IPC operation to use in ping thread - short call, timeout never */
#define WHATTODO_SHORT_PING \
  "sub  %%ebp,%%ebp  \n\t"  \
  "sub  %%ecx,%%ecx  \n\t"  \
  "sub  %%eax,%%eax  \n\t"  \
  IPC_SYSENTER

/* IPC operation to use in pong thread - short call, timeout never */
#define WHATTODO_SHORT_PONG \
  "sub  %%ebp,%%ebp  \n\t"  \
  "sub  %%ecx,%%ecx  \n\t"  \
  "sub  %%eax,%%eax  \n\t"  \
  IPC_SYSENTER

#else

/* IPC operation to use in ping thread - short call */
#define WHATTODO_SHORT_PING \
  "sub  %%ebp,%%ebp  \n\t"  \
  "sub  %%ecx,%%ecx  \n\t"  \
  "sub  %%eax,%%eax  \n\t"  \
  IPC_SYSENTER

/* IPC operation to use in pong thread - short reply, send timeout 0 */
#define WHATTODO_SHORT_PONG \
  "mov  $1,%%ebp     \n\t"  \
  "mov  $0x10,%%ecx  \n\t"  \
  "sub  %%eax,%%eax  \n\t"  \
  IPC_SYSENTER

#endif /* !CALL_ONLY */

/* IPC operation to use in ping thread - short send, receive long */
#define WHATTODO_LONG_PING \
  "sub  %%eax,%%eax  \n\t" \
  "sub  %%ecx,%%ecx  \n\t" \
  "push %%ebp        \n\t" \
  IPC_SYSENTER             \
  "pop  %%ebp        \n\t" \
  "add  $16384,%%ebp \n\t"

/* IPC operation to use in pong thread - reply long, send timeout 0 */
#define WHATTODO_LONG_PONG \
  "mov  $1,%%ebp     \n\t" \
  "mov  $0x10,%%ecx  \n\t" \
  "push %%eax        \n\t" \
  IPC_SYSENTER             \
  "pop  %%eax        \n\t" \
  "add  $16384,%%eax \n\t"

/* IPC operation to use in ping thread - short send, receive long */
#define WHATTODO_INDIRECT_PING \
  "sub  %%eax,%%eax  \n\t" \
  "sub  %%ecx,%%ecx  \n\t" \
  "push %%ebp        \n\t" \
  IPC_SYSENTER             \
  "pop  %%ebp        \n\t" \
  "add  $128,%%ebp   \n\t"

/* IPC operation to use in pong thread - reply long, send timeout 0 */
#define WHATTODO_INDIRECT_PONG \
  "mov  $1,%%ebp     \n\t" \
  "mov  $0x10,%%ecx  \n\t" \
  "push %%eax        \n\t" \
  IPC_SYSENTER             \
  "pop  %%eax        \n\t" \
  "add  $128,%%eax   \n\t"

/* IPC operation to use in ping thread - short send, receive fpage */
#define WHATTODO_FPAGE_PING \
  "mov  $0x82,%%ebp  \n\t"  \
  "sub  %%ecx,%%ecx  \n\t"  \
  "sub  %%eax,%%eax  \n\t"  \
  IPC_SYSENTER

/* IPC operation to use in send thread - short send fpage. We don't need
 * to save the two dwords describing the flexpage since the recevie thread
 * replies both values immediatly to the sender */
#define WHATTODO_FPAGE_PONG \
  "push %%eax        \n\t"  \
  "mov  $1,%%ebp     \n\t"  \
  "mov  $0x10,%%ecx  \n\t"  \
  "mov  $2,%%eax     \n\t"  \
  IPC_SYSENTER              \
  "pop  %%eax        \n\t"  \
  "add  %%eax,%%edx  \n\t"  \
  "add  %%eax,%%ebx  \n\t"

/* threads */
#define PAGER_THREAD  1
#define PING_THREAD   2
#define PONG_THREAD   3
#define MEMCPY_THREAD 4

#define NR_MSG        8
#define NR_DWORDS     4093  /* whole message descriptor occupies 4 pages */
#define NR_STRINGS    4

/*****************************************************************************
 *** global data
 *****************************************************************************/

#define timeout_10s	L4_IPC_TIMEOUT(0,0,152,7,0,0)
#define timeout_20s	L4_IPC_TIMEOUT(0,0,76,6,0,0)

#if defined(L4_API_L4X0) || defined(L4API_l4x0)
#define REGISTER_DWORDS 3
#else
#define REGISTER_DWORDS 2
#endif

/* threads */
       l4_threadid_t main_id;		/* main thread */
       l4_threadid_t pager_id;		/* local pager */
static l4_threadid_t ping_id;		/* ping thread */
static l4_threadid_t pong_id;		/* pong thread */
       l4_threadid_t memcpy_id;
       l4_umword_t scratch_mem;		/* memory for mapping */
static l4_umword_t fpagesize;
static l4_umword_t strsize;
static l4_umword_t strnum;
static l4_umword_t rounds;
static int use_superpages;
static int dont_do_superpages = 0;
static int dont_do_multiple_fpages = 0;
static int dont_do_ipc_asm = 0;

static struct __attribute__ ((aligned(4096)))
{
  l4_fpage_t fp;
  l4_msgdope_t size_dope;
  l4_msgdope_t send_dope;
  l4_umword_t  dw[NR_DWORDS];
} long_send_msg[NR_MSG];

static struct __attribute__ ((aligned(4096)))
{
  l4_fpage_t fp;
  l4_msgdope_t size_dope;
  l4_msgdope_t send_dope;
  l4_umword_t  dw[NR_DWORDS];
} long_recv_msg[NR_MSG];

static struct __attribute__ ((aligned(128)))
{
  l4_fpage_t fp;
  l4_msgdope_t size_dope;
  l4_msgdope_t send_dope;
  l4_umword_t  dw[4];
  l4_strdope_t  str[NR_STRINGS];
} indirect_send_msg[NR_MSG];

static struct __attribute__ ((aligned(128)))
{
  l4_fpage_t fp;
  l4_msgdope_t size_dope;
  l4_msgdope_t send_dope;
  l4_umword_t  dw[4];
  l4_strdope_t  str[NR_STRINGS];
} indirect_recv_msg[NR_MSG];

static char indirect_send_str[NR_STRINGS*NR_MSG*4096*4]
  __attribute__((aligned(4096)));
static char indirect_recv_str[NR_STRINGS*NR_MSG*4096*4]
  __attribute__((aligned(4096)));

/* stacks */
static unsigned char kip[L4_PAGESIZE] __attribute__((aligned(4096)));
static unsigned char pager_stack[STACKSIZE] __attribute__((aligned(4096)));
static unsigned char ping_stack[STACKSIZE] __attribute__((aligned(4096)));
static unsigned char pong_stack[STACKSIZE] __attribute__((aligned(4096)));

extern void pc_reset(void);

extern int l4_i386_ipc_call_asm (
			l4_threadid_t dest, 
			const void *snd_msg,
	      		l4_umword_t snd_dword0,
			l4_umword_t snd_dword1,
			void *rcv_msg,
			l4_umword_t *rcv_dword0,
			l4_umword_t *rcv_dword1,
			l4_timeout_t timeout,
			l4_msgdope_t *result)
			__attribute__((regparm(3)));
extern int l4_i386_ipc_reply_and_wait_asm (
			l4_threadid_t dest,
			const void *snd_msg,
			l4_umword_t snd_dword0,
			l4_umword_t snd_dword1,
			l4_threadid_t *src,
			void *rcv_msg,
			l4_umword_t *rcv_dword0,
			l4_umword_t *rcv_dword1,
			l4_timeout_t timeout,
			l4_msgdope_t *result)
			__attribute__((regparm(3)));

static inline unsigned
log2(unsigned value)
{
  unsigned log2_value;

  asm ("bsrl %1,%0\n\t" : "=r" (log2_value) : "r" (value));
  return log2_value;
}

/** guess kernel by looking at kernel info page version number */
static void
detect_kernel(void)
{
  l4_umword_t dummy;
  l4_msgdope_t result;

  l4_fpage_unmap(l4_fpage((l4_umword_t)kip, L4_LOG2_PAGESIZE, 0, 0), 
		 L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);
  l4_i386_ipc_call(rmgr_pager_id, 
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
  l4_fpage_unmap(l4_fpage((l4_umword_t)kip, L4_LOG2_PAGESIZE, 0, 0), 
		 L4_FP_FLUSH_PAGE|L4_FP_ALL_SPACES);
}

/** receive short message from ping thread */
static inline void
receive_ping_timeout(l4_timeout_t timeout)
{
  l4_umword_t dummy;
  l4_msgdope_t result;

  l4_i386_ipc_receive(ping_id,L4_IPC_SHORT_MSG,&dummy,&dummy,timeout,&result);

  if (L4_IPC_ERROR(result) == L4_IPC_RETIMEOUT)
    puts("  >> TIMEOUT!! <<");
}

#if DO_DEBUG
static void
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
print_testname(const char *test, int nr)
{
  printf(">> %d: %s: %x.%x <==> %x.%x\n",
         nr, test,
	 ping_id.id.task, ping_id.id.lthread,
	 pong_id.id.task, pong_id.id.lthread);
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

  ping_id = pong_id = main_id;
  ping_id.id.task += 1;
  pong_id.id.task += 2;
  ping_id.id.lthread = 0;
  pong_id.id.lthread = 0;

  /* first create pong task */
  t = rmgr_task_new(pong_id,255,(l4_umword_t)pong_stack + STACKSIZE,
		    (l4_umword_t)pong_thread,pager_id);
  if (l4_is_nil_id(t) || l4_is_invalid_id(t))
    {
      printf("failed to create pong task (%x)\n",ping_id.id.task);
      rmgr_task_new(ping_id,rmgr_id.lh.low,0,0,L4_NIL_ID);
      return;
    }

  t = rmgr_task_new(ping_id,255,(l4_umword_t)ping_stack + STACKSIZE,
		    (l4_umword_t)ping_thread,pager_id);
  if (l4_is_nil_id(t) || l4_is_invalid_id(t))
    {
      printf("failed to create ping task (%x)\n",ping_id.id.task);
      return;
    }

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

  l4_i386_ipc_call(from,
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

  l4_i386_ipc_call(from,
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
static void
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
static void
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
      l4_i386_ipc_wait(&src,L4_IPC_SHORT_MSG,&pfa,&eip,L4_IPC_NEVER,&result);
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

	  l4_i386_ipc_reply_and_wait(src,L4_IPC_SHORT_FPAGE,snd_base,fp,
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

/** Pong (reply) thread for short IPCs */
static void 
pong_short_thread(void)
{
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
  l4_umword_t ping_id32 = l4sys_to_id32(ping_id);
#else
  register l4_umword_t idlow  = ping_id.lh.low;
  register l4_umword_t idhigh = ping_id.lh.high;
#endif

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

#if DO_DEBUG
  hello("pong_short");
#endif

  /* ensure that ping is already created */
  call(main_id);

  /* wait for first request from ping thread */
  recv(ping_id);
  call(ping_id);

  while (1)
    {
      asm volatile 
	(
	 "push  %%ebp   \n\t"
	 WHATTODO_SHORT_PONG
	 WHATTODO_SHORT_PONG
	 WHATTODO_SHORT_PONG
	 WHATTODO_SHORT_PONG
	 WHATTODO_SHORT_PONG
	 WHATTODO_SHORT_PONG
	 WHATTODO_SHORT_PONG
	 WHATTODO_SHORT_PONG
	 "pop   %%ebp    \n\t"
	 :
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
	 : "S" (ping_id32)
	 : "eax", "ebx", "ecx", "edx", "edi"
#else
	 : "S" (idlow), "D" (idhigh)
	 : "eax", "ebx", "ecx", "edx"
#endif	 
	 );
    }
}

/** Ping (send) thread */
static void
ping_short_thread(void)
{
  int i;
  l4_cpu_time_t in,out;
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
  register l4_umword_t id32 = l4sys_to_id32(pong_id);
#else
  register l4_umword_t idlow  = pong_id.lh.low;
  register l4_umword_t idhigh = pong_id.lh.high;
#endif
  
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

#if DO_DEBUG
  hello("ping_short");
#endif
  call(pong_id);

  in = l4_rdtsc();
  for (i = ROUNDS; i; i--)
    {
      asm volatile 
	(
	 "push  %%ebp   \n\t"
	 WHATTODO_SHORT_PING
	 WHATTODO_SHORT_PING
	 WHATTODO_SHORT_PING
	 WHATTODO_SHORT_PING
	 WHATTODO_SHORT_PING
	 WHATTODO_SHORT_PING
	 WHATTODO_SHORT_PING
	 WHATTODO_SHORT_PING
	 "pop   %%ebp    \n\t"
	 :
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
	 : "S" (id32)
	 : "eax", "ebx", "ecx", "edx", "edi"
#else
	 : "S" (idlow), "D" (idhigh)
	 : "eax", "ebx", "ecx", "edx"
#endif	 
	);      
    }
  out = l4_rdtsc();

  printf("  %u cycles / %u rounds >> %u <<\n",
	 (l4_uint32_t)(out-in), 
	 8*ROUNDS, (l4_uint32_t)((out-in)/(8*ROUNDS)));

  /* tell main that we are finished */
  send(main_id);
  recv(main_id);

  /* done, sleep */
  sleep_forever();
}

/** Pong (reply) thread for short IPCs */
static void 
pong_short_c_thread(void)
{
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

#if DO_DEBUG
  hello("pong_short_c");
#endif

  /* ensure that ping is already created */
  call(main_id);

  /* wait for first request from ping thread */
  recv(ping_id);
  call(ping_id);

  while (1)
    {
      unsigned dummy1, dummy2;
      l4_msgdope_t result;
      l4_threadid_t src;

      l4_i386_ipc_reply_and_wait(ping_id, L4_IPC_SHORT_MSG, 2, 1,
				 &src, L4_IPC_SHORT_MSG, &dummy1, &dummy2,
				 L4_IPC_TIMEOUT(0,1,0,0,0,0), &result);
      l4_i386_ipc_reply_and_wait(ping_id, L4_IPC_SHORT_MSG, 4, 3,
				 &src, L4_IPC_SHORT_MSG, &dummy1, &dummy2,
				 L4_IPC_TIMEOUT(0,1,0,0,0,0), &result);
      l4_i386_ipc_reply_and_wait(ping_id, L4_IPC_SHORT_MSG, 6, 5,
				 &src, L4_IPC_SHORT_MSG, &dummy1, &dummy2,
				 L4_IPC_TIMEOUT(0,1,0,0,0,0), &result);
      l4_i386_ipc_reply_and_wait(ping_id, L4_IPC_SHORT_MSG, 8, 7,
				 &src, L4_IPC_SHORT_MSG, &dummy1, &dummy2,
				 L4_IPC_TIMEOUT(0,1,0,0,0,0), &result);
    }
}

/** Ping (send) thread */
static void
ping_short_c_thread(void)
{
  int i;
  l4_cpu_time_t in,out;
  
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

#if DO_DEBUG
  hello("ping_short_c");
#endif
  call(pong_id);

  in = l4_rdtsc();
  for (i = ROUNDS*2; i; i--)
    {
      unsigned dummy1, dummy2;
      l4_msgdope_t result;

      l4_i386_ipc_call(pong_id,
		       L4_IPC_SHORT_MSG, 1, 2,
	    	       L4_IPC_SHORT_MSG, &dummy1, &dummy2,
    		       L4_IPC_NEVER, &result);
      l4_i386_ipc_call(pong_id,
		       L4_IPC_SHORT_MSG, 3, 4,
	    	       L4_IPC_SHORT_MSG, &dummy1, &dummy2,
    		       L4_IPC_NEVER, &result);
      l4_i386_ipc_call(pong_id,
		       L4_IPC_SHORT_MSG, 5, 6,
	    	       L4_IPC_SHORT_MSG, &dummy1, &dummy2,
    		       L4_IPC_NEVER, &result);
      l4_i386_ipc_call(pong_id,
		       L4_IPC_SHORT_MSG, 7, 8,
	    	       L4_IPC_SHORT_MSG, &dummy1, &dummy2,
    		       L4_IPC_NEVER, &result);
    }
  out = l4_rdtsc();

  printf("  %u cycles / %u rounds >> %u <<\n",
	 (l4_uint32_t)(out-in), 
	 8*ROUNDS, (l4_uint32_t)((out-in)/(8*ROUNDS)));

  /* tell main that we are finished */
  send(main_id);

  /* done, sleep */
  sleep_forever();
}

/** Pong (reply) thread for short IPCs */
static void 
pong_short_asm_thread(void)
{
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

#if DO_DEBUG
  hello("pong_short_asm");
#endif

  /* ensure that ping is already created */
  call(main_id);

  /* wait for first request from ping thread */
  recv(ping_id);
  call(ping_id);

  while (1)
    {
      unsigned dummy1, dummy2;
      l4_msgdope_t result;
      l4_threadid_t src;

      l4_i386_ipc_reply_and_wait_asm(ping_id, L4_IPC_SHORT_MSG, 2, 1,
				     &src, L4_IPC_SHORT_MSG, &dummy1, &dummy2,
				     L4_IPC_TIMEOUT(0,1,0,0,0,0), &result);
      l4_i386_ipc_reply_and_wait_asm(ping_id, L4_IPC_SHORT_MSG, 4, 3,
				     &src, L4_IPC_SHORT_MSG, &dummy1, &dummy2,
				     L4_IPC_TIMEOUT(0,1,0,0,0,0), &result);
      l4_i386_ipc_reply_and_wait_asm(ping_id, L4_IPC_SHORT_MSG, 6, 5,
				     &src, L4_IPC_SHORT_MSG, &dummy1, &dummy2,
				     L4_IPC_TIMEOUT(0,1,0,0,0,0), &result);
      l4_i386_ipc_reply_and_wait_asm(ping_id, L4_IPC_SHORT_MSG, 8, 7,
				     &src, L4_IPC_SHORT_MSG, &dummy1, &dummy2,
				     L4_IPC_TIMEOUT(0,1,0,0,0,0), &result);
    }
}

/** Ping (send) thread */
static void
ping_short_asm_thread(void)
{
  int i;
  l4_cpu_time_t in,out;
  
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

#if DO_DEBUG
  hello("ping_short_asm");
#endif
  call(pong_id);

  in = l4_rdtsc();
  for (i = ROUNDS*2; i; i--)
    {
      unsigned dummy1, dummy2;
      l4_msgdope_t result;

      l4_i386_ipc_call_asm(pong_id,
			   L4_IPC_SHORT_MSG, 1, 2,
			   L4_IPC_SHORT_MSG, &dummy1, &dummy2,
			   L4_IPC_NEVER, &result);
      l4_i386_ipc_call_asm(pong_id,
			   L4_IPC_SHORT_MSG, 3, 4,
			   L4_IPC_SHORT_MSG, &dummy1, &dummy2,
			   L4_IPC_NEVER, &result);
      l4_i386_ipc_call_asm(pong_id,
			   L4_IPC_SHORT_MSG, 5, 6,
			   L4_IPC_SHORT_MSG, &dummy1, &dummy2,
			   L4_IPC_NEVER, &result);
      l4_i386_ipc_call_asm(pong_id,
			   L4_IPC_SHORT_MSG, 7, 8,
			   L4_IPC_SHORT_MSG, &dummy1, &dummy2,
			   L4_IPC_NEVER, &result);
    }
  out = l4_rdtsc();

  printf("  %u cycles / %u rounds >> %u <<\n",
	 (l4_uint32_t)(out-in), 
	 8*ROUNDS, (l4_uint32_t)((out-in)/(8*ROUNDS)));

  /* tell main that we are finished */
  send(main_id);

  /* done, sleep */
  sleep_forever();
}

/** Pong (reply) thread */
static void 
pong_long_thread(void)
{
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
  l4_umword_t ping_id32 = l4sys_to_id32(ping_id);
#endif
  int i;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

#if DO_DEBUG
  hello("pong_long");
#endif

  for (i=0; i<8; i++)
    {
      long_send_msg[i].size_dope = L4_IPC_DOPE(NR_DWORDS, 0);
      long_send_msg[i].send_dope = L4_IPC_DOPE(strsize, 0);
      memset(&long_send_msg[i].dw[0], 0x67, 4*NR_DWORDS);
    }

  /* ensure that ping is already created */
  call(main_id);

  /* wait for first request from ping thread */
  recv(ping_id);
  call(ping_id);

  while (1)
    {
      l4_umword_t dummy;
      asm volatile 
	(
	 "push %%ebp      \n\t"
	 WHATTODO_LONG_PONG
	 WHATTODO_LONG_PONG
	 WHATTODO_LONG_PONG
	 WHATTODO_LONG_PONG
	 WHATTODO_LONG_PONG
	 WHATTODO_LONG_PONG
	 WHATTODO_LONG_PONG
	 WHATTODO_LONG_PONG
	 "pop   %%ebp    \n\t"
	 : "=a" (dummy)
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
	 : "a" (long_send_msg), "S" (ping_id32)
	 : "ebx", "ecx", "edx", "edi"
#else
	 : "a" (long_send_msg), "D" (ping_id.lh.high), "S" (ping_id.lh.low)
	 : "ebx", "ecx", "edx"
#endif
	 );
    }
}

/** Ping (send) thread */
static void
ping_long_thread(void)
{
  int i;
  l4_cpu_time_t in,out;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  for (i=0; i<NR_MSG; i++)
    {
      long_recv_msg[i].size_dope = L4_IPC_DOPE(NR_DWORDS, 0);
      long_recv_msg[i].send_dope = L4_IPC_DOPE(0, 0);
      memset(&long_recv_msg[i].dw[0], 0x66, 4*NR_DWORDS);
    }

    {
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
      register l4_umword_t id32 = l4sys_to_id32(pong_id);
#else
      register l4_umword_t idlow = pong_id.lh.low;
      register l4_umword_t idhigh = pong_id.lh.high;
#endif    

#if DO_DEBUG
      hello("ping_long");
#endif
      call(pong_id);

      in = l4_rdtsc();
      for (i=rounds; i; i--)
	{
	  l4_umword_t dummy;
	  asm volatile 
	    (
	     "push  %%ebp      \n\t"
	     "mov   %%ebx,%%ebp\n\t"
	     WHATTODO_LONG_PING
	     WHATTODO_LONG_PING
	     WHATTODO_LONG_PING
	     WHATTODO_LONG_PING
	     WHATTODO_LONG_PING
	     WHATTODO_LONG_PING
	     WHATTODO_LONG_PING
	     WHATTODO_LONG_PING
	     "pop   %%ebp      \n\t"
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
	     : "=b" (dummy)
	     :  "b" (&long_recv_msg), "S" (id32)
	     : "eax", "ecx", "edx", "edi"
#else
	     : "=b" (dummy)
	     :  "b" (&long_recv_msg), "S" (idlow), "D" (idhigh)
	     : "eax", "ecx", "edx"
#endif
	    );
	}
      out = l4_rdtsc();
    }

  for (i=0; i<NR_MSG; i++)
    {
      if (memchr(&long_recv_msg[i].dw[REGISTER_DWORDS], 
		 0x66, 4*(strsize-REGISTER_DWORDS)))
	{
	  printf("Test failed (found 0x66 in %08x-%08x)!\n",
	      (unsigned)&long_recv_msg[i].dw[REGISTER_DWORDS],
	      (unsigned)&long_recv_msg[i].dw[REGISTER_DWORDS]
			+4*(strsize-REGISTER_DWORDS));
	  enter_kdebug("stop");
	  break;
	}
    }

  printf("  %4d dwords (%5d bytes): %10u cycles / %6u rounds >> %5u <<\n",
         strsize, strsize*4,
	 (l4_uint32_t)(out-in), 8*rounds, (l4_uint32_t)((out-in)/(8*rounds)));

  /* tell main that we are finished */
  send(main_id);

  /* done, sleep */
  sleep_forever();
}

/** Pong (reply) thread */
static void 
pong_indirect_thread(void)
{
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
  l4_umword_t ping_id32 = l4sys_to_id32(ping_id);
#endif
  int i;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

#if DO_DEBUG
  hello("pong_indirect");
#endif

  for (i=0; i<8; i++)
    {
      int j;

      indirect_send_msg[i].size_dope = L4_IPC_DOPE(4, strnum);
      indirect_send_msg[i].send_dope = L4_IPC_DOPE(2, strnum);
      indirect_send_msg[i].dw[0] = 0x10101010;
      indirect_send_msg[i].dw[1] = 0x11111111;
      indirect_send_msg[i].dw[2] = 0x12121212;
      indirect_send_msg[i].dw[3] = 0x13131313;
      for (j=0; j<strnum; j++)
	{
	  l4_umword_t str = (l4_umword_t)&indirect_send_str
			  + i*NR_STRINGS*(NR_DWORDS+3)*4
			  + j*strsize*4;
	  indirect_send_msg[i].str[j].snd_size = strsize*4;
	  indirect_send_msg[i].str[j].snd_str  = str;
	  indirect_send_msg[i].str[j].rcv_size = 0;
	  indirect_send_msg[i].str[j].rcv_str  = 0;
	  memset((void*)str, 's', strsize*4);
	}
    }

  /* ensure that ping is already created */
  call(main_id);

  /* wait for first request from ping thread */
  recv(ping_id);
  call(ping_id);

  while (1)
    {
      l4_umword_t dummy;
      asm volatile 
	(
	 "push %%ebp      \n\t"
	 WHATTODO_INDIRECT_PONG
	 WHATTODO_INDIRECT_PONG
	 WHATTODO_INDIRECT_PONG
	 WHATTODO_INDIRECT_PONG
	 WHATTODO_INDIRECT_PONG
	 WHATTODO_INDIRECT_PONG
	 WHATTODO_INDIRECT_PONG
	 WHATTODO_INDIRECT_PONG
	 "pop   %%ebp    \n\t"
	 : "=a" (dummy)
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
	 : "a" (indirect_send_msg), "S" (ping_id32)
	 : "ebx", "ecx", "edx", "edi"
#else
	 : "a" (indirect_send_msg), "D" (ping_id.lh.high), "S" (ping_id.lh.low)
	 : "ebx", "ecx", "edx"
#endif
	 );
    }
}

/** Ping (send) thread */
static void
ping_indirect_thread(void)
{
  int i;
  l4_cpu_time_t in,out;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  for (i=0; i<NR_MSG; i++)
    {
      int j;

      indirect_recv_msg[i].size_dope = L4_IPC_DOPE(4, strnum);
      indirect_recv_msg[i].send_dope = L4_IPC_DOPE(0, 0);
      memset(&indirect_recv_msg[i].dw[0], 0x66, 4*4);
      for (j=0; j<strnum; j++)
	{
	  l4_umword_t str = (l4_umword_t)&indirect_recv_str
			  + i*NR_STRINGS*(NR_DWORDS+3)*4
			  + j*strsize*4;
	  indirect_recv_msg[i].str[j].snd_size = 0;
	  indirect_recv_msg[i].str[j].snd_str  = 0;
	  indirect_recv_msg[i].str[j].rcv_size = strsize*4;
	  indirect_recv_msg[i].str[j].rcv_str  = str;
	  memset((void*)str, 'r', strsize*4);
	}
    }

    {
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
      register l4_umword_t id32 = l4sys_to_id32(pong_id);
#else
      register l4_umword_t idlow = pong_id.lh.low;
      register l4_umword_t idhigh = pong_id.lh.high;
#endif    

#if DO_DEBUG
      hello("ping_indirect");
#endif
      call(pong_id);

      in = l4_rdtsc();
      for (i=rounds; i; i--)
	{
	  l4_umword_t dummy;
	  asm volatile 
	    (
	     "push  %%ebp      \n\t"
	     "mov   %%ebx,%%ebp\n\t"
	     WHATTODO_INDIRECT_PING
	     WHATTODO_INDIRECT_PING
	     WHATTODO_INDIRECT_PING
	     WHATTODO_INDIRECT_PING
	     WHATTODO_INDIRECT_PING
	     WHATTODO_INDIRECT_PING
	     WHATTODO_INDIRECT_PING
	     WHATTODO_INDIRECT_PING
	     "pop   %%ebp      \n\t"
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
	     : "=b" (dummy)
	     :  "b" (&indirect_recv_msg), "S" (id32)
	     : "eax", "ecx", "edx", "edi"
#else
	     : "=b" (dummy)
	     :  "b" (&indirect_recv_msg), "S" (idlow), "D" (idhigh)
	     : "eax", "ecx", "edx"
#endif
	    );
	}
      out = l4_rdtsc();
    }

  printf("  %dx%5d dwords (%5d bytes): %10u cycles / %6u rounds >> %7u <<\n",
         strnum, strsize, strnum*strsize*4,
	 (l4_uint32_t)(out-in), rounds*8, (l4_uint32_t)((out-in)/(rounds*8)));

  /* tell main that we are finished */
  send(main_id);

  /* done, sleep */
  sleep_forever();
}


/** receive fpage thread */
static void
pong_fpage_thread(void)
{
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  map_scratch_mem_from_pager();

  /* ensure that ping is already created */
  call(main_id);

  recv(ping_id);
  call(ping_id);

#if DO_DEBUG
  hello("pong_fpage");
#endif

    {
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
      register l4_umword_t ping_id32 = l4sys_to_id32(ping_id);
#else
      register l4_umword_t idlow  = ping_id.lh.low;
      register l4_umword_t idhigh = ping_id.lh.high;
#endif
      register l4_umword_t dw0 = scratch_mem;
      register l4_umword_t dw1 = l4_fpage(scratch_mem, log2(fpagesize),
					  L4_FPAGE_RW, L4_FPAGE_MAP).fpage;
      while (1)
	{
	  asm volatile
	    (
	     "push  %%ebp   \n\t"
	     WHATTODO_FPAGE_PONG
	     WHATTODO_FPAGE_PONG
	     WHATTODO_FPAGE_PONG
	     WHATTODO_FPAGE_PONG
	     WHATTODO_FPAGE_PONG
	     WHATTODO_FPAGE_PONG
	     WHATTODO_FPAGE_PONG
	     WHATTODO_FPAGE_PONG
	     "pop   %%ebp    \n\t"
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
	     : "=d" (dw0), "=b" (dw1)
	     : "S" (ping_id32), "d" (dw0), "b" (dw1), "a" (fpagesize)
	     : "ecx", "edi"
#else
	     : "=d" (dw0), "=b" (dw1)
	     : "S" (idlow), "D" (idhigh), "d" (dw0), "b" (dw1), "a" (fpagesize)
	     : "ecx"
#endif
	    );
	}
    }
}

/** Send short fpages */
static void
ping_fpage_thread(void)
{
  int i;
  l4_cpu_time_t in,out;
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
  register l4_umword_t id32 = l4sys_to_id32(pong_id);
#endif
  register l4_umword_t dw0 = 0, dw1 = 0;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

#if DO_DEBUG
  hello("ping_fpage");
#endif
  call(pong_id);

  in = l4_rdtsc();
  for (i = rounds; i; i--)
    {
      asm volatile
	(
	 "push  %%ebp   \n\t"
	 WHATTODO_FPAGE_PING
	 WHATTODO_FPAGE_PING
	 WHATTODO_FPAGE_PING
	 WHATTODO_FPAGE_PING
	 WHATTODO_FPAGE_PING
	 WHATTODO_FPAGE_PING
	 WHATTODO_FPAGE_PING
	 WHATTODO_FPAGE_PING
	 "pop  %%ebp    \n\t"
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
	 :  "=d" (dw0), "=b" (dw1)
	 :  "S" (id32),  "d" (dw0),  "b" (dw1)
	 : "eax", "ecx", "edi"
#else
	 :  "=d" (dw0), "=b" (dw1)
	 :  "S" (pong_id.lh.low),  "D" (pong_id.lh.high), "d" (dw0),  "b" (dw1)
	 : "eax", "ecx"
#endif
	);
    }
  out = l4_rdtsc();
  
  printf("  %4dkB: %9u cycles / %5u rounds >> %8u <<\n",
         fpagesize/1024,
	 (l4_uint32_t)(out-in), rounds*8, (l4_uint32_t)((out-in)/(rounds*8)));

  /* tell main that we are finished */
  send(main_id);

  /* done, sleep */
  sleep_forever();
}

/** Pong thread replying long fpage messages */
static void 
pong_long_fpage_thread(void)
{
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
  l4_umword_t ping_id32 = l4sys_to_id32(ping_id);
#endif
  int i;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

#if DO_DEBUG
  hello("pong_fpage_long");
#endif

  for (i=0; i<rounds; i++)
    {
      /* map 1024 flexpages each round */
      l4_umword_t mem = scratch_mem + 1024*L4_PAGESIZE*i;
      int j;

      long_send_msg[i].size_dope = L4_IPC_DOPE(NR_DWORDS, 0);
      long_send_msg[i].send_dope = L4_IPC_DOPE(2048, 0);
      long_send_msg[i].dw[ 0] = mem;
      long_send_msg[i].dw[ 1] = l4_fpage(mem, L4_LOG2_PAGESIZE,
					 L4_FPAGE_RW, L4_FPAGE_MAP).fpage;
      for (j=1; j<1024; j++)
	{
	  long_send_msg[i].dw[2*j  ] = long_send_msg[i].dw[2*j-2] + L4_PAGESIZE;
	  long_send_msg[i].dw[2*j+1] = long_send_msg[i].dw[2*j-1] + L4_PAGESIZE;
	}

      long_send_msg[i].dw[2*j  ] = 0; /* terminate */
      long_send_msg[i].dw[2*j+1] = 0; /* terminate */
    }

  map_scratch_mem_from_pager();

  /* ensure that ping is already created */
  call(main_id);

  /* wait for first request from ping thread */
  recv(ping_id);
  call(ping_id);

  for (i=0; ; i++)
    {
      l4_umword_t dummy;
      asm volatile 
	(
	 "push %%ebp        \n\t"
	 "mov  $1,%%ebp     \n\t"
	 "mov  $0x10,%%ecx  \n\t"
	 IPC_SYSENTER
	 "pop   %%ebp       \n\t"
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
	 : "=a" (dummy), "=b" (dummy), "=d" (dummy)
	 : "a" ((l4_umword_t)&long_send_msg[i] | 2),
	   "d" (long_send_msg[i].dw[0]), "b" (long_send_msg[i].dw[1]),
	   "D" (long_send_msg[i].dw[2]), "S" (ping_id32)
	 : "ecx"
#else
	 : "=a" (dummy), "=b" (dummy), "=d" (dummy)
	 : "a" ((l4_umword_t)&long_send_msg[i] | 2),
	   "d" (long_send_msg[i].dw[0]), "b" (long_send_msg[i].dw[1]),
	   "S" (ping_id.lh.low), "D" (ping_id.lh.high)
	 : "ecx"
#endif
	 );
    }
}

/** Ping (send) thread expecting long fpage messages */
static void
ping_long_fpage_thread(void)
{
  int i;
  l4_cpu_time_t in,out;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  for (i=0; i<rounds; i++)
    {
      long_recv_msg[i].fp = l4_fpage(0, L4_WHOLE_ADDRESS_SPACE,
				     L4_FPAGE_RW, L4_FPAGE_MAP);
      long_recv_msg[i].size_dope = L4_IPC_DOPE(NR_DWORDS, 0);
      long_recv_msg[i].send_dope = L4_IPC_DOPE(0, 0);
      memset(&long_recv_msg[i].dw[0], 0x55, NR_DWORDS*4);
    }

    {
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
      register l4_umword_t id32 = l4sys_to_id32(pong_id);
#else
      register l4_umword_t idlow = pong_id.lh.low;
      register l4_umword_t idhigh = pong_id.lh.high;
#endif    

#if DO_DEBUG
      hello("ping_fpage_long");
#endif
      call(pong_id);

      in = l4_rdtsc();
      for (i=0; i<rounds; i++)
	{
	  l4_umword_t dummy;
	  asm volatile 
	    (
	     "push %%ebp        \n\t"
	     "mov  %%ebx,%%ebp  \n\t"
	     "sub  %%eax,%%eax  \n\t"
	     "sub  %%ecx,%%ecx  \n\t"
	     IPC_SYSENTER
	     "pop  %%ebp        \n\t"
#if defined(L4_API_L4X0) || defined(L4API_l4x0)
	     : "=b" (dummy)
	     :  "b" (&long_recv_msg[i]), "S" (id32)
	     : "eax", "ecx", "edx", "edi"
#else
	     : "=b" (dummy)
	     :  "b" (&long_recv_msg[i]), "S" (idlow), "D" (idhigh)
	     : "eax", "ecx", "edx"
#endif
	    );
	}
      out = l4_rdtsc();
    }

  printf("  %d fpages (%d MB): %u cycles / %u rounds a 1024 fpages >> %u/fp <<\n",
         SCRATCH_MEM_SIZE/L4_PAGESIZE, SCRATCH_MEM_SIZE/(1024*1024),
	 (l4_uint32_t)(out-in), rounds,
	 (l4_uint32_t)((out-in)/(rounds*1024)));

  /* tell main that we are finished */
  send(main_id);

  /* done, sleep */
  sleep_forever();
}

/** pager thread */
static void
pong_pagefault_thread(void)
{
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  map_scratch_mem_from_pager();

  /* ensure that ping is already created */
  call(main_id);

  recv(ping_id);

#if DO_DEBUG
  hello("pong_pager");
#endif

  while (1)
    {
      register l4_umword_t dw0, dw1;
      register l4_umword_t idlow, idhigh;
      register l4_umword_t fp = log2(fpagesize) << 2;

      asm volatile
	(
	 "push %%ebp        \n\t"
	 "mov  $2,%%eax     \n\t"
	 "mov  $0x10,%%ecx  \n\t"
     	 "mov  $1,%%ebp     \n\t"
	 IPC_SYSENTER
	 "pop  %%ebp        \n\t"
	 : "=d" (dw0), "=b" (dw1), "=S" (idlow), "=D" (idhigh)
	 :
	 : "eax", "ecx"
	 );

      while (1)
	{
	  dw0 &= ~(fpagesize-1);
	  dw1  = dw0 | fp;
	  asm volatile
	    (
	     "push %%ebp        \n\t"
	     "movl $2,%%eax     \n\t"
	     "mov  $0x10,%%ecx  \n\t"
	     "mov  $1,%%ebp     \n\t"
	     IPC_SYSENTER
	     "pop  %%ebp        \n\t"
	     : "=d" (dw0), "=b" (dw1), "=S" (idlow), "=D" (idhigh)
	     :  "d" (dw0),  "b" (dw1),  "S" (idlow),  "D" (idhigh)
	     : "eax", "ecx"
	     );
	}
    }
}

/** Raise pagefaults */
static void
ping_pagefault_thread(void)
{
  int i;
  l4_cpu_time_t in,out;
  register l4_umword_t dw0 = scratch_mem;
  l4_umword_t dummy;
  
  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

    {
      /* change my pager to pong_pagefault_thread */
      l4_threadid_t preempter = L4_INVALID_ID, pager = pong_id;
      l4_thread_ex_regs(l4_myself(), (l4_umword_t)-1, (l4_umword_t)-1,
			&preempter, &pager, &dummy, &dummy, &dummy);
    }

#if DO_DEBUG
  hello("ping_pagefaultee");
#endif
  call(pong_id);

  in = l4_rdtsc();
  for (i = rounds; i; i--)
    {
      asm volatile
	(
	 "mov  (%%eax),%%ebx        \n\t"
	 "movl (%%eax,%%ecx,1),%%edx\n\t"
	 "addl %%ecx,%%eax          \n\t"
	 "addl %%ecx,%%eax          \n\t"
	 "mov  (%%eax),%%ebx        \n\t"
	 "movl (%%eax,%%ecx,1),%%edx\n\t"
	 "addl %%ecx,%%eax          \n\t"
	 "addl %%ecx,%%eax          \n\t"
	 "mov  (%%eax),%%ebx        \n\t"
	 "movl (%%eax,%%ecx,1),%%edx\n\t"
	 "addl %%ecx,%%eax          \n\t"
	 "addl %%ecx,%%eax          \n\t"
	 "mov  (%%eax),%%ebx        \n\t"
	 "movl (%%eax,%%ecx,1),%%edx\n\t"
	 "addl %%ecx,%%eax          \n\t"
	 "addl %%ecx,%%eax          \n\t"
	 : "=a" (dw0), "=c" (dummy)
	 :  "a" (dw0),  "c" (fpagesize)
	 : "ebx", "edx"
	);
    }
  out = l4_rdtsc();
  
  printf("  %d%cB => %d%cB: %9u cycles / %5u rounds >> %6u <<\n",
         use_superpages ? L4_SUPERPAGESIZE/(1024*1024) : L4_PAGESIZE / 1024,
	 use_superpages ? 'M' : 'k',
         fpagesize > 1024*1024 ? fpagesize/(1024*1024) : fpagesize / 1024,
	 fpagesize > 1024*1024 ? 'M' : 'k',
	 (l4_uint32_t)(out-in), rounds*8, (l4_uint32_t)((out-in)/(rounds*8)));

  /* tell main that we are finished */
  send(main_id);

  /* done, sleep */
  sleep_forever();
}

/** Raise exceptions */
static void
ping_exception_thread(void)
{
  int i;
  l4_cpu_time_t in,out;
  static idt_t idt;

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  idt.limit = 0x20*8 - 1;
  idt.base = &idt.desc;

  for (i=0; i<0x20; i++)
    MAKE_IDT_DESC(idt, i, 0);

  MAKE_IDT_DESC(idt, 13, dummy_exception13_handler);

  asm volatile ("lidt (%%eax)\n\t" : : "a" (&idt));

#if DO_DEBUG
  hello("ping_exceptioner");
#endif

  in = l4_rdtsc();
  for (i = 8*ROUNDS; i; i--)
    {
      // this int is reflected to int 13 because the user must not
      // direct call int 13
      asm volatile ("int $13" : : : "memory");
    }
  out = l4_rdtsc();
  
  printf("  %9u cycles / %5u rounds >> %u <<\n",
	 (l4_uint32_t)(out-in), ROUNDS*8, (l4_uint32_t)((out-in)/(ROUNDS*8)));

  /* tell main that we are finished */
  send(main_id);
  recv(main_id);

  /* done, sleep */
  sleep_forever();
}

/** IPC benchmark - short INTRA-AS */
static void
benchmark_short_intraAS(int nr)
{
  /* start ping and pong thread */
  print_testname("short intra IPC", 0);

  ping_id = pong_id = main_id;
  ping_id.id.lthread = PING_THREAD;
  pong_id.id.lthread = PONG_THREAD;

  create_thread(ping_id,(l4_umword_t)ping_short_thread,
		(l4_umword_t)ping_stack + STACKSIZE,pager_id);

  create_thread(pong_id,(l4_umword_t)pong_short_thread,
		(l4_umword_t)pong_stack + STACKSIZE,pager_id);

  recv(pong_id);
  send(pong_id);

  /* wait for measurement to be finished */
  receive_ping_timeout(timeout_10s);

  /* shutdown pong thread, ping thread already sleeps */
  create_thread(pong_id,(l4_umword_t)asm_do_sleep,
		(l4_umword_t)pong_stack + STACKSIZE,pager_id);

  /* give ping thread time to go to bed */
  send(ping_id);
  /* give pong thread time to go to bed */
  l4_thread_switch(L4_NIL_ID);
}

/** IPC benchmark - short INTER-AS */
static void
benchmark_short_interAS(int nr)
{
  print_testname("short inter IPC", nr);

  create_pingpong_tasks(ping_short_thread, pong_short_thread);

  /* wait for measurement to be finished, then kill tasks */
  receive_ping_timeout(timeout_10s);
  kill_pingpong_tasks();
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

  print_testname("long inter IPC", nr);

  for (i=0; i<sizeof(strsizes)/sizeof(strsizes[0]); i++)
    {
      strsize = strsizes[i].sz;
      rounds  = strsizes[i].rounds;
#ifdef FIASCO_UX
      rounds /= 10;
#endif
      create_pingpong_tasks(ping_long_thread, pong_long_thread); 
      /* wait for measurement to be finished, then kill tasks */
      receive_ping_timeout(timeout_20s);
      kill_pingpong_tasks();
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
	{.sz=   16,.num=4,.rounds=12500},{.sz=   64, .num=1,.rounds=12500},
	{.sz=  256,.num=4,.rounds= 4500},{.sz= 1024, .num=1,.rounds= 4500},
	{.sz=  512,.num=4,.rounds= 3000},{.sz= 2048, .num=1,.rounds= 3000},
	{.sz= 1024,.num=4,.rounds= 2500},{.sz= 4096, .num=1,.rounds= 2500},
	{.sz= 2048,.num=4,.rounds=  500},{.sz= 8192, .num=1,.rounds=  500},
	{.sz= 4096,.num=4,.rounds=  400},{.sz=16384, .num=1,.rounds=  400}
    };
  int i;

  print_testname("indirect inter IPC", nr);

  for (i=0; i<sizeof(strsizes)/sizeof(strsizes[0]); i++)
    {
      strsize = strsizes[i].sz;
      strnum  = strsizes[i].num;
      rounds  = strsizes[i].rounds;
#ifdef FIASCO_UX
      rounds /= 10;
#endif
      create_pingpong_tasks(ping_indirect_thread, pong_indirect_thread);
      /* wait for measurement to be finished, then kill tasks */
      receive_ping_timeout(timeout_20s);
      kill_pingpong_tasks();
    }
}

/** IPC benchmark - Mapping test */
static void
benchmark_shortMap_test(void)
{
#ifdef FIASCO_UX
  static int fpagesizes[] = { 1, 2, 4, 16, 64, 256, 512 };
#else
  static int fpagesizes[] = { 1, 2, 4, 16, 64, 256, 1024 };
#endif
  int i;

  for (i=0; i<sizeof(fpagesizes)/sizeof(fpagesizes[0]); i++)
    {
      fpagesize = fpagesizes[i]*L4_PAGESIZE;
      rounds    = SCRATCH_MEM_SIZE/(fpagesize*8);

      create_pingpong_tasks(ping_fpage_thread, pong_fpage_thread);

      /* wait for measurement to be finished, then kill tasks */
      receive_ping_timeout(timeout_10s);
      kill_pingpong_tasks();
    }
}

/** IPC benchmark - Mapping */
static void
benchmark_shortMap_interAS(int nr)
{
  use_superpages = 0;
  print_testname("short fpage map from 4k", nr);
  benchmark_shortMap_test();

  if (!dont_do_superpages)
    {
      use_superpages = 1;
      print_testname("short fpage map from 4M", nr);
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

  print_testname("long fpage map", nr);
  create_pingpong_tasks(ping_long_fpage_thread, pong_long_fpage_thread);

  /* wait for measurement to be finished, then kill tasks */
  receive_ping_timeout(timeout_10s);
  kill_pingpong_tasks();
}

/** IPC benchmark - pagefault test */
static void
benchmark_pagefault_test(void)
{
  rounds = SCRATCH_MEM_SIZE/(fpagesize*8);
  create_pingpong_tasks(ping_pagefault_thread, pong_pagefault_thread);

  /* wait for measurement to be finished, then kill tasks */
  receive_ping_timeout(timeout_10s);
  kill_pingpong_tasks();
}

/** IPC benchmark - pagefaults */
static void
benchmark_pagefault_interAS(int nr)
{
  print_testname("pagefault inter address space", nr);

  fpagesize = L4_PAGESIZE;
  use_superpages = 0;
  benchmark_pagefault_test();

  if (!dont_do_superpages)
    {
      fpagesize = L4_PAGESIZE;
      use_superpages = 1;
      benchmark_pagefault_test();

#ifndef FIASCO_UX
      /* We need at least 32 MB for this test */
      fpagesize = L4_SUPERPAGESIZE;
      use_superpages = 1;
      benchmark_pagefault_test();
#endif
    }
  else
    puts("INTER-AS-PF disabled from 4M because Jochen LN detected");
}

static void
benchmark_exception_intraAS(int nr)
{
  /* start ping and pong thread */
  print_testname("intra exceptions", 7);

  ping_id = main_id;
  ping_id.id.lthread = PING_THREAD;

  create_thread(ping_id,(l4_umword_t)ping_exception_thread,
		(l4_umword_t)ping_stack + STACKSIZE,pager_id);

  /* wait for measurement to be finished */
  receive_ping_timeout(timeout_10s);
  /* give ping thread time to go to bed */
  send(ping_id);
}

/** IPC benchmark - short INTER-AS with c-bindings */
static void
benchmark_short_c_interAS(int nr)
{
  print_testname("short inter IPC (c-inline-bindings)", nr);

  create_pingpong_tasks(ping_short_c_thread, pong_short_c_thread);

  /* wait for measurement to be finished, then kill tasks */
  receive_ping_timeout(timeout_10s);
  kill_pingpong_tasks();
}

/** IPC benchmark - short INTER-AS with extern assembler functions */
static void
benchmark_short_asm_interAS(int nr)
{
  print_testname("short inter IPC (external assembler)", nr);

  create_pingpong_tasks(ping_short_asm_thread, pong_short_asm_thread);

  /* wait for measurement to be finished, then kill tasks */
  receive_ping_timeout(timeout_10s);
  kill_pingpong_tasks();
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
  l4_calibrate_tsc();

  printf("CPU frequency is %dMhz\n", l4_get_hz()/1000000);
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
"     the client enters the kernel raising exception 0x14 (pagefault) -- and\n"
"     not through the system call interface.\n"
"  7: A thread raises exceptions 0x0d. The user level exception handler\n"
"     returns immediately behind the faulting instruction.\n"
"  8: Same as test 1 except we use the standard C-bindings.\n"
"  9: Same as test 1 except we use static functions implemented in Assembler\n"
"     for performing ipc.\n"
#ifndef FIASCO_UX
"\n"
"  Press any key to continue ..."
#endif
      );
#ifndef FIASCO_UX
  getchar();
#endif
  putchar('\n');
}

/** Main */
int
main(int argc, const char **argv)
{
#ifndef FIASCO_UX
  int c;
#endif
  int i;
  int loop;
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

  puts(  "\n"
         "  L4 pingpong IPC benchmark");
  printf("========= IPC code ==========\n");
  printf("        %s",IPC_SYSENTER);
  puts(  "\r"
         "=============================");

  detect_kernel();
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

  /* allocate scratch memory for sending flexpages */
  scratch_mem = rmgr_reserve_mem(SCRATCH_MEM_SIZE, 
				 L4_LOG2_SUPERPAGESIZE, 0, 0, 0);
  printf("%d MB scratch memory ", SCRATCH_MEM_SIZE/(1024*1024));
  if (scratch_mem == -1)
    puts("not reserved -- disabling fpage tests");
  else
    {
      printf("at %08x-%08x reserved\n",
	     scratch_mem, scratch_mem+SCRATCH_MEM_SIZE);
      map_scratch_mem_from_rmgr();
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
  for (;;)
    {      
      for (i=0; i<MENU_ENTRIES; i++)
	{
	  if (test[i].enabled)
	    test[i].func(i);
	}
#if 0
      if (!loop)
	{
	  puts("Finished.");
	  sleep_forever();
	}
#endif
    }

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

