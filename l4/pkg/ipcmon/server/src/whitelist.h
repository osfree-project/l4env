#ifndef __CAPMANAGER_WHITE_H
#define __CAPMANAGER_WHITE_H

#include <vector>
#include <string>
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
		virtual bool check(unsigned int src_task, unsigned int dest_task);
};

#endif
