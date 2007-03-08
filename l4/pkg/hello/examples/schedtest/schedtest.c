
#include <l4/sys/ipc.h>
#include <l4/sys/kernel.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/timeout.h>
#include <l4/sys/types.h>
#include <l4/sys/rt_sched.h>
#include <l4/util/util.h>
#include <stdio.h>
#include <stdlib.h>
#include "extensions.h"
#include <l4/util/l4_macros.h>

#ifdef USE_L4ENV
#include <l4/thread/thread.h>
#include <l4/util/kip.h>
#endif

#define TEST_THREAD_NUMBER      10

static l4_threadid_t main_thread_id	= L4_INVALID_ID;
static l4_threadid_t test_thread_id	= L4_INVALID_ID;
#ifdef USE_L4ENV
static l4_kernel_info_t *kinfo = 0;
#else
static l4_kernel_info_t *kinfo		= (void *) 0x2000000;
#endif
char test_thread_stack[4096];

char LOG_tag[9] = "sched";
l4_ssize_t l4libc_heapsize = 2*1024*1024;


static int get_thread_ids (l4_threadid_t *id,
                           l4_threadid_t *preempter,    
                           l4_threadid_t *pager,
                           l4_addr_t *eip,
                           l4_addr_t *esp)
{
  l4_umword_t ignore_word;
  l4_threadid_t my_id, ignore_id;

  my_id = l4_myself();
  
  if (id)        *id        = my_id;    
  if (preempter) *preempter = L4_INVALID_ID;
  if (pager)     *pager     = L4_INVALID_ID;

  l4_thread_ex_regs (my_id, -1, -1,	/* ID, EIP, ESP */
                     preempter ? preempter : &ignore_id,
                     pager     ? pager     : &ignore_id,
                     &ignore_word,	/* Flags */
                     eip       ? eip       : &ignore_word,
                     esp       ? esp       : &ignore_word);

  return 0;  
}

static int get_kernel_info_page (l4_threadid_t pager)
{
  int res;
#ifdef USE_L4ENV
  kinfo = l4util_kip_map();
  res = (kinfo) ? 0 : 1;
#else
  l4_snd_fpage_t fp;
  l4_msgdope_t dope;

  printf("pager: " l4util_idfmt "\n", l4util_idstr(pager));

  res = l4_ipc_call (pager,
                     L4_IPC_SHORT_MSG,
                     1, 1,
                     L4_IPC_MAPMSG ((l4_addr_t) kinfo, L4_LOG2_PAGESIZE),
                     &fp.snd_base,                                
                     &fp.fpage.fpage,
                     L4_IPC_NEVER,
                     &dope);
#endif
                     
  if (res)
    puts ("couldn't get KIP");
    
  return res;
}

static void set_prio (l4_threadid_t thread, int prio)
{
  l4_sched_param_t sched;
  l4_threadid_t s = L4_INVALID_ID;
    
  l4_thread_schedule (thread, L4_INVALID_SCHED_PARAM, &s, &s, &sched);
  sched.sp.prio  = prio;
  sched.sp.state = 0;
  sched.sp.small = 0;
  s = L4_INVALID_ID;
  l4_thread_schedule (thread, sched, &s, &s, &sched);
}

static void __attribute__ ((noreturn)) test_thread (void* arg)
{
  l4_threadid_t id = l4_next_period_id (l4_myself());
  l4_msgdope_t result;
  l4_umword_t dummy;
  int ret;

#ifdef USE_L4ENV
  if (l4thread_started(NULL) < 0)
    puts("Startup notification failed");
#endif

  if ((ret = l4_ipc_receive (id,
                             L4_IPC_SHORT_MSG, &dummy, &dummy,
                             L4_IPC_RECV_TIMEOUT_0,
                             &result)) != L4_IPC_RETIMEOUT)
     printf ("next_period returned %x\n", ret);
  
  else
    puts ("RT mode active");

  while (1);		/* burn time */
                                
  l4_sleep_forever();
  exit (1);             /* unreached */
}

int main (void)
{
  l4_threadid_t preempter, pager;
  l4_umword_t word1, word2;
  l4_msgdope_t result;
#ifdef USE_L4ENV
  l4thread_t th;
#endif
  int i;

  printf ("RT-Scheduling Demo...\n");

  get_thread_ids (&main_thread_id, &preempter, &pager, 0, 0);

  /* Get KIP */
  get_kernel_info_page (pager);

  set_prio (l4_myself(), 255);

  /* Create TEST thread */
#ifdef USE_L4ENV
  th = l4thread_create_long(L4THREAD_INVALID_ID,
      test_thread,
      ".sched",
      L4THREAD_INVALID_SP,
      L4THREAD_DEFAULT_SIZE,
      L4THREAD_DEFAULT_PRIO,
      NULL,
      L4THREAD_CREATE_SYNC);
  test_thread_id = l4thread_l4_id(th);
  /* set preemter */
  pager = L4_INVALID_ID;
  l4_thread_ex_regs_flags(test_thread_id, -1, -1,
      &main_thread_id, &pager,
      &word1, &word1, &word1, L4_THREAD_EX_REGS_NO_CANCEL);
#else
  test_thread_id            = main_thread_id;
  test_thread_id.id.lthread = TEST_THREAD_NUMBER;
        
  l4_thread_ex_regs (test_thread_id,			/* dest thread */
                     (l4_umword_t) test_thread,		/* new EIP */        
                     (l4_umword_t) test_thread_stack + sizeof (test_thread_stack),
                     &main_thread_id,			/* preempter */   
                     &pager,				/* pager */
                     &word1,				/* flags */
                     &word1,				/* old EIP */
                     &word1);				/* old ESP */
#endif

  printf ("\nSetting priority: %u\n", 5);
  set_prio (test_thread_id, 5);

  puts ("\nAdding RT timeslices...");
  for (i = 20; i; i--)
    {
      printf ("%u ", i);
      l4_rt_add_timeslice (test_thread_id, i, 100000);
    }
  puts ("");

  printf ("\nSetting period length to: %u usecs\n", 3000000);
  l4_rt_set_period (test_thread_id, 3000000);

  printf ("\nStarting periodic execution at time: %llu usecs\n",
          kinfo->clock + 2000000);
          
  l4_rt_begin_strictly_periodic (test_thread_id, kinfo->clock + 2000000);
  
  printf ("\nWaiting for Preemption-IPCs from %u.%u\n",
          test_thread_id.id.task, test_thread_id.id.lthread);
          
  for (;;)
    {
      if (l4_ipc_receive (l4_preemption_id (test_thread_id),
                          L4_IPC_SHORT_MSG,
                          &word1,
                          &word2,
                          L4_IPC_NEVER,
                          &result) == 0)
	/* Type: Timeslice_overrun = 1, Deadline_miss = 0 */
        printf ("Received P-IPC (Type:%lu ID:%lu, Time:%llu)\n",
                (word2 & 0x80000000U) >> 31,
                (word2 & 0x7f000000U) >> 24,
                ((unsigned long long) word2 << 32 | word1) &
                0xffffffffffffffULL);
      else
	printf("Receive retuned %lx\n", L4_IPC_ERROR(result));
    }

  l4_sleep_forever();
  
  return 0;
}
