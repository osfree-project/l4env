/**
 *  \file    dice/src/fe/FEEndPointAttribute.cpp
 *  \brief   contains the implementation of the class CFEEndPointAttribute
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

#include "fe/FEEndPointAttribute.h"

CFEEndPointAttribute::CFEEndPointAttribute(vector<PortSpec> *pPortSpecs)
: CFEAttribute(ATTR_ENDPOINT),
    m_PortSpecs(*pPortSpecs)
{
}

CFEEndPointAttribute::CFEEndPointAttribute(CFEEndPointAttribute & src)
: CFEAttribute(src),
    m_PortSpecs(src.m_PortSpecs)
{ }

/** cleans up the end-point attribute (delete all port-specs) */
CFEEndPointAttribute::~CFEEndPointAttribute()
{
}

/** creates a copy of this object
 *  \return a copy of this object
 */
CObject* CFEEndPointAttribute::Clone()
{
    return new CFEEndPointAttribute(*this);
}

