/**
 *  \file   dice/src/fe/FEArrayType.cpp
 *  \brief  contains the implementation of the class CFEArrayType
 *
 *  \date   03/23/2001
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "FEArrayType.h"
#include "FEExpression.h"
#include "File.h"
#include "Compiler.h"
#include <iostream>

CFEArrayType::CFEArrayType(CFETypeSpec * pBaseType, CFEExpression * pBound)
:CFETypeSpec(TYPE_ARRAY)
{
    m_pBaseType = pBaseType;
    m_pBound = pBound;
}

CFEArrayType::CFEArrayType(CFEArrayType & src)
:CFETypeSpec(src)
{
    if (src.m_pBaseType)
      {
      m_pBaseType = (CFETypeSpec *) (src.m_pBaseType->Clone());
      m_pBaseType->SetParent(this);
      }
    else
    m_pBaseType = 0;
    if (src.m_pBound)
      {
      m_pBound = (CFEExpression *) (src.m_pBound->Clone());
      m_pBound->SetParent(this);
      }
    else
    m_pBound = 0;
}

/** cleans up the array type object */
CFEArrayType::~CFEArrayType()
{
    if (m_pBaseType)
    delete m_pBaseType;
    if (m_pBound)
    delete m_pBound;
}

/** creates a copy of this object
 *  \return a reference to the copy of this object
 */
CObject *CFEArrayType::Clone()
{
    return new CFEArrayType(this);
}

/** returns the base type of the sequence
 *  \return the base type of the sequence
 */
CFETypeSpec *CFEArrayType::GetBaseType()
{
    return m_pBaseType;
}

/** returns the boundary of the sequence
 *  \return the boundary of the sequence
 */
CFEExpression *CFEArrayType::GetBound()
{
    return m_pBound;
}

/** \brief accept iterations of the visitors
 *  \param v reference to the visitor
 */
void CFEArrayType::Accept(CVisitor& v)
{
    if (m_pBaseType)
	m_pBaseType->Accept(v);
    if (m_pBound)
	m_pBound->Accept(v);
}
