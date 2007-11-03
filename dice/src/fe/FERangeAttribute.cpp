/**
 *  \file    dice/src/fe/FERangeAttribute.cpp
 *  \brief   contains the implementation of the class CFERangeAttribute
 *
 *  \date    10/08/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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

#include "fe/FERangeAttribute.h"

CFERangeAttribute::CFERangeAttribute(ATTR_TYPE nType, int nLower, int nUpper, bool bAbs)
: CFEAttribute(nType),
	m_nLower(nLower),
	m_nUpper(nUpper),
	m_bAbsolute(bAbs)
{ }

CFERangeAttribute::CFERangeAttribute(CFERangeAttribute* src)
: CFEAttribute(src),
	m_nLower(src->m_nLower),
	m_nUpper(src->m_nUpper),
	m_bAbsolute(src->m_bAbsolute)
{ }

/** cleans up the integer attribute */
CFERangeAttribute::~CFERangeAttribute()
{ }

/** \brief create a copy of this object
 *  \return reference to clone
 */
CFERangeAttribute* CFERangeAttribute::Clone()
{
	return new CFERangeAttribute(this);
}
