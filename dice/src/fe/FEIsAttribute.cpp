/**
 *    \file    dice/src/fe/FEIsAttribute.cpp
 *    \brief   contains the implementation of the class CFEIsAttribute
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

#include "fe/FEIsAttribute.h"
#include "fe/FEDeclarator.h"
#include "File.h"

CFEIsAttribute::CFEIsAttribute(ATTR_TYPE nType, vector<CFEDeclarator*> *pAttrParameters)
: CFEAttribute(nType)
{
    if (pAttrParameters)
        m_vAttrParameters.swap(*pAttrParameters);
    vector<CFEDeclarator*>::iterator iter;
    for (iter = m_vAttrParameters.begin();
         iter != m_vAttrParameters.end(); iter++)
    {
        (*iter)->SetParent(this);
    }
}

CFEIsAttribute::CFEIsAttribute(CFEIsAttribute & src)
: CFEAttribute(src)
{
    vector<CFEDeclarator*>::iterator iter = src.m_vAttrParameters.begin();
    for (; iter != src.m_vAttrParameters.end(); iter++)
    {
        CFEDeclarator *pNew = (CFEDeclarator*)((*iter)->Clone());
        m_vAttrParameters.push_back(pNew);
        pNew->SetParent(this);
    }
}

/** cleans up the attribute object (deletes parameters) */
CFEIsAttribute::~CFEIsAttribute()
{
    while (!m_vAttrParameters.empty())
    {
        delete m_vAttrParameters.back();
        m_vAttrParameters.pop_back();
    }
}

/** retrieves a pointer to the first parameter
 *    \return an iterator, which points to the first parameter
 */
vector<CFEDeclarator*>::iterator CFEIsAttribute::GetFirstAttrParameter()
{
    return m_vAttrParameters.begin();
}

/** retrieves the next parameter of the attribute
 *    \param iter the iterator, which points to the next parameter
 *    \return the next parameter
 */
CFEDeclarator *CFEIsAttribute::GetNextAttrParameter(vector<CFEDeclarator*>::iterator &iter)
{
    if (iter == m_vAttrParameters.end())
        return 0;
    return *iter++;
}

/** retireves the number of parameters
 *    \return the size of the parameter collection
 */
int CFEIsAttribute::GetParameterCount()
{
    return m_vAttrParameters.size();
}

/** creates a copy of this object
 *    \return a copy of this object
 */
CObject *CFEIsAttribute::Clone()
{
    return new CFEIsAttribute(*this);
}

/** serializes this object
 *    \param pFile the file to serialize to/from
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
                vector<CFEDeclarator*>::iterator iter = GetFirstAttrParameter();
                CFEDeclarator *pDecl;
                bool bComma = false;
                while ((pDecl = GetNextAttrParameter(iter)) != 0)
                {
                    if (bComma)
                        *pFile << ", ";
                    *pFile << pDecl->GetName();
                    bComma = true;
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
