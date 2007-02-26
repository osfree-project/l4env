/*!
 * \file   lib/src/libipcmon.c
 * \brief  IPCMon library wrappers
 *
 * \date   01/30/2007
 * \author doebel@os.inf.tu-dresden.de
 *
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/ipcmon/ipcmon.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/env/errno.h>

#include <stdlib.h>

#include "ipcmon-client.h"

int l4ipcmon_allow(l4_threadid_t monitor, l4_taskid_t src, l4_taskid_t dest)
{
	DICE_DECLARE_ENV(_env);
	return ipcmon_monitor_allow_call(&monitor, &src, &dest, &_env);
}

int l4ipcmon_allow_named(l4_threadid_t monitor, l4_taskid_t src, char *name)
{
	l4_threadid_t dest = L4_INVALID_ID;
	
	if (names_waitfor_name(name, &dest, 3000))
		return l4ipcmon_allow(monitor, src, dest);
	
	LOG("could not lookup %s", name);
	return -L4_EINVAL;
}

int l4ipcmon_deny(l4_threadid_t monitor, l4_taskid_t src, l4_taskid_t dest)
{
	DICE_DECLARE_ENV(_env);
	return ipcmon_monitor_deny_call(&monitor, &src, &dest, &_env);
}

int l4ipcmon_deny_named(l4_threadid_t monitor, l4_taskid_t src, char *name)
{
	l4_threadid_t dest = L4_INVALID_ID;

	if (names_waitfor_name(name, &dest, 3000))
		return l4ipcmon_deny(monitor, src, dest);
	
	LOG("could not lookup %s", name);
	return -L4_EINVAL;
}

int l4ipcmon_query(l4_threadid_t monitor, l4_taskid_t src, l4_taskid_t dest)
{
	DICE_DECLARE_ENV(_env);
	return ipcmon_monitor_query_call(&monitor, &src, &dest, &_env);
}
