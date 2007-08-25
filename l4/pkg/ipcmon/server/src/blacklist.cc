/*!
 * \file   server/src/blacklist.cc
 * \brief  Blacklist manager implementation
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
