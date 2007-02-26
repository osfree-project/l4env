/**
 *    \file    dice/src/fe/FESizeOfExpression.cpp
 *    \brief   contains the implementation of the class CFESizeOfExpression
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
#include "File.h"

CFESizeOfExpression::CFESizeOfExpression()
 : CFEExpression()
{
    m_pType = NULL;
    m_pExpression = NULL;
}

/** destroys this object */
CFESizeOfExpression::~CFESizeOfExpression()
{
    if (m_pType)
        delete m_pType;
    if (m_pExpression)
        delete m_pExpression;
}

CFESizeOfExpression::CFESizeOfExpression(string sTypeName)
 : CFEExpression(EXPR_SIZEOF, sTypeName)
{
    m_pType = NULL;
    m_pExpression = NULL;
}

CFESizeOfExpression::CFESizeOfExpression(CFETypeSpec *pType)
 : CFEExpression(EXPR_SIZEOF)
{
    m_pType = pType;
    m_pExpression = NULL;
}

CFESizeOfExpression::CFESizeOfExpression(CFEExpression *pExpression)
 : CFEExpression(EXPR_SIZEOF)
{
    m_pType = NULL;
    m_pExpression = pExpression;
}

CFESizeOfExpression::CFESizeOfExpression(CFESizeOfExpression &src)
 : CFEExpression(src)
{
    if (src.m_pType)
        m_pType = (CFETypeSpec*)src.m_pType->Clone();
    else
        m_pType = NULL;
    if (src.m_pExpression)
        m_pExpression = (CFEExpression*)src.m_pExpression->Clone();
    else
        m_pExpression = NULL;
}

/** \brief serialize this class to the file
 *  \param pFile the file to write to
 */
void CFESizeOfExpression::Serialize(CFile *pFile)
{
    pFile->Print("<expression>sizeof(");
    if (m_pType)
        m_pType->Serialize(pFile);
    else if (m_pExpression)
        m_pExpression->Serialize(pFile);
    else
        pFile->Print("%s", m_String.c_str());
    pFile->Print(")</expression>");
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
string CFESizeOfExpression::ToString()
{
    string ret = "sizeof(";
    if (m_pType)
        ret += m_pType->ToString();
    else if (m_pExpression)
        ret += m_pExpression->ToString();
    else
        ret += m_String;
    ret += ")";
    return ret;
}

