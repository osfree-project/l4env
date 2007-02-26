/* $Id$ */
/*****************************************************************************/
/**
 * \file  semaphore/examples/test/main.c
 * \brief Semaphore lib test application.
 *
 * \date   12/30/2000
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/

#include <l4/semaphore/semaphore.h>
#include <l4/thread/thread.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/kdebug.h>
#include <l4/log/l4log.h>
#include <l4/util/rand.h>
#include <l4/util/macros.h>
#include <stdio.h>

#define NUM_THREADS 10

#define WAIT_MAX 10000.0

l4semaphore_t sem = L4SEMAPHORE_INIT(2);

int x = 0;

static void 
test_fn(void * data)
{
  int wait,ret;
  l4thread_t me;
  l4_prio_t prio;

  me = l4thread_myself();
#if 1
  prio = (int)(255.0 * ((float)l4_rand()/(float)L4_RAND_MAX));
#else
  if (me > 10)
    prio = 10;
  else
    prio = 15;
#endif

  ret = l4thread_set_prio(me,prio);
  if (ret < 0)
    printf("failed to set priority: thread %d, prio %d\n",me,prio);

  printf("started: %d, prio %d\n",me,prio);
  
  l4thread_started(NULL);

  while(1)
    {
      l4semaphore_down(&sem);

      x = x + 1;

      wait = (int)(WAIT_MAX * ((float)l4_rand()/(float)L4_RAND_MAX));
      l4thread_sleep(wait);

#if 1
      printf("%2d: %d (counter %d)\n",me,x,sem.counter);
#endif

      x = x - 1;

      l4semaphore_up(&sem);

#if 1
      wait = (int)(WAIT_MAX * ((float)l4_rand()/(float)L4_RAND_MAX));
      l4thread_sleep(wait);
#endif
    }
}
      
int main(void)
{
  int i;
  l4_threadid_t me = l4_myself();
  l4_threadid_t other = me;

  LOG_init("sem_test");

#if 0
  l4semaphore_init();
#endif

  other.id.lthread = 5;
  printf("up, me = "IdFmt", other = "IdFmt"\n",IdStr(me),IdStr(other));

  for (i = 0; i < NUM_THREADS; i++)
    l4thread_create(test_fn,NULL,L4THREAD_CREATE_SYNC);

  l4thread_sleep(60000);

  enter_kdebug("done.");

  return 0;
}
