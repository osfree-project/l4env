#include "capmanager.h"
#include <vector>
#include <l4/log/l4log.h>

CapManager::CapManager()
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

bool CapManager::check(unsigned int src_task, unsigned int dest_task)
{
	return false;
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
