/**
 *    \file    dice/src/fe/FEIntAttribute.cpp
 *  \brief   contains the implementation of the class CFEIntAttribute
 *
 *    \date    01/31/2001
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "fe/FEIntAttribute.h"

CFEIntAttribute::CFEIntAttribute(ATTR_TYPE nType, int nValue, bool bAbs)
: CFEAttribute(nType),
	m_nIntValue(nValue),
	m_bAbsoluteValue(bAbs)
{ }

CFEIntAttribute::CFEIntAttribute(CFEIntAttribute* src)
: CFEAttribute(src),
	m_nIntValue(src->m_nIntValue),
	m_bAbsoluteValue(src->m_bAbsoluteValue)
{ }

/** cleans up the integer attribute */
CFEIntAttribute::~CFEIntAttribute()
{ }

/** \brief create a copy of this object
 *  \return reference to clone
 */
CFEIntAttribute* CFEIntAttribute::Clone()
{
	return new CFEIntAttribute(this);
}
