/*!
 * \file   include/ipcmon.h
 * \brief  IPCMon library function declarations
 *
 * \date   01/30/2007
 * \author doebel@os.inf.tu-dresden.de
 *
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __INCLUDE_IPCMON_H_
#define __INCLUDE_IPCMON_H_

#include <l4/sys/types.h>

int l4ipcmon_allow(l4_threadid_t monitor, l4_taskid_t src, l4_taskid_t dest);
int l4ipcmon_allow_named(l4_threadid_t monitor, l4_taskid_t src, char *name);

int l4ipcmon_deny(l4_threadid_t monitor, l4_taskid_t src, l4_taskid_t dest);
int l4ipcmon_deny_named(l4_threadid_t monitor, l4_taskid_t src, char *name);

int l4ipcmon_query(l4_threadid_t monitor, l4_taskid_t src, l4_taskid_t dest);

#endif
