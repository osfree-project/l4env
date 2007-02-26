/**
 *    \file    dice/src/be/BEUnionCase.cpp
 *    \brief   contains the implementation of the class CBEUnionCase
 *
 *    \date    01/15/2002
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

#include "be/BEUnionCase.h"
#include "be/BEContext.h"
#include "be/BEExpression.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"

#include "fe/FEUnionCase.h"

CBEUnionCase::CBEUnionCase()
{
}

CBEUnionCase::CBEUnionCase(CBEUnionCase & src)
: CBETypedDeclarator(src)
{
    m_bDefault = src.m_bDefault;
    vector<CBEExpression*>::iterator iter;
    for (iter = src.m_vCaseLabels.begin(); iter != src.m_vCaseLabels.end(); iter++)
    {
        CBEExpression *pNew = (CBEExpression*)((*iter)->Clone());
        m_vCaseLabels.push_back(pNew);
        pNew->SetParent(this);
    }
}

/**    \brief destructor of this instance */
CBEUnionCase::~CBEUnionCase()
{
    while (!m_vCaseLabels.empty())
    {
        delete m_vCaseLabels.back();
        m_vCaseLabels.pop_back();
    }
}

/**    \brief creates the back-end structure for a union case
 *    \param pFEUnionCase the corresponding front-end union case
 *    \param pContext the context of the code generation
 *    \return true if code generation was successful
 */
bool CBEUnionCase::CreateBackEnd(CFEUnionCase * pFEUnionCase, CBEContext * pContext)
{
    assert(pFEUnionCase);
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
        vector<CFEExpression*>::iterator iter = pFEUnionCase->GetFirstUnionCaseLabel();
        CFEExpression *pFELabel;
        while ((pFELabel = pFEUnionCase->GetNextUnionCaseLabel(iter)) != 0)
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

/** \brief creates the union case
 *  \param pType the type of the union arm
 *  \param sName the name of the union arm
 *  \param pCaseLabel the case label
 *  \param bDefault true if this is the default arm
 *  \param pContext the context of the create process
 *  \return true if successful
 *
 * If neither pCaseLabel nor bDefault is set, then this is the member of a C
 * style union.
 */
bool CBEUnionCase::CreateBackEnd(CBEType *pType, string sName,
     CBEExpression *pCaseLabel, bool bDefault, CBEContext *pContext)
{
    VERBOSE("%s called for %s\n", __PRETTY_FUNCTION__, sName.c_str());

    if (!CBETypedDeclarator::CreateBackEnd(pType, sName, pContext))
    {
        VERBOSE("%s failed, because base class could not be initialized\n",
            __PRETTY_FUNCTION__);
        return false;
    }
    m_bDefault = bDefault;
    if (pCaseLabel)
        AddLabel(pCaseLabel);

    VERBOSE("%s returns true\n", __PRETTY_FUNCTION__);
    return true;
}

/**    \brief adds another label to the vector
 *    \param pLabel the new label
 */
void CBEUnionCase::AddLabel(CBEExpression * pLabel)
{
    if (!pLabel)
        return;
    m_vCaseLabels.push_back(pLabel);
    pLabel->SetParent(this);
}

/**    \brief removes a label from the labels list
 *    \param pLabel the label to remove
 */
void CBEUnionCase::RemoveLabel(CBEExpression * pLabel)
{
    if (!pLabel)
        return;
    vector<CBEExpression*>::iterator iter;
    for (iter = m_vCaseLabels.begin(); iter != m_vCaseLabels.end(); iter++)
    {
        if (*iter == pLabel)
        {
            m_vCaseLabels.erase(iter);
            return;
        }
    }
}

/**    \brief retrieves pointer to first label
 *    \return the pointer to the first label
 */
vector<CBEExpression*>::iterator CBEUnionCase::GetFirstLabel()
{
    return m_vCaseLabels.begin();
}

/**    \brief retrieves a reference to the next label
 *    \param iter the pointer to this label
 *    \return a reference to the expression
 */
CBEExpression *CBEUnionCase::GetNextLabel(vector<CBEExpression*>::iterator &iter)
{
    if (iter == m_vCaseLabels.end())
        return 0;
    return *iter++;
}

/** \brief returns true if this is the default case
 *  \return true if this is the default case
 */
bool CBEUnionCase::IsDefault()
{
    return m_bDefault;
}
