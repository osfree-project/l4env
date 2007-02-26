/**
 *	\file	dice/src/fe/FEExceptionAttribute.cpp
 *	\brief	contains the implementation of the class CFEExceptionAttribute
 *
 *	\date	01/31/2001
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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
#include "Vector.h"
#include "File.h"

IMPLEMENT_DYNAMIC(CFEExceptionAttribute) 

CFEExceptionAttribute::CFEExceptionAttribute(Vector * pExcepNames)
:CFEAttribute(ATTR_EXCEPTIONS)
{
    IMPLEMENT_DYNAMIC_BASE(CFEExceptionAttribute, CFEAttribute);

    m_pExcepNames = pExcepNames;
}

CFEExceptionAttribute::CFEExceptionAttribute(CFEExceptionAttribute & src)
:CFEAttribute(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFEExceptionAttribute, CFEAttribute);

    if (src.m_pExcepNames)
      {
	  m_pExcepNames = src.m_pExcepNames->Clone();
	  m_pExcepNames->SetParentOfElements(this);
      }
    else
	m_pExcepNames = 0;
}

/** clean up the exception attribute (free the exception names */
CFEExceptionAttribute::~CFEExceptionAttribute()
{
    if (m_pExcepNames)
	delete m_pExcepNames;
}

/**	creates a copy of this object
 *	\return a copy of this object
 */
CObject *CFEExceptionAttribute::Clone()
{
    return new CFEExceptionAttribute(*this);
}

/**	returns a pointer to the first exception name
 *	\return a pointer to the first exception name
 */
VectorElement *CFEExceptionAttribute::GetFirstExceptionName()
{
    if (!m_pExcepNames)
	return 0;
    return m_pExcepNames->GetFirst();
}

/** \brief returns the next exception's name
 *  \param iter the pointer to the next eception name
 *  \return a reference to the next exception's name
 */
CFEIdentifier *CFEExceptionAttribute::GetNextExceptionName(VectorElement * &iter)
{
    if (!m_pExcepNames)
	return 0;
    if (!iter)
	return 0;
    CFEIdentifier *pRet = (CFEIdentifier *) (iter->GetElement());
    iter = iter->GetNext();
    return pRet;
}

/** serializes this object
 *	\param pFile the file to serialize from/to
 */
void CFEExceptionAttribute::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        pFile->PrintIndent("<attribute>exceptions(");
        VectorElement *pIter = GetFirstExceptionName();
        CFEIdentifier *pExc;
        while ((pExc = GetNextExceptionName(pIter)) != 0)
        {
            pFile->Print("%s", (const char *) (pExc->GetName()));
            if (pIter)
                if (pIter->GetElement())
                    pFile->Print(", ");
        }
        pFile->Print(")</attribute>\n");
    }
}
