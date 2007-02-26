#include <vector>
#include <string>
#include "blacklist.h"

void BlacklistManager::allow(unsigned int src, unsigned int dest)
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

void BlacklistManager::deny(unsigned int src, unsigned int dest)
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

bool BlacklistManager::check(unsigned int src, unsigned int dest)
{
	CapDescriptor *d = getDescriptorForTask(src);
	if (d)
		return !d->has_cap(dest);

	return true;
}
