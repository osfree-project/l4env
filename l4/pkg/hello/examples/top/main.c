
#include <stdio.h>
#include <stdlib.h>

#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sigma0/kip.h>
#include <l4/util/util.h>
#include <l4/util/thread.h>

typedef struct
{
  l4_threadid_t  tid;
  l4_uint64_t    us;
  l4_mword_t     prio;
} thread_value_t;

static l4_umword_t ping_stack[L4_PAGESIZE/sizeof(l4_umword_t)];
static l4_umword_t pong_stack[L4_PAGESIZE/sizeof(l4_umword_t)];
static l4_threadid_t ping_id = L4_NIL_ID;
static l4_threadid_t pong_id = L4_NIL_ID;
static thread_value_t thread_value[64];

static inline void
show_top(void)
{
  l4_threadid_t tid;
  l4_threadid_t next_tid;
  l4_uint64_t us, total_us;
  l4_mword_t prio;
  int num, i;

restart:
  tid = L4_NIL_ID;
  num = 0;
  total_us = 0;

  do
    {
      if (fiasco_get_cputime(tid, &next_tid, &us, &prio) != 0)
	{
	  /* Error */
	  if (l4_is_nil_id(tid))
	    {
	      /* very strange: no accounting info for idle thread? */
	      printf("no accounting information for idle thread?");
	      exit(-3);
	    }
	  goto restart;
	}
      thread_value[num].tid  = tid;
      thread_value[num].us   = us;
      thread_value[num].prio = prio;
      if (++num > sizeof(thread_value)/sizeof(thread_value[0]))
	{
	  printf("too many threads");
	  exit(-4);
	}
      total_us += us;
      tid = next_tid;
    } while (!l4_is_nil_id(tid));

  printf("\033[H");
  for (i=0; i<num; i++)
    printf(" %3x.%02x @ 0x%02lx %4lld%% %11lldus\033[K\n",
	thread_value[i].tid.id.task,
	thread_value[i].tid.id.lthread,
	thread_value[i].prio,
	thread_value[i].us * 100 / total_us,
	thread_value[i].us);
}

static void
ping_thread(void)
{
  l4_umword_t dummy;
  l4_msgdope_t result;

  while (l4_is_nil_id(pong_id))
    l4_thread_switch(L4_NIL_ID);

  for (;;)
    l4_ipc_call(pong_id,
		L4_IPC_SHORT_MSG, 0, 0, 
		L4_IPC_SHORT_MSG, &dummy, &dummy,
		L4_IPC_NEVER, &result);
}

static void
pong_thread(void)
{
  l4_umword_t dummy;
  l4_msgdope_t result;

  while (l4_is_nil_id(ping_id))
    l4_thread_switch(L4_NIL_ID);

  l4_ipc_receive(ping_id, L4_IPC_SHORT_MSG, &dummy, &dummy,
		 L4_IPC_NEVER, &result);
  for (;;)
    l4_ipc_call(ping_id,
		L4_IPC_SHORT_MSG, 0, 0,
		L4_IPC_SHORT_MSG, &dummy, &dummy,
		L4_IPC_NEVER, &result);
}

int
main(void)
{
  l4_kernel_info_t *kip = l4sigma0_kip_map(L4_INVALID_ID);
  if (!kip)
    {
      printf("Cannot map kip");
      exit(-1);
    }
  if (kip->version != L4SIGMA0_KIP_VERSION_FIASCO)
    {
      printf("Only works with Fiasco!");
      exit(-2);
    }

  ping_id = l4util_create_thread(l4_myself().id.lthread+1, ping_thread, 
		      (int*)(ping_stack+L4_PAGESIZE/sizeof(l4_umword_t)));
  pong_id = l4util_create_thread(l4_myself().id.lthread+2, pong_thread, 
		      (int*)(pong_stack+L4_PAGESIZE/sizeof(l4_umword_t)));

  printf("\033[2J");
  for (;;)
    {
      l4_sleep(100);
      show_top();
    }
}

