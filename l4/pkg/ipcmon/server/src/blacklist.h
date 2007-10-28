/*!
 * \file   server/src/blacklist.h
 * \brief  IPCMon Blacklist manager
 *
 * \date   01/30/2007
 * \author doebel@os.inf.tu-dresden.de
 *
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __SERVER_SRC_BLACKLIST_H_
#define __SERVER_SRC_BLACKLIST_H_

#include <vector>
#include "capmanager.h"

/* Black list manager. Maintains a blacklist of prohibitted IPC channels,
 * allows everything else.
 */
class BlacklistManager : public CapManager
{
	public:
		BlacklistManager()  {};
		~BlacklistManager() {};

		virtual void allow(unsigned int src_task, unsigned int dest_task);
		virtual void deny(unsigned int src_task, unsigned int dest_task);
		virtual int check(unsigned int src_task, unsigned int dest_task);
};

#endif
