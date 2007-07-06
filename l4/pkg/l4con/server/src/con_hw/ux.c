/*!
 * \file	ux.c
 * \brief	Active screen update pushing
 *
 * \date	11/2003
 * \author	Adam Lackorzynski <adam@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universität Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include <stdio.h>
#include <string.h>
#include <l4/env/errno.h>
#include <l4/sigma0/kip.h>
#include <l4/sys/types.h>
#include <l4/sys/vhw.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/ipc.h>
#include <l4/util/l4_macros.h>
#include <l4/util/util.h>
#include <l4/thread/thread.h>

#include <l4/lxfuxlibc/lxfuxlc.h>

#include "init.h"
#include "iomem.h"
#include "ux.h"

static lx_pid_t ux_con_pid;

static volatile int waiting;
static l4_threadid_t updater_id;

static void updater_thread(void *data)
{
  l4_umword_t d;
  l4_msgdope_t dope;
  l4_threadid_t id;

  while (1)
    {
      if (l4_ipc_wait(&id, L4_IPC_SHORT_MSG, &d, &d, L4_IPC_NEVER, &dope))
        printf("updater_thread: l4_ipc_wait failed\n");
      l4_sleep(30);
      lx_kill(ux_con_pid, LX_SIGUSR1);
      waiting = 0;
    }
}

static void
uxScreenUpdate(int x, int y, int w, int h)
{
  if (!waiting)
    {
      l4_msgdope_t dope;

      lx_kill(ux_con_pid, LX_SIGUSR1);
      waiting = 1;
      while (l4_ipc_send(updater_id, L4_IPC_SHORT_MSG,
                         0, 0, L4_IPC_BOTH_TIMEOUT_0, &dope))
        {
          l4_sleep(1);
        }
    }
}

#if 0
static void
uxRectFill(struct l4con_vc *vc,
           int sx, int sy, int width, int height, unsigned color)
{
  //printf("%s\n", __func__);
}

static void
uxRectCopy(struct l4con_vc *vc, int sx, int sy,
           int width, int height, int dx, int dy)
{
  //printf("%s\n", __func__);
}
#endif

int
ux_probe(con_accel_t *accel)
{
  l4_kernel_info_t *kip;
  struct l4_vhw_descriptor *vhw;
  struct l4_vhw_entry *vhwe;
  l4thread_t update_tid;

  if (!(kip = l4sigma0_kip_map(L4_INVALID_ID)))
    return -L4_ENOTFOUND;

  if (!l4sigma0_kip_kernel_is_ux())
    return -L4_ENOTFOUND;

  printf("Found Fiasco-UX\n");

  if (!(vhw = l4_vhw_get(kip)))
    return -L4_ENOTFOUND;

  if (!(vhwe = l4_vhw_get_entry_type(vhw, L4_TYPE_VHW_FRAMEBUFFER)))
    return -L4_ENOTFOUND;

  ux_con_pid = vhwe->provider_pid;
  accel->drty = uxScreenUpdate;
  accel->caps = ACCEL_POST_DIRTY;

  printf("Found VHW descriptor, provider is %d\n", ux_con_pid);

  /* The update thread needs to run with the same priority than as the vc
   * threads (otherwise the screen won't be updated) */
  update_tid = l4thread_create_long(L4THREAD_INVALID_ID,
                                    updater_thread, ".scr-upd",
                                    L4THREAD_INVALID_SP, L4THREAD_DEFAULT_SIZE,
                                    0xff, NULL,
                                    L4THREAD_CREATE_ASYNC);
  if (update_tid < 0)
    {
      printf("Could not create updater thread.\n");
      return -L4_ENOTHREAD;
    }

  updater_id = l4thread_l4_id(update_tid);

  if (hw_vid_mem_addr != vhwe->mem_start
      || hw_vid_mem_size != vhwe->mem_size)
    printf("!!! Memory area mismatch "l4_addr_fmt"(%lx) vs. "l4_addr_fmt
           "(%lx) ... continuing\n",
           hw_vid_mem_addr, hw_vid_mem_size, vhwe->mem_start, vhwe->mem_size);

  map_io_mem(hw_vid_mem_addr, hw_vid_mem_size, 0, "UX video",
             (l4_addr_t *)&hw_map_vid_mem_addr);

  /* Do not unmap KIP, l4con needs it */
  return 0;
}
