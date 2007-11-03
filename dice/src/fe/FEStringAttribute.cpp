/**
 *    \file    dice/src/fe/FEStringAttribute.cpp
 *  \brief   contains the implementation of the class CFEStringAttribute
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

#include "fe/FEStringAttribute.h"

CFEStringAttribute::CFEStringAttribute(ATTR_TYPE nType, std::string string)
 : CFEAttribute(nType)
{
    m_String = string;
}

CFEStringAttribute::CFEStringAttribute(CFEStringAttribute* src)
:CFEAttribute(src)
{
    m_String = src->m_String;
}

/** cleans up the string attribute object */
CFEStringAttribute::~CFEStringAttribute()
{ }

/** \brief create a copy of this object
 *  \return reference to clone
 */
CFEStringAttribute* CFEStringAttribute::Clone()
{
	return new CFEStringAttribute(this);
}
