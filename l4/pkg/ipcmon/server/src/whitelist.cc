#include <vector>
#include <string>
#include "whitelist.h"
#include <l4/log/l4log.h>

WhitelistManager::WhitelistManager(void)
{
}

WhitelistManager::~WhitelistManager(void)
{
}

void WhitelistManager::allow(unsigned int src, unsigned int dest)
{
	CapDescriptor *d = getDescriptorForTask(src);
	if (!d)
	{
		d = new CapDescriptor(src);
		this->_descriptors.push_back(d);
	}

	if (!d->has_cap(dest))
		d->add_cap(dest);
}

void WhitelistManager::deny(unsigned int src, unsigned int dest)
{
	CapDescriptor *d = getDescriptorForTask(src);
	
	if (!d)
	{
		d = new CapDescriptor(src);
		this->_descriptors.push_back(d);
	}

	if (d->has_cap(dest))
		d->remove_cap(dest);
}

bool WhitelistManager::check(unsigned int src, unsigned int dest)
{
	CapDescriptor *d = getDescriptorForTask(src);

	if (d)
		return d->has_cap(dest);

	return false;
}
