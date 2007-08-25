/**
 *  \file    dice/src/fe/FEUnionType.cpp
 *  \brief   contains the implementation of the class CFEUnionType
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

#include "FEUnionType.h"
#include "FEUnionCase.h"
#include "FEFile.h"
#include "FEIdentifier.h"
#include "Compiler.h"
#include "Visitor.h"
#include <iostream>

CFEUnionType::CFEUnionType(string sTag,
    vector<CFEUnionCase*> * pUnionBody,
    vector<CFEIdentifier*> * pBaseUnions)
: CFEConstructedType(TYPE_UNION),
    m_UnionCases(pUnionBody, this),
    m_BaseUnions(pBaseUnions, this)
{
    m_sTag = sTag;
    if (!pUnionBody)
        m_bForwardDeclaration = true;
}

CFEUnionType::CFEUnionType(CFEUnionType & src)
: CFEConstructedType(src),
    m_UnionCases(src.m_UnionCases),
    m_BaseUnions(src.m_BaseUnions)
{
    m_sTag = src.m_sTag;
    m_UnionCases.Adopt(this);
    m_BaseUnions.Adopt(this);
}

/** cleans up a union type object */
CFEUnionType::~CFEUnionType()
{}

/** creates a copy of this object
 *  \return a reference to a new union type object
 */
CObject* CFEUnionType::Clone()
{
    return new CFEUnionType(*this);
}

/** \brief test a type whether it is a constructed type or not
 *  \return true
 */
bool CFEUnionType::IsConstructedType()
{
    return true;
}

/** \brief add the members after construction
 *  \param pUnionBody the members added
 */
void CFEUnionType::AddMembers(vector<CFEUnionCase*>* pUnionBody)
{
    m_UnionCases.Add(pUnionBody);
    m_UnionCases.Adopt(this);
}

/** \brief accept the iterations of the visitors
 *  \param v reference to the current visitor
 */
void
CFEUnionType::Accept(CVisitor& v)
{
    v.Visit(*this);

    vector<CFEUnionCase*>::iterator iter;
    for (iter = m_UnionCases.begin();
	 iter != m_UnionCases.end();
	 iter++)
    {
	(*iter)->Accept(v);
    }
}
