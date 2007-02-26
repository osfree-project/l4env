/* $Id$ */
/*****************************************************************************/
/**
 * \file  lock/examples/test/main.c
 * \brief Lock test.
 *
 * \date   12/30/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/lock/lock.h>
#include <l4/thread/thread.h>
#include <l4/log/l4log.h>
#include <l4/util/rand.h>
#include <stdio.h>

char LOG_tag[9]="locktest";

#define NUM_THREADS 10
#define WAIT_MAX 100.0

int x = 0;

l4lock_t lock = L4LOCK_UNLOCKED_INITIALIZER;

static void 
test_fn(void * data)
{
  int wait,me;

  me = l4thread_myself();
  printf("started: %d\n",me);

  l4thread_started(NULL);

  while(1)
    {
      while(!l4lock_try_lock(&lock))
	l4thread_sleep(100);

      x = x + 1;

      wait = (int)(WAIT_MAX * ((float)l4util_rand()/(float)L4_RAND_MAX));
      l4thread_sleep(wait);

#if 1
      printf("%2d: %d (counter %d)\n",me,x,lock.sem.counter);
#endif
      x = x - 1;

      l4lock_unlock(&lock);

#if 1
      wait = (int)(WAIT_MAX * ((float)l4util_rand()/(float)L4_RAND_MAX));
      l4thread_sleep(wait);
#endif
    }
}

int main(void)
{
  int i;

  l4semaphore_init();

  for (i = 0; i < NUM_THREADS; i++)
    l4thread_create(test_fn,NULL,L4THREAD_CREATE_SYNC);

  l4thread_sleep(60000);

  enter_kdebug("done.");

  return 0;
}
