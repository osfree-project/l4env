#ifndef __CAPMANAGER_BLACK_H
#define __CAPMANAGER_BLACK_H

#include <vector>
#include <string>
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
		virtual bool check(unsigned int src_task, unsigned int dest_task);
};

#endif
