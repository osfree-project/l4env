/**
 *  \file    dice/src/fe/FEAlignOfExpression.cpp
 *  \brief   contains the implementation of the class CFEAlignOfExpression
 *
 *  \date    07/30/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2007
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

#include "fe/FEAlignOfExpression.h"
#include "fe/FETypeSpec.h"

CFEAlignOfExpression::CFEAlignOfExpression()
 : CFEExpression()
{
    m_pType = 0;
    m_pExpression = 0;
}

/** destroys this object */
CFEAlignOfExpression::~CFEAlignOfExpression()
{
    if (m_pType)
        delete m_pType;
    if (m_pExpression)
        delete m_pExpression;
}

CFEAlignOfExpression::CFEAlignOfExpression(std::string sTypeName)
 : CFEExpression(sTypeName)
{
    m_pType = 0;
    m_pExpression = 0;
}

CFEAlignOfExpression::CFEAlignOfExpression(CFETypeSpec *pType)
 : CFEExpression(EXPR_SIZEOF)
{
    m_pType = pType;
    m_pExpression = 0;
}

CFEAlignOfExpression::CFEAlignOfExpression(CFEExpression *pExpression)
 : CFEExpression(EXPR_SIZEOF)
{
    m_pType = 0;
    m_pExpression = pExpression;
}

CFEAlignOfExpression::CFEAlignOfExpression(CFEAlignOfExpression &src)
 : CFEExpression(src)
{
    CLONE_MEM(CFETypeSpec, m_pType);
    CLONE_MEM(CFEExpression, m_pExpression);
}

/** \brief creates a copy of this object
 *  \return a reference to a new object of this class
 */
CObject* CFEAlignOfExpression::Clone()
{
    return new CFEAlignOfExpression(*this);
}

/** \brief access the type member
 *  \return a reference to the type member
 */
CFETypeSpec* CFEAlignOfExpression::GetSizeOfType()
{
    return m_pType;
}

/** \brief access the expression member
 *  \return a reference to the expression member
 */
CFEExpression* CFEAlignOfExpression::GetSizeOfExpression()
{
    return m_pExpression;
}

/** \brief print the object to a string
 *  \return a string with the content of the object
 */
std::string CFEAlignOfExpression::ToString()
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

