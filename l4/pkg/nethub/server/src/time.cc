/*
 * Copyright (C) 2004  Alexander Warg  <alexander.warg@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the nethub package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */
#include "time.h"
#include <l4/sys/ipc.h>

l4_kernel_info_t *Time::kip;

void Time::init()
{
  extern char _end[];
  kip = (l4_kernel_info_t*)((l4_umword_t)(_end + 4095) & 0xfffff000);
  l4_threadid_t sigma0;
  sigma0.id.lthread = 0;
  sigma0.id.task = 2;

  l4_msgdope_t res;
  l4_umword_t dummy1, dummy2;
  
  int e = l4_ipc_call(sigma0, L4_IPC_SHORT_MSG, 1, 1,
                      L4_IPC_MAPMSG((l4_addr_t)kip, 12),
        	      &dummy1, &dummy2, L4_IPC_NEVER, &res);
  if (e || !l4_ipc_fpage_received(res))
    kip = 0;
}
