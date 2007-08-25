/**
 *    \file    dice/src/fe/FEArrayDeclarator.cpp
 *  \brief   contains the implementation of the class CFEArrayDeclarator
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

#include "fe/FEArrayDeclarator.h"
#include "fe/FEExpression.h"

CFEArrayDeclarator::CFEArrayDeclarator(CFEDeclarator * pDecl)
: CFEDeclarator(*pDecl)
{
    m_nType = DECL_ARRAY;
}

CFEArrayDeclarator::CFEArrayDeclarator(std::string sName, CFEExpression * pUpper)
: CFEDeclarator(DECL_IDENTIFIER, sName)
{
    m_nType = DECL_ARRAY;
    AddBounds(0, pUpper);
}

CFEArrayDeclarator::CFEArrayDeclarator(CFEArrayDeclarator & src)
: CFEDeclarator(src)
{
    vector<CFEExpression*>::iterator iter = src.m_vLowerBounds.begin();
    for (; iter != src.m_vLowerBounds.end(); iter++)
    {
        CFEExpression *pNew = (CFEExpression*)((*iter)->Clone());
        m_vLowerBounds.push_back(pNew);
        pNew->SetParent(this);
    }
    iter = src.m_vUpperBounds.begin();
    for (; iter != src.m_vUpperBounds.end(); iter++)
    {
        CFEExpression *pNew = (CFEExpression*)((*iter)->Clone());
        m_vUpperBounds.push_back(pNew);
        pNew->SetParent(this);
    }
}

/** cleans up the array declarator (deletes the bounds) */
CFEArrayDeclarator::~CFEArrayDeclarator()
{
    while (!m_vLowerBounds.empty())
    {
        delete m_vLowerBounds.back();
        m_vLowerBounds.pop_back();
    }
    while (!m_vUpperBounds.empty())
    {
        delete m_vUpperBounds.back();
        m_vUpperBounds.pop_back();
    }
}

/** retrieves a lower bound
 *  \param nDimension the requested dimension
 *  \return the lower boudn of the requested dimension
 * If the dimension is out of range 0 is returned.
 */
CFEExpression *CFEArrayDeclarator::GetLowerBound(unsigned int nDimension)
{
    if (nDimension > m_vLowerBounds.size() - 1)
        return 0;
    if (m_vLowerBounds[nDimension] &&
        (m_vLowerBounds[nDimension]->GetType() == EXPR_NONE))
        return 0;
    return m_vLowerBounds[nDimension];
}

/** retrieves a upper bound
 *  \param nDimension the requested dimension
 *  \return the upper bound of the requested dimension
 * If the dimension is out of range 0 is returned.
 */
CFEExpression *CFEArrayDeclarator::GetUpperBound(unsigned int nDimension)
{
    if (nDimension > m_vLowerBounds.size() - 1)
        return 0;
    if (m_vUpperBounds[nDimension] &&
        (m_vUpperBounds[nDimension]->GetType() == EXPR_NONE))
        return 0;
    return m_vUpperBounds[nDimension];
}

/** adds an array bound
 *  \param pLower the lower array bound
 *  \param pUpper the upper array bound
 *  \return the dimension these two bound have been added to (-1 if some error occured)
 */
int CFEArrayDeclarator::AddBounds(CFEExpression * pLower, CFEExpression * pUpper)
{
    if (m_vLowerBounds.size() != m_vUpperBounds.size())
        return -1;
    // if one of the arguments is 0 add a EXPR_NONE element to vector
    // vector does not accept 0 as members
    if (!pLower)
        pLower = new CFEExpression(EXPR_NONE);
    if (!pUpper)
        pUpper = new CFEExpression(EXPR_NONE);
    // add elements to vectors
    m_vLowerBounds.push_back(pLower);
    m_vUpperBounds.push_back(pUpper);
    // set parent relation
    pLower->SetParent(this);
    pUpper->SetParent(this);
    return m_vLowerBounds.size() - 1;
}

/** retrieves the maximal dimension
 *  \return the size f the bounds collection
 */
unsigned int CFEArrayDeclarator::GetDimensionCount()
{
    if (m_vLowerBounds.size() != m_vUpperBounds.size())
        return 0;
    return m_vLowerBounds.size();
}

/** creates a copy of this object
 *  \return a copy of this object
 */
CObject *CFEArrayDeclarator::Clone()
{
    return new CFEArrayDeclarator(*this);
}

/** \brief deletes a dimension from the vounds vectors
 *  \param nIndex the dimension to erase
 *
 * This function removes the bounds without returning any reference to them,
 * so if you like to know where there are (e.g. keep your memory clean) get
 * these expressions first.
 */
void CFEArrayDeclarator::RemoveBounds(unsigned int nIndex)
{
    if (m_vLowerBounds.size() != m_vUpperBounds.size())
        return;
    if (nIndex > m_vLowerBounds.size() - 1)
        return;
    vector<CFEExpression*>::iterator iter = m_vLowerBounds.begin();
    unsigned int i = 0;
    for (; iter != m_vLowerBounds.end(); iter++, i++)
    {
        if (i == nIndex)
        {
            m_vLowerBounds.erase(iter);
            break;
        }
    }
    i = 0;
    iter = m_vUpperBounds.begin();
    for (; iter != m_vUpperBounds.end(); iter++, i++)
    {
        if (i == nIndex)
        {
            m_vUpperBounds.erase(iter);
            break;
        }
    }
}

/** \brief replaces the expression at position nIndex with a new expression
 *  \param nIndex the index of the expression to replace
 *  \param pLower the new lower bound
 */
void CFEArrayDeclarator::ReplaceLowerBound(unsigned int nIndex, CFEExpression * pLower)
{
    if (m_vLowerBounds.size() != m_vUpperBounds.size())
        return;
    CFEExpression *pOld = m_vLowerBounds[nIndex];
    m_vLowerBounds[nIndex] = pLower;
    delete pOld;
    pLower->SetParent(this);
}

/** \brief replaces the expression at position nIndex with a new expression
 *  \param nIndex the index of the expression to replace
 *  \param pUpper the new upper bound
 */
void CFEArrayDeclarator::ReplaceUpperBound(unsigned int nIndex, CFEExpression * pUpper)
{
    if (m_vLowerBounds.size() != m_vUpperBounds.size())
        return;
    CFEExpression *pOld = m_vUpperBounds[nIndex];
    m_vUpperBounds[nIndex] = pUpper;
    delete pOld;
    pUpper->SetParent(this);
}
