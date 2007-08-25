/**
 *    \file    dice/src/fe/FESizeOfExpression.cpp
 *  \brief   contains the implementation of the class CFESizeOfExpression
 *
 *    \date    06/01/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004
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

#include "fe/FESizeOfExpression.h"
#include "fe/FETypeSpec.h"
#include "Visitor.h"

CFESizeOfExpression::CFESizeOfExpression()
 : CFEExpression()
{
    m_pType = 0;
    m_pExpression = 0;
}

/** destroys this object */
CFESizeOfExpression::~CFESizeOfExpression()
{
    if (m_pType)
        delete m_pType;
    if (m_pExpression)
        delete m_pExpression;
}

CFESizeOfExpression::CFESizeOfExpression(std::string sTypeName)
 : CFEExpression(sTypeName)
{
    m_pType = 0;
    m_pExpression = 0;
}

CFESizeOfExpression::CFESizeOfExpression(CFETypeSpec *pType)
 : CFEExpression(EXPR_SIZEOF)
{
    m_pType = pType;
    m_pExpression = 0;
}

CFESizeOfExpression::CFESizeOfExpression(CFEExpression *pExpression)
 : CFEExpression(EXPR_SIZEOF)
{
    m_pType = 0;
    m_pExpression = pExpression;
}

CFESizeOfExpression::CFESizeOfExpression(CFESizeOfExpression &src)
 : CFEExpression(src)
{
    CLONE_MEM(CFETypeSpec, m_pType);
    CLONE_MEM(CFEExpression, m_pExpression);
}

/** \brief creates a copy of this object
 *  \return a reference to a new object of this class
 */
CObject* CFESizeOfExpression::Clone()
{
    return new CFESizeOfExpression(*this);
}

/** \brief access the type member
 *  \return a reference to the type member
 */
CFETypeSpec* CFESizeOfExpression::GetSizeOfType()
{
    return m_pType;
}

/** \brief access the expression member
 *  \return a reference to the expression member
 */
CFEExpression* CFESizeOfExpression::GetSizeOfExpression()
{
    return m_pExpression;
}

/** \brief print the object to a string
 *  \return a string with the content of the object
 */
std::string CFESizeOfExpression::ToString()
{
    std::string ret = "sizeof(";
    if (m_pType)
        ret += m_pType->ToString();
    else if (m_pExpression)
        ret += m_pExpression->ToString();
    else
        ret += m_String;
    ret += ")";
    return ret;
}

/** \brief the accept method for the visitors
 *  \param v reference to the current visitor
 */
void CFESizeOfExpression::Accept(CVisitor &v)
{
    v.Visit(*this);
}
