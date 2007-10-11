/**
 *  \file    dice/src/fe/ConsistencyVisitor.cpp
 *  \brief   contains the implementation of the class CConsistencyVisitor
 *
 *  \date    09/15/2006
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006-2007
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

#include "ConsistencyVisitor.h"
#include "FELibrary.h"
#include "FEInterface.h"
#include "FEOperation.h"
#include "FEArrayType.h"
#include "FESimpleType.h"
#include "FEEnumType.h"
#include "FEStructType.h"
#include "FEUnionType.h"
#include "FEUserDefinedType.h"
#include "FEConstDeclarator.h"
#include "FEArrayDeclarator.h"
#include "FETypedDeclarator.h"
#include "FEFile.h"
#include "FEExpression.h"
#include "FEIsAttribute.h"
#include "Compiler.h"
#include "Messages.h"
#include "Error.h"
#include <vector>
#include <cassert>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

CConsistencyVisitor::~CConsistencyVisitor()
{}

/** \brief check consistency for interface
 *  \param interface the interface to check
 */
void CConsistencyVisitor::Visit(CFEInterface& interface)
{
	// check if function names are used twice
	std::vector<CFEOperation*>::iterator iO, iO2;
	for (iO = interface.m_Operations.begin();
		iO != interface.m_Operations.end();
		iO++)
	{
		// first check in same interface
		for (iO2 = iO + 1;
			iO2 != interface.m_Operations.end();
			iO2++)
		{
			if (*iO != *iO2 &&
				(*iO)->GetName() == (*iO2)->GetName())
			{
				CMessages::GccWarning(*iO2,
					"Function name \"%s\" used before (here: %s:%d)\n",
					(*iO2)->GetName().c_str(), (*iO)->m_sourceLoc.getFilename().c_str(),
					(*iO)->m_sourceLoc.getBeginLine());
				throw error::consistency_error();
			}
		}
		// then check in base interface
		std::vector<CFEInterface*>::iterator iI;
		for (iI = interface.m_BaseInterfaces.begin();
			iI != interface.m_BaseInterfaces.end();
			iI++)
		{
			CheckForOpInInterface(*iO, *iI);
		}
	}
}

/** \brief check if an operation is already defined in an interface
 *  \param pFEOperation the operation to look for
 *  \param pFEInterface the interface to search in
 */
void CConsistencyVisitor::CheckForOpInInterface(CFEOperation *pFEOperation,
	CFEInterface *pFEInterface)
{
	std::vector<CFEOperation*>::iterator iO;
	for (iO = pFEInterface->m_Operations.begin();
		iO != pFEInterface->m_Operations.end();
		iO++)
	{
		if ((*iO)->GetName() == pFEOperation->GetName())
		{
			CMessages::GccWarning(pFEOperation,
				"Function \"%s\" redefined. Previously defined in interface %s.\n",
				pFEOperation->GetName().c_str(),
				pFEInterface->GetName().c_str());
			throw error::consistency_error();
		}
	}

	std::vector<CFEInterface*>::iterator iI;
	for (iI = pFEInterface->m_BaseInterfaces.begin();
		iI != pFEInterface->m_BaseInterfaces.end();
		iI++)
	{
		CheckForOpInInterface(pFEOperation, *iI);
	}
}

/** \brief check consistency for constant
 *  \param constant the constant to check
 */
