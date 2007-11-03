/**
 *  \file   dice/src/fe/FEPtrDefaultAttribute.cpp
 *  \brief  contains the implementation of the class CFEPtrDefaultAttribute
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

#include "fe/FEPtrDefaultAttribute.h"

CFEPtrDefaultAttribute::CFEPtrDefaultAttribute(CFEAttribute * pPtrAttr)
:CFEAttribute(ATTR_POINTER_DEFAULT)
{
    m_pPtrAttr = pPtrAttr;
}

CFEPtrDefaultAttribute::CFEPtrDefaultAttribute(CFEPtrDefaultAttribute* src)
:CFEAttribute(src)
{
	CLONE_MEM(CFEAttribute, m_pPtrAttr);
}

/** cleans up the pointer-default attribute object */
CFEPtrDefaultAttribute::~CFEPtrDefaultAttribute()
{
	if (m_pPtrAttr)
		delete m_pPtrAttr;
}

/** \brief create a copy of this object
 *  \return reference to clone
 */
CFEPtrDefaultAttribute* CFEPtrDefaultAttribute::Clone()
{
	return new CFEPtrDefaultAttribute(this);
}

/** retrieves the pointer attribute
 *  \return the pointer attribute
 */
CFEAttribute *CFEPtrDefaultAttribute::GetPtrAttribute()
{
    return m_pPtrAttr;
}
