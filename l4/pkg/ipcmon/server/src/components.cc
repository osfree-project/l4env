/*!
 * \file   server/src/components.cc
 * \brief  IDL component functions for the IPCMon server
 *
 * \date   01/30/2007
 * \author doebel@os.inf.tu-dresden.de
 *
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <vector>
#include <string>

#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <l4/names/libnames.h>
#include <l4/util/macros.h>
#include <l4/util/parse_cmd.h>
#include <l4/sys/types.h>

#include "ipcmon-server.h"
#include "whitelist.h"
#include "blacklist.h"

static CapManager   *theManager = NULL;
static bool         verbose     = false;

#define MT_USE_WHITELIST    1
#define MT_USE_BLACKLIST    2

int ipcmon_pagefault(CORBA_Object obj,
                     ipcmon_monitor_msg_buffer_t *msgbuf,
                     CORBA_Server_Environment *env)
{
	l4_fpage_t fp;
	l4_snd_fpage_t sfp;

	fp.raw = DICE_GET_DWORD(msgbuf, 0);
	LOGd(verbose, l4util_idfmt" trying to do IPC to %X", 
	    l4util_idstr(*obj), fp.iofp.iopage);

	if (theManager->check(obj->id.task, fp.iofp.iopage))
	{
		LOGd(verbose, "ipc allowed");
		sfp.fpage = l4_iofpage(fp.iofp.iopage, 0, L4_FPAGE_MAP);
		sfp.fpage.iofp.zero2 = 1;
		sfp.snd_base = 0;
	}
	else
	{
		LOG("\033[31;1mipc %X -> %X DENIED!\033[0m", (*obj).id.task, fp.iofp.iopage);
		sfp.fpage = l4_iofpage(0, 0, 0);
		sfp.snd_base  = 0;
	}

	DICE_MARSHAL_FPAGE(msgbuf, sfp, 0);
	DICE_SET_SHORTIPC_COUNT(msgbuf);
	DICE_SET_SEND_FPAGE(msgbuf);

	return DICE_REPLY;
}

int
ipcmon_monitor_query_component (CORBA_Object _dice_corba_obj,
                                const l4_taskid_t *src,
                                const l4_taskid_t *dest,
                                CORBA_Server_Environment *_dice_corba_env)
{
	if (theManager->check(src->id.task, dest->id.task))
		return 1;

	return 0;
}


int
ipcmon_monitor_allow_component (CORBA_Object _dice_corba_obj,
                                const l4_taskid_t *src,
                                const l4_taskid_t *dest,
                                CORBA_Server_Environment *_dice_corba_env)
{
	if (l4_task_equal(*_dice_corba_obj, *src))
		return -L4_EBADF;

	LOG("allowing %X -> %X", src->id.task, dest->id.task);
	theManager->allow(src->id.task, dest->id.task);
	return 0;
}

int
ipcmon_monitor_deny_component (CORBA_Object _dice_corba_obj,
                               const l4_taskid_t *src,
                               const l4_taskid_t *dest,
                               CORBA_Server_Environment *_dice_corba_env)
{
	l4_fpage_t fp;

	LOG("denying %X -> %X", src->id.task, dest->id.task);
	theManager->deny(src->id.task, dest->id.task);

	fp = l4_iofpage(dest->id.task, 0, L4_FPAGE_MAP);
	fp.iofp.zero2 = 1;
	l4_fpage_unmap(fp, 0);
 
	return 0;
}

int main(int argc, const char **argv)
{
	int manager_type = MT_USE_WHITELIST;
	DICE_DECLARE_ENV(env);

	if (parse_cmdline(&argc, &argv,
				'b', "blacklist", "blacklisting IPC monitor",
				PARSE_CMD_SWITCH, MT_USE_BLACKLIST, &manager_type,
				'w', "whitelist", "whitelisting IPC monitor",
				PARSE_CMD_SWITCH, MT_USE_WHITELIST, &manager_type,
				'v', "verbose", "verbose mode",
				PARSE_CMD_SWITCH, true, &verbose,
				0, 0))
		return 1;

	LOGd(verbose, "Verbose mode on.");
	LOGd(verbose, "manager type: %s",
	     manager_type == MT_USE_WHITELIST ? "whitelist" : "blacklist");

	LOGd(verbose, "registering at names");
	names_register("ipcmon");

	switch(manager_type) {
		case MT_USE_BLACKLIST:
			theManager = new BlacklistManager();
			break;
		case MT_USE_WHITELIST:
		default:
			theManager = new WhitelistManager();
			break;
	}

	LOG("IPC monitor is up and running.");
	ipcmon_monitor_server_loop(&env);

	delete theManager;

	return 0;
}