void CConsistencyVisitor::Visit(CFEConstDeclarator& constant)
{
	CFEFile *pRoot = constant.GetRoot();
	assert(pRoot);
	// try to find me
	string sName = constant.GetName();
	if (sName.empty())
	{
		CMessages::GccError(&constant,
			"A constant without a name has been defined.\n");
		throw error::consistency_error();
	}
	// see if this constant exists somewhere
	CFEConstDeclarator *pConstant = pRoot->FindConstDeclarator(sName);
	if (!pConstant)
	{
		CMessages::GccError(&constant,
			"Internal Compiler Error:\n"
			"The chaining of the front-end classes is wrong - please contact\n"
			PACKAGE_BUGREPORT " with a description of this error.\n");
		throw error::consistency_error();
	}
	// check if it is really me
	if (pConstant != &constant)
	{
		CMessages::GccError(&constant, "The constant %s is defined twice.\n",
			sName.c_str());
		throw error::consistency_error();
	}
	// found me     - now check the type
	CFETypeSpec* pType = constant.GetType();
	while (pType && (pType->GetType() == TYPE_USER_DEFINED))
	{
		string sTypeName = ((CFEUserDefinedType*)pType)->GetName();
		CFETypedDeclarator *pTypedef = pRoot->FindUserDefinedType(sTypeName);
		if (!pTypedef)
		{
			CMessages::GccError(&constant,
				"The type (%s) of expression \"%s\" is not defined.\n",
				sTypeName.c_str(), sName.c_str());
			throw error::consistency_error();
		}
		pType = pTypedef->GetType();
	}
	if (!(constant.GetValue()->IsOfType(pType->GetType())))
	{
		CMessages::GccError(&constant,
			"The expression of %s does not match its type.\n", sName.c_str());
		throw error::consistency_error();
	}
}

/** \brief check consistency for an operaiton
 *  \param operation the operation to check
 *
 * This function checks whether or not the operation's syntax and grammar have
 * been parsed correctly. It return false if there inconsitencies.
 *
 * It first performs some integrity checks and returns an error if these fail.
 *
 * This function then replaces some types with a simpler equivalent to ease the
 * latter implementations of the back-end (REFSTRING, STRING, WSTRING). These
 * replacements are usually done when performing the consistency check for each
 * parameter.
 *
 * Currently this function tests (and corrects the following conditions):
 * - does the operation has a directional IN attribute it may not have any OUT
 *   parameters
 * - the same as above, only the directions exchanged
 * - if the parameter have no directional attribute set, it is replaced with
 *   the standard attribute ATTR_IN
 * - do not allow refstring as return type
 * - if an in-parameter is a struct or union type it is referenced
 * - parameter names may not appear mutltiple times
 *
 * The following functionality is obsolete
 * - does the operation have any OUT flexpages an additional parameter is added
 *   -> because we have the receive fpage parameter inside the CORBA_Environment
 */
