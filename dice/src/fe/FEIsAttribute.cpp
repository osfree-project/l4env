/**
 *	\file	dice/src/fe/FEIsAttribute.cpp
 *	\brief	contains the implementation of the class CFEIsAttribute
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

#include "fe/FEIsAttribute.h"

IMPLEMENT_DYNAMIC(CFEIsAttribute) 

CFEIsAttribute::CFEIsAttribute(ATTR_TYPE nType, Vector * pAttrParameters)
: CFEAttribute(nType)
{
    IMPLEMENT_DYNAMIC_BASE(CFEIsAttribute, CFEAttribute);
    m_pAttrParameters = pAttrParameters;
}

CFEIsAttribute::CFEIsAttribute(CFEIsAttribute & src)
: CFEAttribute(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFEIsAttribute, CFEAttribute);

    if (src.m_pAttrParameters)
    {
        m_pAttrParameters = src.m_pAttrParameters->Clone();
        m_pAttrParameters->SetParentOfElements(this);
    }
    else
        m_pAttrParameters = 0;
}

/** cleans up the attribute object (deletes parameters) */
CFEIsAttribute::~CFEIsAttribute()
{
    if (m_pAttrParameters)
        delete m_pAttrParameters;
}

/** retrieves a pointer to the first parameter
 *	\return an iterator, which points to the first parameter
 */
VectorElement *CFEIsAttribute::GetFirstAttrParameter()
{
    if (!m_pAttrParameters)
        return 0;
    return m_pAttrParameters->GetFirst();
}

/** retrieves the next parameter of the attribute
 *	\param iter the iterator, which points to the next parameter
 *	\return the next parameter
 */
CFEDeclarator *CFEIsAttribute::GetNextAttrParameter(VectorElement * &iter)
{
    if (!m_pAttrParameters)
        return 0;
    if (!iter)
        return 0;
    CFEDeclarator *pRet = (CFEDeclarator *) (iter->GetElement());
    iter = iter->GetNext();
    return pRet;
}

/** retireves the number of parameters
 *	\return the size of the parameter collection
 */
int CFEIsAttribute::GetParameterCount()
{
    if (!m_pAttrParameters)
        return 0;
    return m_pAttrParameters->GetSize();
}

/** creates a copy of this object
 *	\return a copy of this object
 */
CObject *CFEIsAttribute::Clone()
{
    return new CFEIsAttribute(*this);
}

/** serializes this object
 *	\param pFile the file to serialize to/from
 */
void CFEIsAttribute::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        switch (m_nType)
        {
        case ATTR_SWITCH_IS:
        case ATTR_FIRST_IS:
        case ATTR_LAST_IS:
        case ATTR_LENGTH_IS:
        case ATTR_MIN_IS:
        case ATTR_MAX_IS:
        case ATTR_SIZE_IS:
        case ATTR_IID_IS:
            {
                pFile->PrintIndent("<attribute>");
                switch (m_nType)
                {
                case ATTR_SWITCH_IS:
                    pFile->Print("switch_is ");
                    break;
                case ATTR_FIRST_IS:
                    pFile->Print("first_is");
                    break;
                case ATTR_LAST_IS:
                    pFile->Print("last_is");
                    break;
                case ATTR_LENGTH_IS:
                    pFile->Print("length_is");
                    break;
                case ATTR_MIN_IS:
                    pFile->Print("min_is");
                    break;
                case ATTR_MAX_IS:
                    pFile->Print("max_is");
                    break;
                case ATTR_SIZE_IS:
                    pFile->Print("size_is");
                    break;
                case ATTR_IID_IS:
                    pFile->Print("iid_is");
                    break;
                default:
                    break;
                }
                pFile->Print("(");
                VectorElement *pIter = GetFirstAttrParameter();
                CFEDeclarator *pDecl;
                while ((pDecl = GetNextAttrParameter(pIter)) != 0)
                {
                    pFile->Print("%s", (const char *) pDecl->GetName());
                    if (pIter)
                        if (pIter->GetElement())
                            pFile->Print(", ");
                }
                pFile->Print(")</attribute>\n");
            }
            break;
        default:
            CFEAttribute::Serialize(pFile);
            break;
        }
    }
}
