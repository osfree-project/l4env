/**
 *  \file   dice/src/fe/FEVersionAttribute.cpp
 *  \brief  contains the implementation of the class CFEVersionAttribute
 *
 *  \date   01/31/2001
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2004
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2 as
 * published by the Free Software Foundation (see the file COPYING).
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * For different licensing schemes please contact
 * <contact@os.inf.tu-dresden.de>.
 */

#include "fe/FEVersionAttribute.h"

CFEVersionAttribute::CFEVersionAttribute(int nMajor, int nMinor)
: CFEAttribute(ATTR_VERSION)
{
    m_nMajor = nMajor;
    m_nMinor = nMinor;
}

CFEVersionAttribute::CFEVersionAttribute(version_t version)
: CFEAttribute(ATTR_VERSION)
{
    m_nMajor = version.nMajor;
    m_nMinor = version.nMinor;
}

CFEVersionAttribute::CFEVersionAttribute(CFEVersionAttribute* src)
: CFEAttribute(src)
{
    m_nMajor = src->m_nMajor;
    m_nMinor = src->m_nMinor;
}

/** clleans up the version attribute */
CFEVersionAttribute::~CFEVersionAttribute()
{ }

/** \brief create a copy of this object
 *  \return reference to clone
 */
CFEVersionAttribute* CFEVersionAttribute::Clone()
{
	return new CFEVersionAttribute(this);
}

/** retrieves the version from the attribute
 *  \return a version_t structure containing the version numbers
 */
version_t CFEVersionAttribute::GetVersion()
{
    version_t v;
    v.nMajor = m_nMajor;
    v.nMinor = m_nMinor;
    return v;
}

/** retrieves the version from the attribute
 *  \param major will receive the major version number
 *  \param minor will receive the minor version number
 */
void CFEVersionAttribute::GetVersion(int &major, int &minor)
{
    major = m_nMajor;
    minor = m_nMinor;
}