void CConsistencyVisitor::Visit(CFEOperation& operation)
{
	//////////////////////////////////////////////////////////////
	// if function is set to one way (ATTR_IN) it cannot have a return value
	if (operation.m_Attributes.Find(ATTR_IN) &&
		operation.GetReturnType()->GetType() != TYPE_VOID)
	{
		CMessages::GccError(&operation,
			"A function with attribute [in] cannot have a return type (%s).\n",
			operation.GetName().c_str());
		throw error::consistency_error();
	}
	///////////////////////////////////////////////////////////////
	// check directional attribute of operation and directional attributes of
	// parameter we do this for the message passing stuff
	CheckAttributesOfParams(operation, ATTR_IN);
	CheckAttributesOfParams(operation, ATTR_OUT);
	///////////////////////////////////////////////////////////////
	// check return type
	CheckReturnType(operation);

	////////////////////////////////////////////////////////////////
	// check if [out] parameters are referenced
	// TODO: replace with non-referenced out
	// We could remove the reference internally, thus, we can
	// still parse the existing IDLs but can work with "dereferenced" [out]s
	// internally.
	vector<CFETypedDeclarator*>::iterator iterP;
	for (iterP = operation.m_Parameters.begin();
		iterP != operation.m_Parameters.end();
		iterP++)
	{
		if ((*iterP)->m_Attributes.Find(ATTR_OUT))
		{
			// get declarator
			CFEDeclarator *pD = (*iterP)->m_Declarators.First();
			assert(pD);
			int nStars = pD->GetStars();
			if ((*iterP)->GetType()->IsPointerType())
				nStars++;
			CFEArrayDeclarator *pAD = dynamic_cast<CFEArrayDeclarator*>(pD);
			int nAD = 0;
			if (pAD)
				nAD = pAD->GetDimensionCount();
			nStars += nAD;
			// if neither stars nor array dimension in an array declarator are
			// set, then print an error and exit
			if (nStars == 0)
			{
				CMessages::GccError(*iterP,
					"[out] parameter (%s) must be reference.\n",
					pD->GetName().c_str());
				throw error::consistency_error();
			}
			// if parameter is [out] and has a size or length parameter and is
			// not an array declarator, that is, has array dimensions, then
			// require at least two stars (one for [out] and one for the size
			// attribute). This is required, because stub would allocate
			// memory for that array...
			if ((nAD == 0) &&
				((*iterP)->m_Attributes.Find(ATTR_SIZE_IS) ||
				 (*iterP)->m_Attributes.Find(ATTR_LENGTH_IS)) &&
				!((*iterP)->m_Attributes.Find(ATTR_MAX_IS) ||
					(*iterP)->m_Attributes.Find(ATTR_PREALLOC_CLIENT) ||
					(*iterP)->m_Attributes.Find(ATTR_PREALLOC_SERVER)) &&
				(nStars < 2))
			{
				CMessages::GccError(*iterP,
					"[out] parameter (%s) with [size_is] must have at least 2 references.\n",
					pD->GetName().c_str());
				throw error::consistency_error();
			}
		}
	}
	///////////////////////////////////////////////////////////
	// check if all identifiers in _is attributes are defined
	// as parameters.
	for (iterP = operation.m_Parameters.begin();
		iterP != operation.m_Parameters.end();
		iterP++)
	{
		CheckAttributeParameters(operation, *iterP, ATTR_SIZE_IS, "[size_is]");
		CheckAttributeParameters(operation, *iterP, ATTR_LENGTH_IS, "[length_is]");
		CheckAttributeParameters(operation, *iterP, ATTR_MAX_IS, "[max_is]");
	}
	////////////////////////////////////////////////////////////
	// check for double naming
	for (iterP = operation.m_Parameters.begin();
		iterP != operation.m_Parameters.end();
		iterP++)
	{
		// get name of first param
		CFEDeclarator *pDecl = (*iterP)->m_Declarators.First();
		string sName = pDecl->GetName();
		// now search if this name occures somewhere else
		vector<CFETypedDeclarator*>::iterator iterP2;
		for (iterP2 = iterP;
			iterP2 != operation.m_Parameters.end();
			iterP2++)
		{
			if ((*iterP) != (*iterP2))
			{
				// get name of second param
				pDecl = (*iterP2)->m_Declarators.First();
				if (sName == pDecl->GetName())
				{
					CMessages::GccError(&operation,
						"The operation %s has the parameter %s defined more than once.\n",
						operation.GetName().c_str(), sName.c_str());
					throw error::consistency_error();
				}
			}
		}
	}
	////////////////////////////////////////////////////////
	// check if used exceptions are declared in interface
	vector<CFEIdentifier*>::iterator iterR;
	CFEInterface *pFEInterface = operation.GetSpecificParent<CFEInterface>();
	assert(pFEInterface);
	for (iterR = operation.m_RaisesDeclarators.begin();
		iterR != operation.m_RaisesDeclarators.end();
		iterR++)
	{
		string sName = (*iterR)->GetName();
		if (pFEInterface->m_Exceptions.Find(sName))
			continue;
		// not found
		CMessages::GccError(*iterR,
			"The raises declaration uses an undefined exception (%s).\n",
			sName.c_str());
		throw error::consistency_error();
	}
}

/** \brief checks if the parameter of IS attributes are declared somewhere
 *  \param operation the operation the parameter belongs to
 *  \param pParameter this parameter's attributes should be checked
 *  \param nAttribute the attribute to check
 *  \param sAttribute a string describing the attribute
 *  \return true if no errors
 */
