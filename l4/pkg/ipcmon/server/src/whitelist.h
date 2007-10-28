/*!
 * \file   server/src/whitelist.h
 * \brief  IPCMon Whitelist server class
 *
 * \date   01/30/2007
 * \author doebel@os.inf.tu-dresden.de
 *
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __SERVER_SRC_WHITELIST_H_
#define __SERVER_SRC_WHITELIST_H_

#include <vector>
#include "capmanager.h"

#include <l4/log/l4log.h>

/* White list manager. Maintains a whitelist of interaction capabilities,
 * denies everything else.
 */
class WhitelistManager : public CapManager
{
	public:
		WhitelistManager();
		virtual ~WhitelistManager();

		virtual void allow(unsigned int src_task, unsigned int dest_task);
		virtual void deny(unsigned int src_task, unsigned int dest_task);
		virtual int check(unsigned int src_task, unsigned int dest_task);
};

#endif
