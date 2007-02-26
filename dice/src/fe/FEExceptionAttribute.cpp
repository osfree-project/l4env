/**
 *    \file    dice/src/fe/FEExceptionAttribute.cpp
 *    \brief   contains the implementation of the class CFEExceptionAttribute
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

#include "fe/FEExceptionAttribute.h"
#include "fe/FEIdentifier.h"
#include "File.h"

CFEExceptionAttribute::CFEExceptionAttribute(vector<CFEIdentifier*> *pExcepNames)
: CFEAttribute(ATTR_EXCEPTIONS)
{
    m_vExcepNames.swap(*pExcepNames);
    vector<CFEIdentifier*>::iterator iter;
    for (iter = m_vExcepNames.begin(); iter != m_vExcepNames.end(); iter++)
        (*iter)->SetParent(this);
}

CFEExceptionAttribute::CFEExceptionAttribute(CFEExceptionAttribute & src)
: CFEAttribute(src)
{
    vector<CFEIdentifier*>::iterator iter = src.m_vExcepNames.begin();
    for (; iter != src.m_vExcepNames.end(); iter++)
    {
        CFEIdentifier *pNew = (CFEIdentifier*)((*iter)->Clone());
        m_vExcepNames.push_back(pNew);
        pNew->SetParent(this);
    }
}

/** clean up the exception attribute (free the exception names */
CFEExceptionAttribute::~CFEExceptionAttribute()
{
    while (!m_vExcepNames.empty())
    {
        delete m_vExcepNames.back();
        m_vExcepNames.pop_back();
    }
}

/**    creates a copy of this object
 *    \return a copy of this object
 */
CObject *CFEExceptionAttribute::Clone()
{
    return new CFEExceptionAttribute(*this);
}

/**    returns a pointer to the first exception name
 *    \return a pointer to the first exception name
 */
vector<CFEIdentifier*>::iterator CFEExceptionAttribute::GetFirstExceptionName()
{
    return m_vExcepNames.begin();
}

/** \brief returns the next exception's name
 *  \param iter the pointer to the next eception name
 *  \return a reference to the next exception's name
 */
CFEIdentifier *CFEExceptionAttribute::GetNextExceptionName(vector<CFEIdentifier*>::iterator &iter)
{
    if (iter == m_vExcepNames.end())
        return 0;
    return *iter++;
}

/** serializes this object
 *    \param pFile the file to serialize from/to
 */
void CFEExceptionAttribute::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        pFile->PrintIndent("<attribute>exceptions(");
        vector<CFEIdentifier*>::iterator iter = GetFirstExceptionName();
        CFEIdentifier *pExc;
        bool bFirst = true;
        while ((pExc = GetNextExceptionName(iter)) != 0)
        {
            if (!bFirst)
            {
                *pFile << ", ";
                bFirst = false;
            }
            pFile->Print("%s", pExc->GetName().c_str());
        }
        pFile->Print(")</attribute>\n");
    }
}