void
CConsistencyVisitor::CheckAttributeParameters(CFEOperation& operation,
	CFETypedDeclarator *pParameter,
	ATTR_TYPE nAttribute,
	const char* sAttribute)
{
	assert(pParameter);
	CFEDeclarator *pDecl = pParameter->m_Declarators.First();
	assert(pDecl);
	CFEFile *pRoot = operation.GetRoot();
	assert(pRoot);
	CFEAttribute *pAttr;
	if ((pAttr = pParameter->m_Attributes.Find(nAttribute)) != 0)
	{
		CFEIsAttribute *pIsAttr = dynamic_cast<CFEIsAttribute*>(pAttr);
		if (!pIsAttr)
			return;
		// check if it has a declarator
		vector<CFEDeclarator*>::iterator iterAttr;
		for (iterAttr = pIsAttr->m_AttrParameters.begin();
			iterAttr != pIsAttr->m_AttrParameters.end();
			iterAttr++)
		{
			// check if parameter exists
			if (operation.FindParameter((*iterAttr)->GetName()))
				continue;
			// check if it is a const
			if (pRoot->FindConstDeclarator((*iterAttr)->GetName()))
				continue;
			// nothing found, assume its wrongly used
			CMessages::GccError(&operation,
				"The argument \"%s\" of attribute %s for parameter %s is not declared as a parameter or constant.\n",
				(*iterAttr)->GetName().c_str(), sAttribute,
				pDecl->GetName().c_str());
			throw error::consistency_error();
		}
	}
}

/** \brief check the attributes of the parameters
 *  \param operation the operaiton to check the parameters
 *  \param nAttr the attribute to check for
 */
void CConsistencyVisitor::CheckAttributesOfParams(CFEOperation& operation,
	ATTR_TYPE nAttr)
{
	if (!operation.m_Attributes.Find(nAttr))
		return;

	ATTR_TYPE nOther = (nAttr == ATTR_IN) ? ATTR_OUT : ATTR_IN;
	vector<CFETypedDeclarator*>::iterator iterP;
	// check all parameters: if we find ATTR_OUT/IN print error
	// if we find ATTR_NONE replace it with ATTR_IN/OUT
	for (iterP = operation.m_Parameters.begin();
		iterP != operation.m_Parameters.end();
		iterP++)
	{
		if ((*iterP)->m_Attributes.Find(nOther))
		{
			string sAttr = (nAttr == ATTR_IN) ? "[in]" : "[out]";
			string sOther = (nOther == ATTR_IN) ? "[in]" : "[out]";
			CMessages::GccError(&operation,
				"Operation %s cannot have %s parameter and operation attribute %s",
				operation.GetName().c_str(), sOther.c_str(), sAttr.c_str());
			throw error::consistency_error();
		}
	}
}

/** \brief check the retrn type of the operation
 *  \param operation the operation to check the return type in
 */
void CConsistencyVisitor::CheckReturnType(CFEOperation& operation)
{
	////////////////////////////////////////////////////////////////
	// check attributes of operation whether they are for return type
	vector<CFEAttribute*>::iterator iterA;
	CFETypeSpec *pReturnType = operation.GetReturnType();
	for (iterA = operation.m_Attributes.begin();
		iterA != operation.m_Attributes.end();
		iterA++)
	{
		// operation attributes are:
		// IDEMPOTENT, BROADCAST, MAYBE, REFLECT_DELETIONS,
		// UUID, ONEWAY, NOOPCODE, NOEXCEPTIONS, ALLOW_REPLY_ONLY,
		// IN, OUT, STRING, CONTEXT_HANDLE
		// all others should belong to return type
		ATTR_TYPE nType = (*iterA)->GetAttrType();
		switch (nType)
		{
		case ATTR_IDEMPOTENT:
		case ATTR_BROADCAST:
		case ATTR_MAYBE:
		case ATTR_REFLECT_DELETIONS:
		case ATTR_UUID:
		case ATTR_UUID_RANGE:
		case ATTR_NOOPCODE:
		case ATTR_NOEXCEPTIONS:
		case ATTR_ALLOW_REPLY_ONLY:
		case ATTR_IN:
		case ATTR_OUT:
		case ATTR_STRING:
		case ATTR_CONTEXT_HANDLE:
		case ATTR_SCHED_DONATE:
		case ATTR_DEFAULT_TIMEOUT:
			/* keep attribute */
			break;
		default:
			if (pReturnType->GetType() == TYPE_VOID)
			{
				CMessages::GccError(*iterA,
					"Attribute (%d) not allowed for void return type.\n",
					nType);
				throw error::consistency_error();
			}
			break;
		}
	}
	////////////////////////////////////////////////////////////////
	// check if return value is of type refstring
	if (pReturnType->GetType() == TYPE_REFSTRING)
	{
		CMessages::GccError(&operation,
			"Type \"refstring\" is not a valid return type of a function (%s).",
			operation.GetName().c_str());
		throw error::consistency_error();
	}
}

