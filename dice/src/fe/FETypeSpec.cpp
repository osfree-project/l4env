/**
 *	\file	dice/src/fe/FETypeSpec.cpp
 *	\brief	contains the implementation of the class CFETypeSpec
 *
 *	\date	01/31/2001
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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

#include "fe/FETypeSpec.h"
#include "fe/FESimpleType.h"
#include "fe/FEUserDefinedType.h"
#include "fe/FEStructType.h"
#include "fe/FEUnionType.h"
#include "fe/FEFile.h"
#include "fe/FEDeclarator.h"

// needed for Error function
#include "Compiler.h"

IMPLEMENT_DYNAMIC(CFETypeSpec) 

CFETypeSpec::CFETypeSpec(TYPESPEC_TYPE nType)
{
    IMPLEMENT_DYNAMIC_BASE(CFETypeSpec, CFEInterfaceComponent);

    m_nType = nType;
}

CFETypeSpec::CFETypeSpec(CFETypeSpec & src):CFEInterfaceComponent(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFETypeSpec, CFEInterfaceComponent);

    m_nType = src.m_nType;
}

/** cleans up the type spec object */
CFETypeSpec::~CFETypeSpec()
{
    // nothing to clean up
}

/** retrieves the type of the type spec
 *	\return the type of the type spec
 */
TYPESPEC_TYPE CFETypeSpec::GetType()
{
    return m_nType;
}

/** \brief test a type whether it is a constructed type or not
 *	\param pType the type to test
 *	\return true if it is a constructed type, false if not
 *
 * This function also follows user-defined types
 */
bool CFETypeSpec::IsConstructedType(CFETypeSpec * pType)
{
	// if type is simple -> return false
	if (pType->IsKindOf(RUNTIME_CLASS(CFESimpleType)))
		return false;
	// if user defined -> follow the definition
	if (pType->IsKindOf(RUNTIME_CLASS(CFEUserDefinedType)))
	{
		CFEFile *pRoot = pType->GetRoot();
		assert(pRoot);
		// find type
		String sUserName = ((CFEUserDefinedType *) pType)->GetName();
		CFETypedDeclarator *pUserDecl = pRoot->FindUserDefinedType(sUserName);
		// check if we found the user defined type (if not: panic)
		if (!pUserDecl)
		{
			// if not found now, this can be an interface
		    if (pRoot->FindInterface(sUserName))
			    return true; // is CORBA_Object a constructed type?
			CCompiler::GccError(pType, 0, "User defined type \"%s\" not defined\n", (const char *) sUserName);
			return false;
		}
		// test the found type
		return IsConstructedType(pUserDecl->GetType());
	}
	// is constructed -> test for struct and union
	if (pType->IsKindOf(RUNTIME_CLASS(CFEStructType)))
		return true;
	if (pType->IsKindOf(RUNTIME_CLASS(CFEUnionType)))
		return true;
	// not a constructed type -> return false
	return false;
}

/** \brief test if a type is a pointered type
 *	\param pType the type to test
 *	\return true if it is a pointered type, false if not
 *
 * This function also follows user-defined types
 */
bool CFETypeSpec::IsPointerType(CFETypeSpec * pType)
{
	// if type is simple -> return false
	if (pType->IsKindOf(RUNTIME_CLASS(CFESimpleType)))
		return false;
	// if user defined -> follow the definition
	if (pType->IsKindOf(RUNTIME_CLASS(CFEUserDefinedType)))
	{
		CFEFile *pRoot = pType->GetRoot();
		assert(pRoot);
		// find type
		String sUserName = ((CFEUserDefinedType *) pType)->GetName();
		CFETypedDeclarator *pUserDecl = pRoot->FindUserDefinedType(sUserName);
		// if not found now, this can be an interface
		if (!pUserDecl)
		    if (pRoot->FindInterface(sUserName))
			    pUserDecl = pRoot->FindUserDefinedType(String("CORBA_Object"));
		// check if we found the user defined type (if not: panic)
		if (!pUserDecl)
		{
			CCompiler::GccError(pType, 0, "User defined type \"%s\" not defined\n", (const char *) sUserName);
			return false;
		}
		// test decls for pointers
		VectorElement *pIter = pUserDecl->GetFirstDeclarator();
		CFEDeclarator *pDecl = pUserDecl->GetNextDeclarator(pIter);
		if (pDecl && (pDecl->GetStars() > 0))
		    return true;
		// test the found type
		return IsPointerType(pUserDecl->GetType());
	}
	// not a pointered type -> return false
	return false;
}

/** helper function */
bool CFETypeSpec::CheckConsistency()
{
    CCompiler::GccError(this, 0, "The type %d does not implement the CheckConsistency function.", m_nType);
    return false;
}
