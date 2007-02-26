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
#include "File.h"
#include "Visitor.h"

CFESimpleType::CFESimpleType(unsigned int nType,
                 bool bUnSigned,
                 bool bUnsignedFirst,
                 int nSize,
                 bool bShowType)
:CFETypeSpec(nType)
{
    m_bUnSigned = bUnSigned;
    m_bUnsignedFirst = bUnsignedFirst;
    m_bShowType = bShowType;
    m_nSize = nSize;
}

CFESimpleType::CFESimpleType(CFESimpleType & src)
:CFETypeSpec(src)
{
    m_bUnSigned = src.m_bUnSigned;
    m_bUnsignedFirst = src.m_bUnsignedFirst;
    m_bShowType = src.m_bShowType;
    m_nSize = src.m_nSize;
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

/** checks if this type is unsigned
 *  \return true if unsigned
 */
bool CFESimpleType::IsUnsigned()
{
    return m_bUnSigned;
}

/** \brief accepts the iterations of the visitors
 *  \param v reference to the visitor
 */
void CFESimpleType::Accept(CVisitor& v)
{
    v.Visit(*this);
}

/** sets the signed/unsigned variable
 *  \param bUnsigned the new unsigned value
 */
void CFESimpleType::SetUnsigned(bool bUnsigned)
{
    m_bUnSigned = bUnsigned;
}

/** \brief resturns the size of the type
 *  \return the size of the type specified in the IDL file
 */
int CFESimpleType::GetSize()
{
    return m_nSize;
}