/** \brief check the consistency of a type
 *  \param type the type to check
 */
void CConsistencyVisitor::Visit(CFETypeSpec& type)
{
	CMessages::GccError(&type,
		"The type %d has no consistency check implementation.",
		type.GetType());
	throw error::consistency_error();
}

/** \brief check the consistency of the array type
 *  \param type the array type to check
 */
void CConsistencyVisitor::Visit(CFEArrayType& type)
{
	// check if base type is defined
	if (!type.GetBaseType())
	{
		CMessages::GccError(&type, "An array-type without a base type.");
		throw error::consistency_error();
	}
}

/** \brief check the consistency of an enum type
 *  \param type the enum type to check
 */
void CConsistencyVisitor::Visit(CFEEnumType& type)
{
	if (!type.m_Members.empty())
		return;
	if (dynamic_cast<CFETypedDeclarator*>(type.GetParent()))
		return;
	CMessages::GccError(&type,
		"An enum should contain at least one member.");
	throw error::consistency_error();
}

/** \brief check the consistency of a struct type
 *  \param type the struct type to check
 */
void CConsistencyVisitor::Visit(CFEStructType& type)
{
	if (!type.IsForwardDeclaration())
		return;

	/* no members: try to find typedef for this */
	string sTag = type.GetTag();
	CFETypedDeclarator *pType;
	CFEConstructedType *pTagType = 0;
	CFEInterface *pFEInterface = type.GetSpecificParent<CFEInterface>();
	if (pFEInterface &&
		((pType = pFEInterface->m_Typedefs.Find(sTag)) != 0))
	{
		pType->Accept(*this);
		return;
	}
	if (pFEInterface &&
		((pTagType = pFEInterface->m_TaggedDeclarators.Find(sTag)) != 0) &&
		(pTagType != &type))
	{
		pTagType->Accept(*this);
		return;
	}

	CFELibrary *pFELibrary = type.GetSpecificParent<CFELibrary>();
	while (pFELibrary)
	{
		if ((pType = pFELibrary->FindUserDefinedType(sTag)) != 0)
		{
			pType->Accept(*this);
			return;
		}
		if (((pTagType = pFELibrary->FindTaggedDecl(sTag)) != 0) &&
			(pTagType != &type))
		{
			pTagType->Accept(*this);
			return;
		}
		pFELibrary = pFELibrary->GetSpecificParent<CFELibrary>();
	}

	CFEFile *pFEFile = type.GetRoot();
	if (pFEFile && ((pType = pFEFile->FindUserDefinedType(sTag)) != 0))
	{
		pType->Accept(*this);
		return;
	}
	if (pFEFile && ((pTagType = pFEFile->FindTaggedDecl(sTag)) != 0) &&
		(pTagType != &type))
	{
		pTagType->Accept(*this);
		return;
	}
}

/** \brief check the consistency of a union type
 *  \param type the union type to check
 */
void CConsistencyVisitor::Visit(CFEUnionType& type)
{
	if (type.IsForwardDeclaration())
	{
		CFEFile *pRoot = type.GetRoot();
		if (pRoot->FindTaggedDecl(type.GetTag()))
			return;
	}
	if (type.m_UnionCases.empty())
	{
		CMessages::GccError(&type,
			"A union without members is not allowed.");
		throw error::consistency_error();
	}
}

/** \brief check the consistency of a simple type
 *  \param type the type to check
 */
void CConsistencyVisitor::Visit(CFESimpleType& type)
{
	if (type.GetType() == TYPE_NONE)
	{
		CMessages::GccError(&type, "The type has no type?!");
		throw error::consistency_error();
	}
	if (type.GetSize() < 0)
	{
		CMessages::GccError(&type, "Type with negative size.");
		throw error::consistency_error();
	}
}

/** \brief check the consistency of a user defined type
 *  \param type the type to check
 */
