/*!
 * \file   server/src/capmanager.cc
 * \brief  Capability manager base implementation
 *
 * \date   01/30/2007
 * \author doebel@os.inf.tu-dresden.de
 *
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "capmanager.h"
#include <vector>
#include <l4/log/l4log.h>

CapManager::CapManager() : _descriptors()
{
}

CapManager::~CapManager()
{
}

void CapManager::allow(unsigned int src, unsigned int dest)
{
}

void CapManager::deny(unsigned int src_task, unsigned int dest_task)
{
}

int CapManager::check(unsigned int src_task, unsigned int dest_task)
{
	return IPC_FALSE;
}

CapDescriptor *CapManager::getDescriptorForTask(unsigned int task)
{
	std::vector<CapDescriptor *>::iterator it = this->_descriptors.begin();
	for ( ; it < this->_descriptors.end(); it++)
	{
		if ((*it)->task() == task)
			return *it;
	}
	return NULL;
}

CapDescriptor *CapManager::removeDescriptorForTask(unsigned int task)
{
	std::vector<CapDescriptor *>::iterator it = this->_descriptors.begin();
	for ( ; it < this->_descriptors.end(); it++)
	{
		if ((*it)->task() == task)
		{
			CapDescriptor *d = *it;
			this->_descriptors.erase(it);
			return d;
		}
	}

	return NULL;
}
