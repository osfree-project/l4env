/**
 *  \file   dice/src/fe/FETypeAttribute.cpp
 *  \brief  contains the implementation of the class CFETypeAttribute
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

#include "fe/FETypeAttribute.h"
#include "fe/FETypeSpec.h"
#include "File.h"

CFETypeAttribute::CFETypeAttribute(ATTR_TYPE nType, CFETypeSpec * pType)
: CFEAttribute(nType)
{
    m_pType = pType;
}

CFETypeAttribute::CFETypeAttribute(CFETypeAttribute & src)
: CFEAttribute(src)
{
    if (src.m_pType)
      {
      m_pType = (CFETypeSpec *) (src.m_pType->Clone());
      m_pType->SetParent(this);
      }
    else
    m_pType = 0;
}

/** cleans up the type attribute */
CFETypeAttribute::~CFETypeAttribute()
{
    if (m_pType)
    delete m_pType;
}

/** retrieves the contained type of the attribute
 *  \return the type, which is the parameter of this attribute */
CFETypeSpec *CFETypeAttribute::GetType()
{
    return m_pType;
}

/** creates a copy of this object
 *  \return a copy of this object
 */
CObject *CFETypeAttribute::Clone()
{
    return new CFETypeAttribute(*this);
}

/** serializes this object
 *  \param pFile the file to serialize to/from
 */
void CFETypeAttribute::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
      {
      switch (m_nType)
        {
        case ATTR_SWITCH_TYPE:
        pFile->PrintIndent("<attribute>switch_type(\n");
        GetType()->Serialize(pFile);
        pFile->PrintIndent(")</attribute>\n");
        break;
        case ATTR_TRANSMIT_AS:
        pFile->PrintIndent("<attribute>transmit_as(\n");
        GetType()->Serialize(pFile);
        pFile->PrintIndent(")</attribute>\n");
        break;
        default:
        CFEAttribute::Serialize(pFile);
        break;
        }
      }
}
