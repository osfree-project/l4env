/**
 *	\file	dice/src/fe/FEUserDefinedType.cpp
 *	\brief	contains the implementation of the class CFEUserDefinedType
 *
 *	\date	01/31/2001
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

#include "fe/FEUserDefinedType.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEConstructedType.h"
#include "fe/FEFile.h"

// needed for Error function
#include "Compiler.h"

IMPLEMENT_DYNAMIC(CFEUserDefinedType)

CFEUserDefinedType::CFEUserDefinedType(String sName)
: CFETypeSpec(TYPE_USER_DEFINED)
{
    IMPLEMENT_DYNAMIC_BASE(CFEUserDefinedType, CFETypeSpec);
    m_sName = sName;
}

CFEUserDefinedType::CFEUserDefinedType(CFEUserDefinedType & src)
: CFETypeSpec(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFEUserDefinedType, CFETypeSpec);
    m_sName = src.m_sName;
}

/** cleans up the user defined type */
CFEUserDefinedType::~CFEUserDefinedType()
{

}

/**
 *	\brief creates a copy of this class
 *	\return the copy of this class
 */
CObject *CFEUserDefinedType::Clone()
{
    return new CFEUserDefinedType(*this);
}

/**
 *	\brief returns the name of the type
 *	\return the name of the type
 */
String CFEUserDefinedType::GetName()
{
    return m_sName;
}

/** \brief check consistency
 *  \return false if error occured, true if everything is fine
 *
 * A user-defined type is consistent if it exists somewhere.
 */
bool CFEUserDefinedType::CheckConsistency()
{
    CFEFile *pFile = GetRoot();
    ASSERT(pFile);
    if (m_sName.IsEmpty())
      {
	  CCompiler::GccError(this, 0, "A user defined type without a name.");
	  return false;
      }
    if (!(pFile->FindUserDefinedType(m_sName)))
      {
	  CCompiler::GccError(this, 0,
			      "User defined type \"%s\" not defined.",
			      (const char *) m_sName);
	  return false;
      }
    return true;
}

/** serialize this object
 *	\param pFile the file to serialize from/to
 */
void CFEUserDefinedType::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        pFile->PrintIndent("<type>%s</type>\n", (const char *) m_sName);
    }
}

/**	\brief should we ignore to resolve this user defined type
 *	\return true if this user type should be ignored
 *
 * We check the parent typed decl for the ignore attribute. If we have no parent type
 * decl or no attribute is found return false.
 */
bool CFEUserDefinedType::Ignore()
{
    CFEConstructedType *pStruct = GetParentConstructedType();
    if (!pStruct)
	return false;
    CFEBase *pTypedDecl = (CFEBase *) (pStruct->GetParent());
    if (!(pTypedDecl->IsKindOf(RUNTIME_CLASS(CFETypedDeclarator))))
	return false;
    if (((CFETypedDeclarator *) pTypedDecl)->FindAttribute(ATTR_IGNORE))
	return true;
    return false;
}

/** \brief get the type of the original type
 *  \return the type of the type this type is an alias for
 */
TYPESPEC_TYPE CFEUserDefinedType::GetOriginalType()
{
    CFEFile *pFile = GetRoot();
    ASSERT(pFile);
    ASSERT(!m_sName.IsEmpty());
    CFETypedDeclarator *pTypedef = pFile->FindUserDefinedType(m_sName);
    ASSERT(pTypedef);
    CFETypeSpec* pType = pTypedef->GetType();
    ASSERT(pType);
    return pType->GetType();
}
