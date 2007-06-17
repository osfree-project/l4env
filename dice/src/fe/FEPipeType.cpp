/**
 *  \file   dice/src/fe/FEPipeType.cpp
 *  \brief  contains the implementation of the class CFEPipeType
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

#include "fe/FEPipeType.h"

CFEPipeType::CFEPipeType(CFETypeSpec * pType)
:CFEConstructedType(TYPE_PIPE)
{
    m_pType = pType;
}

CFEPipeType::CFEPipeType(CFEPipeType & src)
:CFEConstructedType(src)
{
    if (src.m_pType)
      {
      m_pType = (CFETypeSpec *) (src.m_pType->Clone());
      m_pType->SetParent(this);
      }
    else
    m_pType = 0;
}

/** cleans up the pipe type object */
CFEPipeType::~CFEPipeType()
{
    if (m_pType)
    delete m_pType;
}

/** copies this object
 *  \return a reference to a new pipe type object
 */
CObject *CFEPipeType::Clone()
{
    return new CFEPipeType(*this);
}
