/**
 *	\file	dice/src/fe/FEIntAttribute.cpp
 *	\brief	contains the implementation of the class CFEIntAttribute
 *
 *	\date	01/31/2001
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

IMPLEMENT_DYNAMIC(CFEIntAttribute) 

CFEIntAttribute::CFEIntAttribute(ATTR_TYPE nType, int nValue)
:CFEAttribute(nType)
{
    IMPLEMENT_DYNAMIC_BASE(CFEIntAttribute, CFEAttribute);

    m_nIntValue = nValue;
}

CFEIntAttribute::CFEIntAttribute(CFEIntAttribute & src)
:CFEAttribute(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFEIntAttribute, CFEAttribute);

    m_nIntValue = src.m_nIntValue;
}

/** cleans up the integer attribute */
CFEIntAttribute::~CFEIntAttribute()
{
    // nothing to clean up
}

/** retrieves the integer values of this attribute
 *	\return  the integer values of this attribute
 */
int CFEIntAttribute::GetIntValue()
{
    return m_nIntValue;
}

/**	creates a copy of this object
 *	\return a copy of this object
 */
CObject *CFEIntAttribute::Clone()
{
    return new CFEIntAttribute(*this);
}

/** \brief serializes this object
 *	\param pFile the file to serialize from/to
 */
void CFEIntAttribute::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        switch (m_nType)
        {
        case ATTR_HELPCONTEXT:
            pFile->PrintIndent("<attribute>helpcontext(%d)</attribute>\n", GetIntValue());
            break;
        case ATTR_LCID:
            pFile->PrintIndent("<attribute>lcid(%d)</attribute>\n", GetIntValue());
            break;
        case ATTR_UUID:
            pFile->PrintIndent("<attribute>uuid(%d)</attribute>\n", GetIntValue());
            break;
        case ATTR_SIZE_IS:
            pFile->PrintIndent("<attribute>size_is(%d)</attribute>\n", GetIntValue());
            break;
        case ATTR_FIRST_IS:
            pFile->PrintIndent("<attribute>first_is(%d)</attribute>\n", GetIntValue());
            break;
        case ATTR_LAST_IS:
            pFile->PrintIndent("<attribute>last_is(%d)</attribute>\n", GetIntValue());
            break;
        case ATTR_LENGTH_IS:
            pFile->PrintIndent("<attribute>length_is(%d)</attribute>\n", GetIntValue());
            break;
        case ATTR_MIN_IS:
            pFile->PrintIndent("<attribute>min_is(%d)</attribute>\n", GetIntValue());
            break;
        case ATTR_MAX_IS:
            pFile->PrintIndent("<attribute>max_is(%d)</attribute>\n", GetIntValue());
            break;
        case ATTR_IID_IS:
            pFile->PrintIndent("<attribute>iid_is(%d)</attribute>\n", GetIntValue());
            break;
        default:
            CFEAttribute::Serialize(pFile);
            break;
        }
    }
}