void CConsistencyVisitor::Visit(CFEUserDefinedType& type)
{
	string sName = type.GetName();
	CFEFile *pFile = type.GetRoot();
	assert(pFile);
	if (sName.empty())
	{
		CMessages::GccError(&type, "A user defined type without a name.");
		throw error::consistency_error();
	}
	// the user defined type can also reference an interface
	CFELibrary *pFELibrary = type.GetSpecificParent<CFELibrary>();
	if ((sName.find("::") != string::npos) ||
		(!pFELibrary))
	{
		if (pFile->FindInterface(sName))
			return;
	}
	else
	{
		if (pFELibrary->FindInterface(sName))
			return;
	}

	// test if type has really been defined
	if (!(pFile->FindUserDefinedType(sName)))
	{
		CMessages::GccError(&type,
			"User defined type \"%s\" not defined.",
			sName.c_str());
		throw error::consistency_error();
	}
}

/** \brief checks consistency of typed declarator
 *  \param typeddecl the typed declarator to check
 *
 * A typed declarator is consitent if it's type is consistent and the
 * declarators are globally unique. The declarators have to be unique only if
 * this is a typedef.  Before we check the declarators, we make them valid
 * within the namespace.
 *
 * - if we have a refstring type, replace it with a char* and [ref]
 *
 */
