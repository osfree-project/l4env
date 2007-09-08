/**
 *  \file   dice/src/fe/FESimpleType.cpp
 *  \brief  contains the implementation of the class CFESimpleType
 *
 *  \date   01/31/2001
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

#include "FESimpleType.h"
#include "Compiler.h"
#include "Visitor.h"

CFESimpleType::CFESimpleType(unsigned int nType,
                 bool bUnSigned,
                 bool bUnsignedFirst,
                 int nSize,
                 bool bShowType)
  : CFETypeSpec(nType),
    m_bUnSigned(bUnSigned),
    m_bUnsignedFirst(bUnsignedFirst),
    m_bShowType(bShowType),
    m_nSize(nSize),
    m_FixedPrecision(0, 0)
{ }

CFESimpleType::CFESimpleType(unsigned int nType,
                 int nFirst,
                 int nSecond)
  : CFETypeSpec(nType),
    m_bUnSigned(false),
    m_bUnsignedFirst(false),
    m_bShowType(true),
    m_nSize(0),
    m_FixedPrecision(nFirst, nSecond)
{ }

CFESimpleType::CFESimpleType(CFESimpleType & src)
  : CFETypeSpec(src)
{
    m_bUnSigned = src.m_bUnSigned;
    m_bUnsignedFirst = src.m_bUnsignedFirst;
    m_bShowType = src.m_bShowType;
    m_nSize = src.m_nSize;
    m_FixedPrecision = src.m_FixedPrecision;
}

/** CFESimpleType destructor */
CFESimpleType::~CFESimpleType()
{
    // nothing to clean up
}

/** clones the object using the copy constructor */
CObject *CFESimpleType::Clone()
{
    return new CFESimpleType(*this);
}

/** \brief test a type whether it is a constructed type or not
 *  \return true
 */
bool CFESimpleType::IsConstructedType()
{
    return false;
}

/** \brief test if a type is a pointered type
 *  \return true if it is a pointered type, false if not
 */
bool CFESimpleType::IsPointerType()
{
    // if type is simple -> return false
    if (m_nType == TYPE_VOID_ASTERISK ||
	m_nType == TYPE_CHAR_ASTERISK)
	return true;
    return false;
}

/** \brief accepts the iterations of the visitors
 *  \param v reference to the visitor
 */
void CFESimpleType::Accept(CVisitor& v)
{
    v.Visit(*this);
}

/** \brief resturns the size of the type
 *  \return the size of the type specified in the IDL file
 */
int CFESimpleType::GetSize()
{
    return m_nSize;
}
