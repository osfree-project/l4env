/**
 *    \file    dice/src/fe/FEUserDefinedType.cpp
 *  \brief   contains the implementation of the class CFEUserDefinedType
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

#include "FEUserDefinedType.h"
#include "FETypedDeclarator.h"
#include "FEConstructedType.h"
#include "FEFile.h"
#include "FELibrary.h"
#include "File.h"
// needed for Error function
#include "Compiler.h"
#include "Visitor.h"
#include <iostream>
#include <cassert>

CFEUserDefinedType::CFEUserDefinedType(string sName)
: CFETypeSpec(TYPE_USER_DEFINED)
{
    m_sName = sName;
}

CFEUserDefinedType::CFEUserDefinedType(CFEUserDefinedType & src)
: CFETypeSpec(src)
{
    m_sName = src.m_sName;
}

/** cleans up the user defined type */
CFEUserDefinedType::~CFEUserDefinedType()
{

}

/**
 *  \brief creates a copy of this class
 *  \return the copy of this class
 */
CObject *CFEUserDefinedType::Clone()
{
    return new CFEUserDefinedType(*this);
}

/**
 *  \brief returns the name of the type
 *  \return the name of the type
 */
string CFEUserDefinedType::GetName()
{
    return m_sName;
}

/** \brief accept the iterations of the visitor
 *  \param v reference to the current visitor
 */
void
CFEUserDefinedType::Accept(CVisitor& v)
{
    v.Visit(*this);
}

/** \brief should we ignore to resolve this user defined type
 *  \return true if this user type should be ignored
 *
 * We check the parent typed decl for the ignore attribute. If we have no
 * parent type decl or no attribute is found return false.
 */
bool CFEUserDefinedType::Ignore()
{
    CFEConstructedType *pStruct = GetSpecificParent<CFEConstructedType>();
    if (!pStruct)
	return false;
    CFEBase *pTypedDecl = (CFEBase *) (pStruct->GetParent());
    if (!(dynamic_cast<CFETypedDeclarator*>(pTypedDecl)))
	return false;
    if (((CFETypedDeclarator *) pTypedDecl)->m_Attributes.Find(ATTR_IGNORE))
	return true;
    return false;
}

/** \brief get the type of the original type
 *  \return the type of the type this type is an alias for
 */
unsigned int CFEUserDefinedType::GetOriginalType()
{
    CFEFile *pFile = dynamic_cast<CFEFile*>(GetRoot());
    assert(pFile);
    assert(!m_sName.empty());
    CFETypedDeclarator *pTypedef = pFile->FindUserDefinedType(m_sName);
    assert(pTypedef);
    CFETypeSpec* pType = pTypedef->GetType();
    assert(pType);
    return pType->GetType();
}
