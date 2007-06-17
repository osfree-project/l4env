/**
 *  \file    dice/src/fe/FETypedDeclarator.cpp
 *  \brief   contains the implementation of the class CFETypedDeclarator
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

#include "FETypedDeclarator.h"
#include "FESimpleType.h"
#include "FEStructType.h"
#include "FEUserDefinedType.h"
#include "FEArrayDeclarator.h"
#include "FEFile.h"
#include "FELibrary.h"
#include "FEInterface.h"
#include "FEOperation.h"
#include "FEExpression.h"
#include "FEIntAttribute.h"
#include "FEIsAttribute.h"
#include "FETypeAttribute.h"
#include "FEPrimaryExpression.h"
#include "FEConstDeclarator.h"
#include "FEAttribute.h"
#include "Compiler.h"
#include "Visitor.h"
#include <iostream>
#include <cassert>

CFETypedDeclarator::CFETypedDeclarator(TYPEDDECL_TYPE nType,
    CFETypeSpec * pType,
    vector<CFEDeclarator*>* pDeclarators,
    vector<CFEAttribute*>* pTypeAttributes)
: m_Attributes(pTypeAttributes, this),
    m_Declarators(pDeclarators, this)
{
    m_nType = nType;
    m_pType = pType;
    if (m_pType)
        m_pType->SetParent(this);
}

CFETypedDeclarator::CFETypedDeclarator(CFETypedDeclarator & src)
: CFEInterfaceComponent(src),
    m_Attributes(src.m_Attributes),
    m_Declarators(src.m_Declarators)
{
    m_nType = src.m_nType;
    CLONE_MEM(CFETypeSpec, m_pType);
}

/** cleans up the typed declarator object and all its members (type, parameters and attributes) */
CFETypedDeclarator::~CFETypedDeclarator()
{
    if (m_pType)
        delete m_pType;
}

/**
 *  \brief returns the type of the typed declarator (typedef, parameter, exception, ...)
 *  \return the type of the typed declarator
 */
TYPEDDECL_TYPE CFETypedDeclarator::GetTypedDeclType()
{
    return m_nType;
}

/** \brief return true if this is a typedef
 *  \return true if this is a typedef
 */
bool CFETypedDeclarator::IsTypedef()
{
    return m_nType == TYPEDECL_TYPEDEF;
}

/**
 *  \brief replaces the contained type
 *  \param pNewType the new type for this declarator
 *  \return the old type
 */
CFETypeSpec *CFETypedDeclarator::ReplaceType(CFETypeSpec * pNewType)
{
    assert(pNewType);
    CFETypeSpec *pRet = m_pType;
    m_pType = pNewType;
    m_pType->SetParent(this);
    return pRet;
}

/** \brief accepts the iterations of the visitors
 *  \param v reference to the visitor
 */
void CFETypedDeclarator::Accept(CVisitor& v)
{
    v.Visit(*this);
}

/** \brief creates a copy of this object
 *  \return an exact copy of this object
 */
CObject* CFETypedDeclarator::Clone()
{
    return new CFETypedDeclarator(*this);
}

/** \brief returns the contained type
 *  \return the contained type
 */
CFETypeSpec* CFETypedDeclarator::GetType()
{
    return m_pType;
}

/** \brief try to match the given name to the internal names
 *  \param sName the name to check against
 *  \return true if one of the delcarators matches
 */
bool CFETypedDeclarator::Match(string sName)
{
    return m_Declarators.Find(sName) != 0;
}
