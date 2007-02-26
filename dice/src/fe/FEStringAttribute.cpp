/**
 *    \file    dice/src/fe/FEStringAttribute.cpp
 *    \brief   contains the implementation of the class CFEStringAttribute
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
#include "File.h"

CFEStringAttribute::CFEStringAttribute(ATTR_TYPE nType, string string)
:CFEAttribute(nType)
{
    m_String = string;
}

CFEStringAttribute::CFEStringAttribute(CFEStringAttribute & src)
:CFEAttribute(src)
{
    m_String = src.m_String;
}

/** cleans up the string attribute object */
CFEStringAttribute::~CFEStringAttribute()
{

}

/** retrieves the contained string
 *    \return a reference to the string, which is parameter of this attribute
 * Because the returned string is only a reference to the member data, please copy
 * the string before you manipulate it.
 */
string CFEStringAttribute::GetString()
{
    return m_String;
}

/** creates a copy of this object
 *    \return a copy of this object
 */
CObject *CFEStringAttribute::Clone()
{
    return new CFEStringAttribute(*this);
}

/** serializes this object
 *    \param pFile the file to serialize from/to
 */
void CFEStringAttribute::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        switch (m_nType)
        {
        case ATTR_UUID:
            pFile->PrintIndent("<attribute>uuid(%s)</attribute>\n", GetString().c_str());
            break;
        case ATTR_HELPFILE:
            pFile->PrintIndent("<attribute>helpfile(%s)</attribute>\n", GetString().c_str());
            break;
        case ATTR_HELPSTRING:
            pFile->PrintIndent("<attribute>helpstring(%s)</attribute>\n", GetString().c_str());
            break;
        case ATTR_DEFAULT_FUNCTION:
            pFile->PrintIndent("<attribute>default_function(%s)</attribute>\n", GetString().c_str());
            break;
        case ATTR_ERROR_FUNCTION:
            pFile->PrintIndent("<attribute>error_string(%s)</attribute>\n", GetString().c_str());
            break;
        case ATTR_ERROR_FUNCTION_CLIENT:
            pFile->PrintIndent("<attribute>error_string_client(%s)</attribute>\n", GetString().c_str());
            break;
        case ATTR_ERROR_FUNCTION_SERVER:
            pFile->PrintIndent("<attribute>error_string_server(%s)</attribute>\n", GetString().c_str());
            break;
        default:
            CFEAttribute::Serialize(pFile);
            break;
        }
    }
}
