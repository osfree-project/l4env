/*!
 * \file   server/src/capdescriptor.cc
 * \brief  Capability descriptor implementation
 *
 * \date   01/30/2007
 * \author doebel@os.inf.tu-dresden.de
 *
 */
/* (c) 2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "capdescriptor.h"

void CapDescriptor::add_cap(unsigned int dest)
{
	if (!has_cap(dest))
		_caps.push_back(dest);
}


void CapDescriptor::remove_cap(unsigned int task)
{
	std::vector<unsigned int>::iterator it;
	for (it = _caps.begin(); it < _caps.end(); it++) {
		if (*it == task)
			break;
	}
	if (it != _caps.end())
		_caps.erase(it);
}


bool CapDescriptor::has_cap(unsigned int task)
{
	std::vector<unsigned int>::iterator it;

	for (it = _caps.begin(); it < _caps.end(); it++) {
		if (*it == task)
			return true;
	}

    return false;
}


void CapDescriptor::dumpCaps(void)
{
	std::vector<unsigned int>::iterator it;

	LOG("caps size: %d", _caps.size());
	LOG("---");
	
	for (it = _caps.begin(); it < _caps.end(); it++)
		LOG("%d", *it);

	LOG("---");
}
