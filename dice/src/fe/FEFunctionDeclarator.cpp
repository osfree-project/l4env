/**
 *	\file	dice/src/fe/FEFunctionDeclarator.cpp
 *	\brief	contains the implementation of the class CFEFunctionDeclarator
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

#include "fe/FEFunctionDeclarator.h"
#include "fe/FEDeclarator.h"
#include "fe/FETypedDeclarator.h"
#include "Vector.h"
#include "File.h"

IMPLEMENT_DYNAMIC(CFEFunctionDeclarator) 

CFEFunctionDeclarator::CFEFunctionDeclarator(CFEDeclarator * pDecl, Vector * pParams)
:CFEDeclarator(DECL_NONE, (pDecl) ? (pDecl->GetName()) : String(), 0)
{
	IMPLEMENT_DYNAMIC_BASE(CFEFunctionDeclarator, CFEDeclarator);

	m_pDeclarator = pDecl;
	m_pParameters = pParams;
}

CFEFunctionDeclarator::CFEFunctionDeclarator(CFEFunctionDeclarator & src)
:CFEDeclarator(src)
{
	IMPLEMENT_DYNAMIC_BASE(CFEFunctionDeclarator, CFEDeclarator);

	if (src.m_pDeclarator)
	{
		m_pDeclarator = (CFEDeclarator *) (src.m_pDeclarator->Clone());
		m_pDeclarator->SetParent(this);
	}
	else
		m_pDeclarator = 0;
	if (src.m_pParameters)
	{
		m_pParameters = src.m_pParameters->Clone();
		m_pParameters->SetParentOfElements(this);
	}
	else
		m_pParameters = 0;
}

/** cleans up the function declarator */
CFEFunctionDeclarator::~CFEFunctionDeclarator()
{
	if (m_pDeclarator)
		delete m_pDeclarator;
	if (m_pParameters)
		delete m_pParameters;
}

/**	creates a copy of this object
 *	\return a copy of this object
 */
CObject *CFEFunctionDeclarator::Clone()
{
    return new CFEFunctionDeclarator(*this);
}

/** retrieves the declarator of this function
 *	\return the declarator of this function
 */
CFEDeclarator *CFEFunctionDeclarator::GetDeclarator()
{
    return m_pDeclarator;
}

/** retrieves a pointer to the first parameter
 *	\return a pointer to the first parameter
 */
VectorElement *CFEFunctionDeclarator::GetFirstParameter()
{
    if (!m_pParameters)
		return 0;
    return m_pParameters->GetFirst();
}

/** retrieves the next parameter
 *	\param iter a pointer to the nexct parameter
 *	\return a reference to th next parameter
 */
CFETypedDeclarator *CFEFunctionDeclarator::GetNextParameter(VectorElement * &iter)
{
    if (!m_pParameters)
		return 0;
    if (!iter)
		return 0;
    CFETypedDeclarator *pRet = (CFETypedDeclarator *) (iter->GetElement());
    iter = iter->GetNext();
    return pRet;
}

/** serializes this object
 *	\param pFile the file to serialize to/from
 */
void CFEFunctionDeclarator::Serialize(CFile * pFile)
{
	if (pFile->IsStoring())
	{
		pFile->PrintIndent("<function_declarator>\n");
		pFile->IncIndent();
		if (m_pDeclarator)
			m_pDeclarator->Serialize(pFile);
		VectorElement *pIter = GetFirstParameter();
		CFEBase *pElement;
		while ((pElement = GetNextParameter(pIter)))
		{
			pFile->PrintIndent("<parameter>\n");
			pFile->IncIndent();
			pElement->Serialize(pFile);
			pFile->DecIndent();
			pFile->PrintIndent("</parameter>\n");
		}
		pFile->DecIndent();
		pFile->PrintIndent("</function_declarator>\n");
	}
}
