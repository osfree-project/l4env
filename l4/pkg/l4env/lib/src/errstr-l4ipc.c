/*!
 * \file   l4env/lib/src/errstr-v2x0.c
 * \brief  Error string handling for l4env, L4v2 and L4X0 IPC codes
 *
 * \date   08/06/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/crtx/ctor.h>
#include <l4/env/errno.h>
#include <l4/sys/ipc.h>

/* hack: symbols that can be referenced to suck in this object */
asm("l4env_err_ipcstrings:");

/* L4 IPC errors */
L4ENV_ERR_DESC_STATIC(l4env_err_ipcmsg,
  { L4_IPC_ENOT_EXISTENT, "IPC: Non-existing destination or source" },
  { L4_IPC_RETIMEOUT,   "IPC: Receive timeout" },
  { L4_IPC_SETIMEOUT,   "IPC: Send timeout" },
  { L4_IPC_RECANCELED,  "IPC: Receive operation canceled" },
  { L4_IPC_SECANCELED,  "IPC: Send operation canceled" },
  { L4_IPC_REMAPFAILED, "IPC: Map flexpage failed in receive operation" },
  { L4_IPC_SEMAPFAILED, "IPC: Map flexpage failed in send operation" },
  { L4_IPC_RESNDPFTO,   "IPC: Send-pagefault timeout in receive operation" },
  { L4_IPC_SESNDPFTO,   "IPC: Send-pagefault timeout in send operation" },
  { L4_IPC_RERCVPFTO,   "IPC: Receive-pagefault timeout in receive operation" },
  { L4_IPC_SERCVPFTO,   "IPC: Receive-pagefault timeout in send operation" },
  { L4_IPC_REABORTED,   "IPC: Receive operation aborted" },
  { L4_IPC_SEABORTED,   "IPC: Send operation aborted" },
  { L4_IPC_REMSGCUT,    "IPC: Receive message cut" },
  { L4_IPC_SEMSGCUT,    "IPC: Send message cut" }
);

static void l4env_err_register_ipc_codes(void)
{
  l4env_err_register_desc(&l4env_err_ipcmsg);
}

L4C_CTOR(l4env_err_register_ipc_codes, L4CTOR_BEFORE_BACKEND);

