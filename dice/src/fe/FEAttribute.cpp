/**
 *  \file    dice/src/fe/FEAttribute.cpp
 *  \brief   contains the implementation of the class CFEAttribute
 *
 *  \date    01/31/2001
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "FEAttribute.h"
#include <iostream>

CFEAttribute::CFEAttribute()
{
    m_nType = ATTR_NONE;
}

CFEAttribute::CFEAttribute(ATTR_TYPE nType)
: m_nType(nType)
{
}

CFEAttribute::CFEAttribute(CFEAttribute* src)
: CFEBase(src)
{
    m_nType = src->m_nType;
}

/** cleans up the attribute */
CFEAttribute::~CFEAttribute()
{ }

/** \brief create a copy of this object
 *  \return reference to clone
 */
CFEAttribute* CFEAttribute::Clone()
{
	return new CFEAttribute(this);
}

/** returns the attribute's type
 *  \return the attribute's type
 */
ATTR_TYPE CFEAttribute::GetAttrType()
{
    return m_nType;
}

/** return true if attribute type matches
 *  \param type the type to test
 *  \return true on match
 */
bool CFEAttribute::Match(ATTR_TYPE type)
{
    return m_nType == type;
}
