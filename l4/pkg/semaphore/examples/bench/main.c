/* $Id$ */
/*****************************************************************************/
/**
 * \file   semaphore/examples/bench/main.c
 * \brief  Some tests to measure semaphore performance
 *
 * \date   02/20/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4/L4Env includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/util/rdtsc.h>
#include <l4/log/l4log.h>
#include <l4/thread/thread.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/util/util.h>
#include <l4/util/macros.h>

/* Sempahore includes */
#include <l4/semaphore/semaphore.h>

#define SEM_THREAD_PRIO  255

#define TEST_NUM         200
#define TEST_RDTSC_NUM   500
#define SEM_INIT         0 

#define DO_DOWN          1

l4semaphore_t sem;

volatile l4_cpu_time_t down_out;
volatile l4_cpu_time_t up_in;
volatile l4_cpu_time_t up_out;

unsigned long time_up,min_up,max_up;
unsigned long time_wakeup,min_wakeup,max_wakeup;

char LOG_tag[9] = "sem_be";

/*****************************************************************************/
/**
 * \brief Down-thread
 */
/*****************************************************************************/ 
static void
down_thread(void * data)
{
  while (1)
    {
      l4semaphore_down(&sem);
      down_out = l4_rdtsc();
    }
}

/*****************************************************************************/
/**
 * \brief Up-thread
 */
/*****************************************************************************/ 
static void
up_thread(void * data)
{
  int i;
  unsigned long diff;
  l4_threadid_t parent = l4thread_l4_id(l4thread_get_parent());
  l4_umword_t dummy;
  l4_msgdope_t result;

  /* wait for start message */
  l4_ipc_receive(parent,L4_IPC_SHORT_MSG,&dummy,&dummy,L4_IPC_NEVER,
		 &result);

  l4thread_sleep(500);

  time_up = time_wakeup = 0;
  min_up = min_wakeup = -1;
  max_up = max_wakeup = 0;

  for (i = 0; i < TEST_NUM; i++)
    {
      //enter_kdebug("go..");

      up_in = l4_rdtsc();
      l4semaphore_up(&sem);
      up_out = l4_rdtsc();

      l4thread_sleep(50);

      //enter_kdebug("done.");

      diff = (unsigned long)(up_out - up_in);
      time_up += diff;
      if (diff < min_up)
	min_up = diff;
      if (diff > max_up)
	max_up = diff;

#if DO_DOWN
      diff = (unsigned long)(down_out - up_in);
      time_wakeup += diff;
      if (diff < min_wakeup)
	min_wakeup = diff;
      if (diff > max_wakeup)
	max_wakeup = diff;

#endif
    }

  printf("  up:       %4lu/%4lu/%4lu\n",
         min_up,(unsigned long)(time_up / TEST_NUM),max_up);
#if DO_DOWN
  printf("  wakeup:   %4lu/%4lu/%4lu\n",
         min_wakeup,(unsigned long)(time_wakeup / TEST_NUM),max_wakeup);
#endif
  printf("\n");
  
  /* done */
  l4_ipc_send(parent,L4_IPC_SHORT_MSG,0,0,L4_IPC_NEVER,&result);

  l4thread_sleep(10000);
}

/*****************************************************************************/
/**
 * \brief  Run test
 * 
 * \param  up_prio       Up-thread priority
 * \param  down_prio     Down-thread priority
 */
/*****************************************************************************/ 
static void
do_test(l4_prio_t up_prio, l4_prio_t down_prio)
{
  l4thread_t up, down;
  l4_umword_t dummy;
  l4_msgdope_t result;

  LOGL("test:");

  /* initialize semaphore */
  sem = L4SEMAPHORE_INIT(SEM_INIT);

  /* start down thread */
#if DO_DOWN
  down = l4thread_create_long(L4THREAD_INVALID_ID,down_thread,
			      L4THREAD_INVALID_SP,L4THREAD_DEFAULT_SIZE,
			      down_prio,NULL,L4THREAD_CREATE_ASYNC);
  if (down < 0)
    {
      Error("start down thread failed: %s (%d)!",l4env_errstr(down),down);
      return;
    }
  printf("  down thread %x.%x, prio %u\n",
         l4thread_l4_id(down).id.task,l4thread_l4_id(down).id.lthread,down_prio);
#endif

  /* start up thread */
  up = l4thread_create_long(L4THREAD_INVALID_ID,up_thread,L4THREAD_INVALID_SP,
			    L4THREAD_DEFAULT_SIZE,up_prio,NULL,
			    L4THREAD_CREATE_ASYNC);
  if (up < 0)
    {
      Error("start up thread failed: %s (%d)!",l4env_errstr(up),up);
      return;
    }
  printf("  up thread   %x.%x, prio %u\n",
         l4thread_l4_id(up).id.task,l4thread_l4_id(up).id.lthread,up_prio);

  /* start test */
  l4_ipc_call(l4thread_l4_id(up),L4_IPC_SHORT_MSG,0,0,
	      L4_IPC_SHORT_MSG,&dummy,&dummy,L4_IPC_NEVER,&result); 

  /* test finished */
  l4thread_shutdown(up);
#if DO_DOWN
  l4thread_shutdown(down);
#endif
}

/*****************************************************************************/
/**
 * \brief Estimate how long rdtsc takes
 */
/*****************************************************************************/ 
static void
test_rdtsc(void)
{
  int i;
  l4_cpu_time_t start,end,mid;
  l4_cpu_time_t diff,min1,min2,min3,max1,max2,max3;
  l4_cpu_time_t total1,total2,total3;

  total1 = total2 = total3 = 0;
  min1 = min2 = min3 = -1;
  max1 = max2 = max3 = 0;
  for (i = 0; i < TEST_RDTSC_NUM; i++)
    {
      start = l4_rdtsc();
      mid = l4_rdtsc();
      end = l4_rdtsc();
      
      diff = (unsigned long)(mid - start);
      total1 += diff;

      if (diff < min1)
	min1 = diff;
      if (diff > max1)
	max1 = diff;

      diff = (unsigned long)(end - mid);
      total2 += diff;

      if (diff < min2)
	min2 = diff;
      if (diff > max2)
	max2 = diff;

      diff = (unsigned long)(end - start);
      total3 += diff;

      if (diff < min3)
	min3 = diff;
      if (diff > max3)
	max3 = diff;
      
    }
  
  LOGL("rdtsc:\n  1-2: %lu/%lu/%lu 2-3: %lu/%lu/%lu, 1-3: %lu/%lu/%lu\n",
       min1,(unsigned long)(total1 / TEST_RDTSC_NUM),max1,
       min2,(unsigned long)(total2 / TEST_RDTSC_NUM),max2,
       min3,(unsigned long)(total3 / TEST_RDTSC_NUM),max3);
}

/*****************************************************************************/
/**
 * \brief Main
 */
/*****************************************************************************/ 
int main(int argc, char * argv[])
{
  extern char _stext;
  extern char _etext;
  extern char _end;
  
  /* set semaphore thread priority */
  l4semaphore_set_thread_prio(SEM_THREAD_PRIO);

  /* prevent page faults */
  l4_touch_ro(&_stext, &_etext-&_stext);
  l4_touch_rw(&_etext, &_end-&_etext);

  test_rdtsc();

  /* do tests */
#if 1
  do_test(30,     /* prio up-thread */
	  20      /* prio down-thread */
	  );
#endif

#if 0
  do_test(30,20);

  do_test(30,20);
#endif

  KDEBUG("done.");

  return 0;
}
