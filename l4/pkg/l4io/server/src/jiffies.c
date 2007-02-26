/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4io/server/src/jiffies.c
 * \brief  L4Env l4io I/O Server jiffies Thread
 *
 * \date   05/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/sys/kernel.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/util/util.h>
#include <l4/rmgr/librmgr.h>
#include <l4/env/errno.h>
#include <l4/sys/types.h>
#include <l4/l4rm/l4rm.h>
#include <l4/thread/thread.h>

/* local includes */
#include "io.h"
#include "jiffies.h"
#include "__config.h"
#include "__macros.h"


/*
 * vars
 */
static unsigned long long volatile *kclock; /**< kernel clock reference
                                             * \ingroup grp_misc */
/** address of L4 kernel info
 * \ingroup grp_misc */
static l4_addr_t kernel_info_addr = 0x1000;

/** Get RMGR pager id.
 * 
 * \retval pager         RMGR pager thread id
 *
 * Return the thread id of the RMGR pager.
 * We asume that we have a flat clan structure and our chief is the RMGR!
 */
static void __get_rmgr_pager(l4_threadid_t * pager)
{
  l4_threadid_t my_pager, my_preempter, chief;
  l4_umword_t dummy;

  /* get pager/preempter */
  my_preempter = my_pager = L4_INVALID_ID;
  l4_thread_ex_regs(l4_myself(), (l4_umword_t) - 1, (l4_umword_t) - 1, 
                    &my_preempter, &my_pager, &dummy, &dummy, &dummy);

  /* get chief */
  l4_nchief(L4_INVALID_ID, &chief);

  pager->raw = chief.raw;
}

/** Map kernel info page.
 *	
 * \return 0 on success, negative error code otherwise
 *
 * Map L4 kernel info page (L4 kernel timer as time base).
 */
static int __map_kernel_info_page(void)
{
  l4_threadid_t chief;
  int error;
  l4_umword_t dummy, magic;
  l4_msgdope_t result;
  l4_uint32_t kernel_info_areaid;

  __get_rmgr_pager(&chief);

  /* reserve kernel info page area */
  error = l4rm_area_reserve(L4_PAGESIZE, 0,
                            &kernel_info_addr, &kernel_info_areaid);
  if (error)
    {
      ERROR("kernel info page area already used!");
      return -L4_ENOMAP;
    }

  /* map kernel info page */
  error = l4_ipc_call(chief,
                           L4_IPC_SHORT_MSG, 1, 1,
                           L4_IPC_MAPMSG(kernel_info_addr, L4_LOG2_PAGESIZE),
                           &dummy, &dummy, L4_IPC_NEVER, &result);
  if (error)
    {
      ERROR("calling pager (0x%02x)", error);
      return -L4_EIPC;
    }

  magic = ((l4_kernel_info_t *) kernel_info_addr)->magic;

  if (!l4_ipc_fpage_received(result) || (magic != L4_KERNEL_INFO_MAGIC))
    {
      ERROR("mapping kernel info page");
      return -L4_ENOTFOUND;
    }

  DMSG("mapped kernel info page @ %p\n", (void*)kernel_info_addr);

  kclock = &((l4_kernel_info_t *) kernel_info_addr)->clock;

  return 0;
}

#if L4SCSI_JIFFIES
/** Jiffies thread loop.
 * \ingroup grp_misc
 *
 * \param  data         dummy data pointer (unused)
 *
 * This thread implements the LINUX jiffies counter.
 * It uses a timeouted \c l4_ipc_receive() on \c L4_NIL_ID.
 *
 * (another implementation can be selected by defining \c L4_SCSI_JIFFIES=0 in
 * jiffies.h)
 *
 * \krishna Lars' L4SCSI implementation
 */
static void jiffies_thread(void *data)
{
  int ms, to_e, to_m, ret;
  int error;
  l4_umword_t dummy;
  l4_msgdope_t result;

  ms = 1000 / IOJIFFIES_HZ;

  ret = micros2l4to(ms * 1000, &to_e, &to_m);
  if (ret)
    Panic("failed to calculate timeout!\n");

  /* I'm up */
  l4thread_started();

  /* jiffie loop */
  for (;;)
    {
      error = l4_ipc_receive(L4_NIL_ID, L4_IPC_SHORT_MSG,
                                  &dummy, &dummy,
                                  L4_IPC_TIMEOUT(0, 0, to_m, to_e, 0, 0), 
                                  &result);

      if (error != L4_IPC_RETIMEOUT)
        Panic("while receiving from NIL (IPC 0x%02x)", error);

      io_info.jiffies++;
      io_info.xtime.tv_sec = io_info.jiffies / HZ;
    }

  /* that schould never happen */
  Panic("left jiffies loop!");
}
#else
/** Jiffies thread loop.
 * \ingroup grp_misc
 *
 * \param  data         dummy data pointer (unused)
 *
 * This thread implements the LINUX jiffies counter.
 * It \c l4_sleep()s and tries to correct the period using the L4 kernel
 * clock.
 *
 * (another implementation can be selected by defining \c L4_SCSI_JIFFIES=1 in
 * jiffies.h)
 *
 * \lars test for (wait < 0) to increment jiffies by 2 or more if \c
 * JIFFIE_PERIOD is too short
 */
static void jiffies_thread(void *data)
{
  int wait;
  int next_tick = 0;

  /* I'm up */
  l4thread_started(NULL);

  next_tick = *kclock + 5 * IOJIFFIES_PERIOD; /* it's just the first tick */

  /* jiffie loop */
  for (;;)
    {
      wait = next_tick - *kclock;

      while (wait > 0)
        {
          l4thread_usleep((l4_uint32_t) wait);
          wait = next_tick - *kclock;
        }

      io_info.jiffies++;
      io_info.xtime.tv_sec = io_info.jiffies / IOJIFFIES_HZ;
      next_tick += IOJIFFIES_PERIOD;
    }

  /* that should never happen */
  Panic("left jiffies loop!\n");
}
#endif

/** Jiffies thread initialization.
 * \ingroup grp_misc
 *
 * \return 0 on success, negative error code otherwise
 *
 * Initialize jiffie counter thread.
 */
int io_jiffies_init()
{
  int error;
  l4thread_t jiffies_tid;

  /* map kernel info (time base) */
  if ((error = __map_kernel_info_page()))
    return error;

  /* create jiffies thread */
  jiffies_tid = l4thread_create((l4thread_fn_t) jiffies_thread,
				NULL, L4THREAD_CREATE_SYNC);

  if (jiffies_tid <= 0)
    {
      ERROR("create jiffies thread (%d)", jiffies_tid);
      return jiffies_tid;
    }

  return 0;
}
