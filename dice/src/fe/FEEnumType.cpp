/**
 *  \file    dice/src/fe/FEEnumType.cpp
 *  \brief   contains the implementation of the class CFEEnumType
 *
 *  \date    01/31/2001
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "FEEnumType.h"
#include "FEIdentifier.h"
#include "FETypedDeclarator.h"
#include "Compiler.h"
#include "Visitor.h"

CFEEnumType::CFEEnumType(string sTag,
    vector<CFEIdentifier*> * pMembers)
: CFEConstructedType(TYPE_ENUM),
    m_Members(pMembers, this)
{
    m_sTag = sTag;
}

CFEEnumType::CFEEnumType(CFEEnumType & src)
: CFEConstructedType(src),
    m_Members(src.m_Members)
{
    m_sTag = src.m_sTag;
    m_Members.Adopt(this);
}

/** clean up the enum type (delete the members) */
CFEEnumType::~CFEEnumType()
{ }

/** copies the object
 *  \return a reference to a new enumeration type object
 */
CObject* CFEEnumType::Clone()
{
    return new CFEEnumType(*this);
}

/** \brief accepts the iterations of the visitors
 *  \param v reference to the visitor
 */
void
CFEEnumType::Accept(CVisitor& v)
{
    v.Visit(*this);
}
