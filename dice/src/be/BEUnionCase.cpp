/**
 *	\file	dice/src/be/BEUnionCase.cpp
 *	\brief	contains the implementation of the class CBEUnionCase
 *
 *	\date	01/15/2002
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

#include "be/BEUnionCase.h"
#include "be/BEContext.h"
#include "be/BEExpression.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"

#include "fe/FEUnionCase.h"

IMPLEMENT_DYNAMIC(CBEUnionCase);

CBEUnionCase::CBEUnionCase():m_vCaseLabels(RUNTIME_CLASS(CBEExpression))
{
    IMPLEMENT_DYNAMIC_BASE(CBEUnionCase, CBETypedDeclarator);
}

CBEUnionCase::CBEUnionCase(CBEUnionCase & src):CBETypedDeclarator(src),
m_vCaseLabels(RUNTIME_CLASS(CBEExpression))
{
    IMPLEMENT_DYNAMIC_BASE(CBEUnionCase, CBETypedDeclarator);
}

/**	\brief destructor of this instance */
CBEUnionCase::~CBEUnionCase()
{
    m_vCaseLabels.DeleteAll();
}

/**	\brief creates the back-end structure for a union case
 *	\param pFEUnionCase the corresponding front-end union case
 *	\param pContext the context of the code generation
 *	\return true if code generation was successful
 */
bool CBEUnionCase::CreateBackEnd(CFEUnionCase * pFEUnionCase, CBEContext * pContext)
{
    if (!pFEUnionCase)
    {
        VERBOSE("CBEUnionCase::CreateBE failed because FE union case is 0\n");
        return false;
    }
    // the union arm is the typed declarator we initialize the base class with:
    if (!CBETypedDeclarator::CreateBackEnd(pFEUnionCase->GetUnionArm(), pContext))
    {
        VERBOSE("CBEUnionCase::CreateBE failed because base class could not be initialized\n");
        return false;
    }
    // now init union case specific stuff
    m_bDefault = pFEUnionCase->IsDefault();
    if (!m_bDefault)
    {
        VectorElement *pIter = pFEUnionCase->GetFirstUnionCaseLabel();
        CFEExpression *pFELabel;
        while ((pFELabel = pFEUnionCase->GetNextUnionCaseLabel(pIter)) != 0)
        {
            CBEExpression *pLabel = pContext->GetClassFactory()->GetNewExpression();
            AddLabel(pLabel);
            if (!pLabel->CreateBackEnd(pFELabel, pContext))
            {
                RemoveLabel(pLabel);
                delete pLabel;
                VERBOSE("CBEUnionCase::CreateBE failed because label could not be added\n");
                return false;
            }
        }
    }
    return true;
}

/**	\brief adds another label to the vector
 *	\param pLabel the new label
 */
void CBEUnionCase::AddLabel(CBEExpression * pLabel)
{
    if (!pLabel)
        return;
    m_vCaseLabels.Add(pLabel);
    pLabel->SetParent(this);
}

/**	\brief removes a label from the labels list
 *	\param pLabel the label to remove
 */
void CBEUnionCase::RemoveLabel(CBEExpression * pLabel)
{
    if (!pLabel)
        return;
    m_vCaseLabels.Remove(pLabel);
}

/**	\brief retrieves pointer to first label
 *	\return the pointer to the first label
 */
VectorElement *CBEUnionCase::GetFirstLabel()
{
    return m_vCaseLabels.GetFirst();
}

/**	\brief retrieves a reference to the next label
 *	\param pIter the pointer to this label
 *	\return a reference to the expression
 */
CBEExpression *CBEUnionCase::GetNextLabel(VectorElement * &pIter)
{
    if (!pIter)
        return 0;
    CBEExpression *pRet = (CBEExpression *) pIter->GetElement();
    pIter = pIter->GetNext();
    if (!pRet)
        return GetNextLabel(pIter);
    return pRet;
}

/**	\brief writes the union case to the target file
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The C translation of a union case does only contain the union arm.
 */
void CBEUnionCase::WriteDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    CBETypedDeclarator::WriteDeclaration(pFile, pContext);
}

/** \brief returns true if this is the default case
 *  \return true if this is the default case
 */
bool CBEUnionCase::IsDefault()
{
    return m_bDefault;
}