void CConsistencyVisitor::Visit(CFETypedDeclarator& typeddecl)
{
	CFETypeSpec *pType = typeddecl.GetType();
	if (!pType)
	{
		CMessages::GccError(&typeddecl, "Typed Declarator has no type.");
		throw error::consistency_error();
	}
	// check type
	// to avoid circular calls of CheckConsitency for something like:
	// struct A {
	//   struct A *next;
	// }
	// or
	// typedef struct A A_t;
	// struct A {
	//   A_t *next;
	// }
	// we have to check if the type is the same as a possible struct parent
	// The latter case is covered by our normal type checkup, so we only test
	// for the first case.
	CFEStructType *pEnclosingStruct =
		typeddecl.GetSpecificParent<CFEStructType>();
	if (!(pEnclosingStruct &&
			dynamic_cast<CFEStructType*>(pType) &&
			// our type should be a forward declaration (have no members)
			((CFEStructType*)pType)->IsForwardDeclaration() &&
			// our type has a tag, so check if this is similar to the
			// enclosing struct's tag
			(pEnclosingStruct->GetTag() ==
			 ((CFEStructType*)pType)->GetTag())))
	{
		pType->Accept(*this);
	}

	// check declarators
	if (typeddecl.IsTypedef())
	{
		CFEFile *pRoot = typeddecl.GetRoot();
		assert(pRoot);
		CFEFile *pMyFile = typeddecl.GetSpecificParent<CFEFile>(0);
		vector<CFEDeclarator*>::iterator iterD;
		for (iterD = typeddecl.m_Declarators.begin();
			iterD != typeddecl.m_Declarators.end();
			iterD++)
		{
			CFETypedDeclarator *pSecondType;
			// now check if name is unique
			if (!(pRoot->FindUserDefinedType((*iterD)->GetName())))
			{
				CMessages::GccError(&typeddecl, "The type %s is not defined.",
					(*iterD)->GetName().c_str());
				throw error::consistency_error();
			}
			if ((pSecondType = pRoot->FindUserDefinedType((*iterD)->GetName())) !=
				&typeddecl)
			{
				// check if both are in C header files and if they are include
				// in different trees
				bool bEqualPath = true;
				CFEFile *pSecondFile =
					pSecondType->GetSpecificParent<CFEFile>(0);
				if (!(pMyFile->IsIDLFile()) && !(pSecondFile->IsIDLFile()) &&
					(pMyFile->GetFileName() == pSecondFile->GetFileName()))
				{
					// they have to have different parents somewhere
					// they start off being the same file
					while (bEqualPath && pMyFile)
					{
						if (!pMyFile->GetParent())
							break;
						// then tey get the next parent file
						pMyFile = pMyFile->GetSpecificParent<CFEFile>(1);
						pSecondFile =
							pSecondFile->GetSpecificParent<CFEFile>(1);
						if (!pSecondFile)
						{
							bEqualPath = false;
							break;
						}
						// test if the file names are the same
						if (pMyFile->GetFileName() !=
							pSecondFile->GetFileName())
						{
							bEqualPath = false;
							break;
						}
					}
				}
				// if the path is equal (which is also true if none of the
				// first if conditions are met) then we print an error message
				if (bEqualPath)
				{
					CMessages::GccError(&typeddecl, "The type %s is defined" \
						" multiple times. Previously defined here: %s at" \
						" line %d.", (*iterD)->GetName().c_str(),
						(pSecondFile) ?
						(pSecondFile->GetFileName().c_str()) : "",
						pSecondType->m_sourceLoc.getBeginLine());
					throw error::consistency_error();
				}
			}
		}
	}
	// check string and wstring
	if (((pType->GetType() == TYPE_CHAR) ||
			(pType->GetType() == TYPE_WCHAR)) &&
		typeddecl.m_Attributes.Find(ATTR_STRING))
	{
		// now we search all declarators. We make from any more than 1 star
		// only one star (a string can have only one star (per definition)
		// set star with declarator (can only be one or array dimension)
		vector<CFEDeclarator*>::iterator iterD;
		for (iterD = typeddecl.m_Declarators.begin();
			iterD != typeddecl.m_Declarators.end(); )
		{
			// we check if there are too many stars
			int nMaxStars = 1;
			if (typeddecl.m_Attributes.Find(ATTR_OUT))
				nMaxStars = 2;
			if ((*iterD)->GetStars() > nMaxStars)
			{
				CFEOperation *pFEOperation =
					typeddecl.GetSpecificParent<CFEOperation>();
				if (pFEOperation)
					CMessages::GccWarning(&typeddecl,
						"\"%s\" in function \"%s\" has more than one pointer (fixed).\n",
						(*iterD)->GetName().c_str(),
						pFEOperation->GetName().c_str());
				else
					CMessages::GccWarning(&typeddecl,
						"\"%s\" has more than one pointer (fixed).\n",
						(*iterD)->GetName().c_str());
			}
		}
	}
	// check if unbound arrays have at least a size_is or length_is or max_is
	// attribute
	vector<CFEDeclarator*>::iterator iterD;
	for (iterD = typeddecl.m_Declarators.begin();
		iterD != typeddecl.m_Declarators.end();
		iterD++)
	{
		CFEArrayDeclarator* pArray = dynamic_cast<CFEArrayDeclarator*>(*iterD);
		if (pArray)
		{
			// if array -> check bounds
			for (unsigned int i = 0; i < pArray->GetDimensionCount(); i++)
			{
				CFEExpression *pLower = pArray->GetLowerBound(i);
				CFEExpression *pUpper = pArray->GetUpperBound(i);
				// if both not set -> unbound
				if ((!pLower) && (!pUpper))
				{
					// check for size_is or length_is or max_is
					// if OUT it only needs size_is or length_is -> user has
					// to provide buffer which is large enough if IN we need
					// max_is, so server loop can provide buffer
					if ((typeddecl.m_Attributes.Find(ATTR_IN)) &&
						(!typeddecl.m_Attributes.Find(ATTR_MAX_IS)))
					{
						CMessages::GccError(&typeddecl,
							"Unbound array declarator \"%s\" with direction IN needs max_is attribute",
							pArray->GetName().c_str());
						throw error::consistency_error();
					}
					if ((!typeddecl.m_Attributes.Find(ATTR_SIZE_IS)) &&
						(!typeddecl.m_Attributes.Find(ATTR_MAX_IS)) &&
						(!typeddecl.m_Attributes.Find(ATTR_LENGTH_IS)))
					{
						CMessages::GccError(&typeddecl,
							"Unbound array declarator \"%s\" needs size_is, length_is or max_is attribute",
							pArray->GetName().c_str());
						throw error::consistency_error();
					}
				}
			}
		}
	}
}

/** \brief check the consistency of a union case
 *  \param ucase the union case to check
 */
void CConsistencyVisitor::Visit(CFEUnionCase& ucase)
{
	if (!ucase.IsDefault() &&
		ucase.m_UnionCaseLabelList.empty())
	{
		CMessages::GccError(&ucase,
			"A Union case has to be either default or have a switch value");
		throw error::consistency_error();
	}
	if (!ucase.GetUnionArm())
	{
		CMessages::GccError(&ucase,
			"A union case has to have a typed declarator.");
		throw error::consistency_error();
	}
}

