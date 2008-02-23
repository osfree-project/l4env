/**
 *    \file    dice/src/be/BEMsgBufferType.cpp
 *    \brief   contains the implementation of the class CBEMsgBufferType
 *
 *    \date    11/10/2004
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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

#include "BEMsgBufferType.h"
#include "BEClassFactory.h"
#include "BEContext.h"
#include "BEStructType.h"
#include "BEUnionCase.h"
#include "BEFunction.h"
#include "BEWaitFunction.h"
#include "BEUnmarshalFunction.h"
#include "BEDeclarator.h"
#include "BEClass.h"
#include "BEMsgBuffer.h"
#include "BEIDLUnionType.h"
#include "BEAttribute.h"
#include "BEUserDefinedType.h"
#include "BEExpression.h"
#include "BENameFactory.h"
#include "BESizes.h"
#include "Compiler.h"
#include "Error.h"
#include "Messages.h"
#include "TypeSpec-Type.h"
#include "fe/FEOperation.h"
#include "fe/FEInterface.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEDeclarator.h"

#include <string>
#include <cassert>

CBEMsgBufferType::CBEMsgBufferType()
: CBEUnionType()
{}

CBEMsgBufferType::CBEMsgBufferType(CBEMsgBufferType* src)
: CBEUnionType(src)
{ }

CBEMsgBufferType::~CBEMsgBufferType()
{ }

/** \brief create a copy of this object
 *  \return a reference to the clone
 */
CBEMsgBufferType* CBEMsgBufferType::Clone()
{
	return new CBEMsgBufferType(this);
}

/** \brief initialize instance of this class
 *  \param pFEOperation the front-end operation to use as reference
 *  \return true on success
 *
 * The message buffer type for a simple function consists of structs for the
 * communication directions given by the back-end function.
 */
void
CBEMsgBufferType::CreateBackEnd(CFEOperation *pFEOperation)
{
	CCompiler::VerboseI("CBEMsgBufferType::%s(fe-op) called\n",	__func__);

	CBEUnionType::CreateBackEnd(string());
	AddStruct(pFEOperation);

	CCompiler::VerboseD("CBEMsgBufferType::%s(fe-op) returns\n", __func__);
}

/** \brief initialize instance of this class
 *  \param pFEInterface the front-end interface to usa as reference
 *  \return true on success
 *
 * Creates the message buffer type for the class. A previous implementation
 * iterated the function group of the class and select a client side function
 * and simply took a reference to the client side message buffer and added it
 * to the class' message buffer.
 *
 * We can't do that because the server side message buffer might look
 * different. For instance can something like:
 * [in, size_is(p2)] char * p1, [in] long p2
 * look at the client (in the message buffer) like this:
 * char p1[p2];
 * long p2;
 * at the server however we do not know p2 before receiving the message and p2
 * is no global variable that can be used in the global message buffer type
 * declaration. Therefore we have to assume a size at the server:
 * char p1[\<max size of char arrays\>];
 * long p2;
 *
 * So message buffers are different for client and server and we therefore
 * have to create separate structs for the class.
 */
void CBEMsgBufferType::CreateBackEnd(CFEInterface *pFEInterface)
{
	CCompiler::VerboseI("CBEMsgBufferType::%s(fe-if) called\n",	__func__);

	// get tag
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sTag = pNF->GetMessageBufferTypeName(pFEInterface);
	sTag = pNF->GetTypeName(pFEInterface, sTag);
	CBEUnionType::CreateBackEnd(sTag);
	AddStruct(pFEInterface);

	CCompiler::VerboseD("CBEMsgBufferType::%s(fe-if) called\n",	__func__);
}

/** \brief adds the structs of an interface
 *  \param pFEInterface the interface to add structs for
 *
 * This function merely iterator the operations of an interface and the base
 * interfaces if any exist.
 */
void CBEMsgBufferType::AddStruct(CFEInterface *pFEInterface)
{
	CCompiler::VerboseI("CBEMsgBufferType::%s(fe-if) called\n", __func__);

	// iterate the operations of the interface
	vector<CFEOperation*>::iterator iter;
	for (iter = pFEInterface->m_Operations.begin();
		iter != pFEInterface->m_Operations.end();
		iter++)
	{
		AddStruct(*iter);
	}

	// because the server loop of an interface should also be able to handle
	// the functions of the base interface, we have to iterate those functions
	// as well
	vector<CFEInterface*>::iterator iI;
	for (iI = pFEInterface->m_BaseInterfaces.begin();
		iI != pFEInterface->m_BaseInterfaces.end();
		iI++)
	{
		AddStruct(*iI);
	}

	CCompiler::VerboseD("CBEMsgBufferType::%s(fe-if) called\n", __func__);
}

/** \brief helper function to add struct for a specific operation
 *  \param pFEOperation the operation to add the struct for
 */
void CBEMsgBufferType::AddStruct(CFEOperation *pFEOperation)
{
	AddStruct(pFEOperation, CMsgStructType::In);
	AddStruct(pFEOperation, CMsgStructType::Out);
	if (!pFEOperation->m_RaisesDeclarators.empty())
		AddStruct(pFEOperation, CMsgStructType::Exc);
}

/** \brief adds a struct to the union
 *  \param pFEOperation the front-end operation to use as reference
 *  \param nType the type of struct to add
 *
 * This creates a struct and adds a union case with the struct as type to the
 * union.  Then it calls the AddElements method, which inserts the parameters
 * into the struct.
 *
 * Then we have to resort the elements to:
 * # align them according to type size
 * # if sum of elements is bigger than maximum message size, some elements have
 *   to be transformed (convert big fixed sizes buffer into indirect part)
 * # variable sized elements should be combined into word array
 *   (also regard maximum size of buffer)
 */
void CBEMsgBufferType::AddStruct(CFEOperation *pFEOperation, CMsgStructType nType)
{
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	assert(pFEOperation);
	// struct type
	CBEStructType *pType = (CBEStructType*)pCF->GetNewType(TYPE_STRUCT);
	CBEUnionCase *pCase = pCF->GetNewUnionCase();
	pType->SetParent(pCase);
	m_UnionCases.Add(pCase);
	// create struct type
	pType->CreateBackEnd(string(), pFEOperation);
	// get name of struct
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sTag = pNF->GetMessageBufferStructName(nType,
		pFEOperation->GetName(),
		pFEOperation->GetSpecificParent<CFEInterface>()->GetName());
	// create union case
	pCase->CreateBackEnd(pType, sTag, 0, false);
	delete pType; // cloned in CBEUnionCase::CBETypedDeclarator::CreateBackEnd
	CCompiler::Verbose("CBEMsgBufferType::AddStruct(%s, %d) created struct @ %p with name %s\n",
		pFEOperation->GetName().c_str(), (int)nType, pType, sTag.c_str());

	// add the elements
	AddElements(pFEOperation, nType);
	CCompiler::Verbose("CBEMsgBufferType::AddStruct returns\n");
}

/** \brief adds the elements of the structs
 *  \param pFEOperation the front-end operation to extract the elements from
 *  \param nType the type of struct
 *  \return true if successful
 *
 * This method iterates over the parameters and calls the AddElement method
 * for each. We do NOT test the direction here, because it might be necessary
 * to add additional members in one direction, even if the parameter is in the
 * other. For example, it might be necessary to send the size of a receive
 * buffer to the server for a buffer which is send from the server to the
 * client.  The parameter (the buffer) is OUT and we still need an IN size
 * element for it.
 */
void CBEMsgBufferType::AddElements(CFEOperation *pFEOperation, CMsgStructType nType)
{
	CCompiler::VerboseI("CBEMsgBufferType::%s called for %s\n", __func__,
		pFEOperation->GetName().c_str());

	if (CMsgStructType::In == nType)
	{
		vector<CFETypedDeclarator*>::iterator iter;
		for (iter = pFEOperation->m_Parameters.begin();
			iter != pFEOperation->m_Parameters.end();
			iter++)
		{
			if ((*iter)->m_Attributes.Find(ATTR_IGNORE))
				continue;
			if (!(*iter)->m_Attributes.Find(ATTR_IN))
				continue;
			AddElement(*iter, nType);
		}
	}
	if (CMsgStructType::Out == nType ||
		CMsgStructType::Exc == nType)
	{
		vector<CFETypedDeclarator*>::iterator iter;
		for (iter = pFEOperation->m_Parameters.begin();
			iter != pFEOperation->m_Parameters.end();
			iter++)
		{
			if ((*iter)->m_Attributes.Find(ATTR_IGNORE))
				continue;
			if (!(*iter)->m_Attributes.Find(ATTR_OUT))
				continue;
			AddElement(*iter, nType);
		}
	}

	CCompiler::VerboseD("CBEMsgBufferType::%s returns\n", __func__);
}

/** \brief adds elements for a single parameter to the message buffer
 *  \param pFEParameter the parameter to add the elements for
 *  \param nType the type of the struct
 *  \return true if successful
 *
 * To add an element we first look for the parameter in the back-end function.
 * If we find it, we clone it and flatten it, that is, we try to reduce it
 * into a flat structure.  This implementation checks direction. If there are
 * other possible tests, implement them by overloading this method
 *
 * If this message buffer belongs to a function, we clone the function's
 * parameters. If this message belongs to a class, we create the parameters
 * anew and add them directly to the struct.
 */
void CBEMsgBufferType::AddElement(CFETypedDeclarator *pFEParameter, CMsgStructType nType)
{
	CCompiler::VerboseI("CBEMsgBufferType::%s(%s, %d) called\n", __func__,
		pFEParameter->m_Declarators.First()->GetName().c_str(),
		(int)nType);

	assert(pFEParameter);
	// get struct
	CFEOperation *pFEOperation =
		pFEParameter->GetSpecificParent<CFEOperation>();
	CFEInterface *pFEInterface =
		pFEOperation->GetSpecificParent<CFEInterface>();
	CBEStructType *pStruct = GetStruct(pFEOperation->GetName(),
		pFEInterface->GetName(), nType);
	assert(pStruct);

	CBEClassFactory *pCF = CBEClassFactory::Instance();
	// get function
	CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
	CCompiler::Verbose("CBEMsgBufferType::%s(%s, %d) has function %s\n", __func__,
		pFEParameter->m_Declarators.First()->GetName().c_str(),
		(int)nType, pFunction ? pFunction->GetName().c_str() : "(null)");
	// create a member, by either cloning a parameter or if parent is class,
	// by creating a new member
	CBETypedDeclarator *pParameter = 0;
	if (pFunction)
	{
		// find parameter in function
		CFEDeclarator *pFEDecl = pFEParameter->m_Declarators.First();
		CBETypedDeclarator *pOriginal =
			pFunction->FindParameter(pFEDecl->GetName());
		// if parameter is not found then this function might not
		// support this parameter, e.g. the unmarshal function does
		// not have OUT parameters. At this point we simply return.
		if (!pOriginal)
		{
			CCompiler::VerboseD("CBEMsgBufferType::%s returns (no orig)\n", __func__);
			return;
		}
		// clone it
		pParameter = pOriginal->Clone();
		pParameter->SetParent(pStruct);
	}
	else
	{
		// just checking
		CBEClass *pClass = GetSpecificParent<CBEClass>();
		assert(pClass);
		// create parameter from front-end parameter
		pParameter = pCF->GetNewTypedDeclarator();
		pParameter->SetParent(pStruct);
		pParameter->CreateBackEnd(pFEParameter);
	}

	AddElement(pStruct, pParameter);

	CCompiler::VerboseD("CBEMsgBufferType::%s returns\n", __func__);
}

/** \brief really add the element to the struct and do all the init work
 *  \param pStruct the struct to add to
 *  \param pParameter the parameter to add
 */
void CBEMsgBufferType::AddElement(CBEStructType *pStruct, CBETypedDeclarator *pParameter)
{
	CCompiler::VerboseI("CBEMsgBufferType::%s for param %s called\n",
		__func__, pParameter->m_Declarators.First()->GetName().c_str());

	// if there is a transmit_as attribute, replace the type
	CBEAttribute *pAttr = pParameter->m_Attributes.Find(ATTR_TRANSMIT_AS);
	// if user defined type, then the alias might have a transmit as attribute
	// as well (the parameter's transmit-as overrides the one of the alias)
	if (!pAttr)
	{
		CBEType *pType = pParameter->GetType();
		while (!pAttr && pType->IsOfType(TYPE_USER_DEFINED))
		{
			string sName = static_cast<CBEUserDefinedType*>(pType)->GetName();
			CBETypedef *pTypedef = FindTypedef(sName);
			if (pTypedef)
			{
				pAttr = pTypedef->m_Attributes.Find(ATTR_TRANSMIT_AS);
				if (pType == pTypedef->GetType())
				{
					// FIXME: something is wrong: the typedef should not be an
					// alias for itself
					break;
				}
				pType = pTypedef->GetType();
			}
		}
	}
	if (pAttr)
	{
		// get type from attribute
		CBEType *pType = pAttr->GetAttrType()->Clone();
		// replace type
		pParameter->ReplaceType(pType);
	}
	// add C language property to avoid const qualifier
	// in struct
	pParameter->AddLanguageProperty(string("noconst"), string());
	// flatten it
	FlattenElement(pParameter, pStruct);
	// add to struct
	pStruct->m_Members.Add(pParameter);

	CCompiler::Verbose("CBEMsgBufferType::%s member %s added to struct\n",
		__func__, pParameter->m_Declarators.First()->GetName().c_str());

	// if variable sized, check size_is/length_is attribute
	if (pParameter->IsVariableSized())
	{
		CCompiler::Verbose("CBEMsgBufferType::%s param %s is var sized\n",
			__func__, pParameter->m_Declarators.First()->GetName().c_str());
		// get function
		CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
		// try to use the original parameter in the declarator stack, because
		// the pParameter is flattened above, thus removing all pointers,
		// which might be required during writing the stack
		CBETypedDeclarator *pOriginal = 0;
		if (pFunction)
			pOriginal = pFunction->FindParameter(
				pParameter->m_Declarators.First()->GetName());
		if (!pOriginal)
			pOriginal = pParameter;
		CDeclStack vStack;
		vStack.push_back(pOriginal->m_Declarators.First());
		// check if string
		CheckElementForString(pParameter, pFunction, pStruct, &vStack);
		// if constructed type, check for variable sized members
		CheckConstructedElementForVariableSize(pParameter, pFunction,
			pStruct, &vStack);
	}

	CCompiler::VerboseD("CBEMsgBufferType::%s returns\n", __func__);
}

/** \brief adds a word sized member struct containing no members yet
 *  \param pFERefObj the front-end reference object
 *  \return true if successful
 */
bool CBEMsgBufferType::AddGenericStruct(CFEBase *pFERefObj)
{
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	// struct type
	CBEStructType *pType = static_cast<CBEStructType*>(
		pCF->GetNewType(TYPE_STRUCT));
	CBEUnionCase *pCase = pCF->GetNewUnionCase();
	pType->SetParent(pCase);
	m_UnionCases.Add(pCase);
	// create struct type
	pType->CreateBackEnd(string(), pFERefObj);
	// get name of struct
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sTag = pNF->GetMessageBufferStructName(CMsgStructType::Generic,
		string(), string());
	// create union case
	pCase->CreateBackEnd(pType, sTag, 0, false);
	delete pType; // cloned in CBETypedDeclarator::CreateBackEnd

	return true;
}

/** \brief retrieve a reference to the struct for a specific function and \
 *         direction
 *  \param sFuncName the front-end operation to use as reference
 *  \param sClassName the front-end interface to use as reference
 *  \param nType the type of the struct
 *  \return a reference to the struct or 0 if not found
 *
 * We first try to find the struct including the function name. If it is not
 * found, we search for the name without the function name.
 */
CBEStructType* CBEMsgBufferType::GetStruct(std::string sFuncName, std::string sClassName, CMsgStructType nType)
{
	CCompiler::Verbose("CBEMsgBufferType::%s called for func %s, class %s and %d\n", __func__,
		sFuncName.c_str(), sClassName.c_str(), (int)nType);

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetMessageBufferStructName(nType, string(), string());
	sFuncName = pNF->GetMessageBufferStructName(nType, sFuncName, sClassName);
	vector<CBEUnionCase*>::iterator iter;
	for (iter = m_UnionCases.begin();
		iter != m_UnionCases.end();
		iter++)
	{
		CCompiler::Verbose("CBEMsgBufferType::%s testing union %s against %s\n",
			__func__, (*iter)->m_Declarators.First()->GetName().c_str(),
			sFuncName.c_str());
		if ((*iter)->m_Declarators.First()->GetName() == sFuncName)
			return (CBEStructType*)(*iter)->GetType();
		CCompiler::Verbose("CBEMsgBufferType::%s testing union %s against %s\n",
			__func__, (*iter)->m_Declarators.First()->GetName().c_str(),
			sName.c_str());
		if ((*iter)->m_Declarators.First()->GetName() == sName)
			return (CBEStructType*)(*iter)->GetType();
	}
	return 0;
}

/** \brief flatten an element
 *  \param pParameter the parameter to flatten
 *  \param pStruct the struct where to add new members
 *
 * To flatten a parameter means, that all indirections (pointers) should be
 * removed (if possible).
 *
 * Also, arrays with pointers, should have a maximum attribute. Use that as
 * array bounds. ([out] parameters have one additional pointer.)
 */
void CBEMsgBufferType::FlattenElement(CBETypedDeclarator *pParameter, CBEStructType *pStruct)
{
	CCompiler::VerboseI("CBEMsgBufferType::%s(param: %s, struct: %p) called\n",
		__func__, pParameter->m_Declarators.First()->GetName().c_str(),
		pStruct);

	CBEDeclarator *pDecl = pParameter->m_Declarators.First();

	CCompiler::Verbose("CBEMsgBufferType::%s for %s and has %d stars (%s,%s,%s)\n",
		__func__, pDecl->GetName().c_str(), pDecl->GetStars(),
		pParameter->m_Attributes.Find(ATTR_IN) ? "IN" : "",
		pParameter->m_Attributes.Find(ATTR_OUT) ? "OUT" : "",
		pParameter->GetType()->IsConstructedType() ? "Constr Type":"");

	if (pParameter->m_Attributes.Find(ATTR_OUT) ||
		(pParameter->m_Attributes.Find(ATTR_IN) &&
		 pParameter->GetType()->IsConstructedType()))
		pDecl->SetStars(0);
	// if IN, simple type, stars, and NO pointer or size attributes,
	// erase pointers
	CCompiler::Verbose("CBEMsgBufferType::%s stars now %d, %s,%s,%s,%s\n",
		__func__, pDecl->GetStars(),
		pParameter->GetType()->IsSimpleType() ? "Simple Type" : "",
		pParameter->m_Attributes.Find(ATTR_SIZE_IS) ? "SIZE" : "",
		pParameter->m_Attributes.Find(ATTR_LENGTH_IS) ? "LENGTH" : "",
		pParameter->m_Attributes.Find(ATTR_MAX_IS) ? "MAX" : "");

	if (pParameter->m_Attributes.Find(ATTR_IN) &&
		pParameter->GetType()->IsSimpleType() &&
		(pDecl->GetStars() > 0) &&
		!((pParameter->m_Attributes.Find(ATTR_SIZE_IS) != 0) ||
			(pParameter->m_Attributes.Find(ATTR_LENGTH_IS) != 0) ||
			(pParameter->m_Attributes.Find(ATTR_MAX_IS) != 0)))
	{
		pDecl->SetStars(0);
		CCompiler::VerboseD("CBEMsgBufferType::%s stars to 0, returns\n", __func__);
		return;
	}
	// if string, check for max_is and set it as array boundary. Also set type
	// to basic type
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	if (pParameter->IsString())
	{
		CCompiler::Verbose("CBEMsgBufferType::%s is string\n", __func__);
		string exc = string(__func__);
		// check for max_is
		CBEAttribute *pAttr = pParameter->m_Attributes.Find(ATTR_MAX_IS);
		if (!pAttr)
		{
			// add max_is attribute with heuristic size
			pAttr = pCF->GetNewAttribute();
			pParameter->m_Attributes.Add(pAttr);
			pAttr->CreateBackEndInt(ATTR_MAX_IS,
				CCompiler::GetSizes()->GetMaxSizeOfType(TYPE_CHAR));
		}
		// add array boundary
		CBEExpression *pBoundary = pCF->GetNewExpression();
		if (pAttr->IsOfType(ATTR_CLASS_INT))
		{
			pBoundary->CreateBackEnd(pAttr->GetIntValue());
		}
		else if (pAttr->IsOfType(ATTR_CLASS_STRING))
		{
			pBoundary->CreateBackEnd(pAttr->GetString());
		}
		else if (pAttr->IsOfType(ATTR_CLASS_IS))
		{
			CBEDeclarator *pIsDecl = pAttr->m_Parameters.First();
			pBoundary->CreateBackEnd(pIsDecl->GetName());
		}
		pDecl->AddArrayBound(pBoundary);
		// flatten base type
		if (pParameter->GetType()->IsPointerType())
		{
			if (pParameter->GetType()->GetFEType() != TYPE_CHAR_ASTERISK)
			{
				exc += " failed, because type of ";
				exc += pDecl->GetName();
				exc += " is not 'char*'.";
				throw new error::create_error(exc);
			}
			CBEType *pType = pCF->GetNewType(TYPE_CHAR);
			pType->CreateBackEnd(false, 0, TYPE_CHAR);
			pParameter->ReplaceType(pType);
		}
		// return here, because the following is not done concurrently with
		// this branch
		CCompiler::VerboseD("CBEMsgBufferType::%s strings done, returns\n", __func__);
		return;
	}

	// test for empty array boundaries
	int nEmptyBounds = 0;
	vector<CBEExpression*>::iterator iterB;
	for (iterB = pDecl->m_Bounds.begin();
		iterB != pDecl->m_Bounds.end();
		iterB++)
	{
		if ((*iterB)->GetIntValue() == 0)
			nEmptyBounds++;
	}

	// handle arrays: if we have a max_is and no array bounds, we add the
	// max_is as array bound.
	// If there is no max_is we have to guess.
	if ((pParameter->m_Attributes.Find(ATTR_SIZE_IS) ||
			pParameter->m_Attributes.Find(ATTR_LENGTH_IS) ||
			pParameter->m_Attributes.Find(ATTR_MAX_IS)) &&
		(pDecl->GetArrayDimensionCount() == nEmptyBounds))
	{
		CCompiler::Verbose("CBEMsgBufferType::%s more empty array bounds\n", __func__);
		// check for stars of declarator: have to remove one for the array
		// boundary
		if (pDecl->GetStars() > 0)
			pDecl->IncStars(-1);

		// look for empty bound and remove it (it will be replaced by new
		// boundary
		if (nEmptyBounds > 0)
		{
			for (iterB = pDecl->m_Bounds.begin();
				iterB != pDecl->m_Bounds.end();
				iterB++)
			{
				if ((*iterB)->GetIntValue() == 0)
				{
					pDecl->RemoveArrayBound(*iterB);
					break;
				}
			}
		}

		CBESizes *pSizes = CCompiler::GetSizes();
		int nFEType = pParameter->GetType()->GetFEType();
		int nMaxSize = pSizes->GetMaxSizeOfType(nFEType);
		CBEExpression *pExpr = pCF->GetNewExpression();
		CBEAttribute *pAttr;
		if ((pAttr = pParameter->m_Attributes.Find(ATTR_MAX_IS)) != 0)
		{
			if (pAttr->IsOfType(ATTR_CLASS_INT))
			{
				pExpr->CreateBackEnd(pAttr->GetIntValue());
			}
			else if (pAttr->IsOfType(ATTR_CLASS_IS))
			{
				CBEDeclarator *pIsDecl = pAttr->m_Parameters.First();
				pExpr->CreateBackEnd(pIsDecl->GetName());
			}
			else
			{
				pExpr->CreateBackEnd(nMaxSize);
			}
			pDecl->AddArrayBound(pExpr);
		}
		else
		{
			// no max-is found, get an estimated maximu
			pExpr->CreateBackEnd(nMaxSize);
			pDecl->AddArrayBound(pExpr);
		}

		// CBEWaitFunction and CBEUnmarshalFunction add a star to the
		// parameter depending on the absence of array dimensionsi and that it
		// is an IN parameter. Now, that we added a array dimension, we can
		// remove the star.
		CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
		CCompiler::Verbose("CBEMsgBufferType::%s check func %s\n", __func__,
			pFunction ? pFunction->GetName().c_str() : "(none)");
		if (dynamic_cast<CBEWaitFunction*>(pFunction) ||
			dynamic_cast<CBEUnmarshalFunction*>(pFunction))
		{
			ATTR_TYPE nAttribute = (pFunction->GetReceiveDirection() == CMsgStructType::In) ? ATTR_IN : ATTR_OUT;
			CCompiler::Verbose("CBEMsgBufferType::%s param has attribute %s? %s\n",
				__func__, (nAttribute == ATTR_IN) ? "IN" : "OUT",
				pParameter->m_Attributes.Find(nAttribute) ? "yes" : "no");
			if (pParameter->m_Attributes.Find(nAttribute))
				pDecl->IncStars(-1);
		}

		// return here
		CCompiler::VerboseD("CBEMsgBufferType::%s empty array fixed (%d stars left for %s), returns\n",
			__func__, pDecl->GetStars(), pDecl->GetName().c_str());
		return;
	}

	// check if type is contructed and parameter is of variable size. Then we
	// have to iterate the members and find the variable sized on. Because
	// constructed types may be nested freely, we use recursive functions.
	if (pParameter->IsVariableSized() &&
		pParameter->GetType()->IsConstructedType())
	{
		CCompiler::Verbose("CBEMsgBufferType::%s param is var sized and has constr type\n",
			__func__);

		CDeclStack vStack;
		vStack.push_back(pParameter->m_Declarators.First());

		FlattenConstructedElement(pParameter, &vStack, pStruct);

		// done
		CCompiler::VerboseD("CBEMsgBufferType::%s var && const fixed, returns\n",
			__func__);
		return;
	}

	// check for pointer types, get the base type and assign that type to
	// element in message buffer
	CBEType *pType = pParameter->GetType();
	CBEUserDefinedType *pUserType = dynamic_cast<CBEUserDefinedType*>(pType);
	if (pType->IsPointerType() && pUserType)
	{
		CCompiler::Verbose("CBEMsgBufferType::%s pointer type and user type\n",
			__func__);

		pType = pUserType->GetRealType();
		CCompiler::Verbose("CBEMsgBufferType::%s user-type %p, real %p\n",
			__func__, pUserType, pType);
		// the new type should be different from the previous (GetRealType
		// returns NULL if not)
		assert(pType);
		CBEDeclarator *pUserDecl = pUserType->GetRealName();
		// add the stars of the pointer type alias to the declarator of the
		// member
		pDecl = pParameter->m_Declarators.First();
		pDecl->IncStars(pUserDecl->GetStars());
		// replace type (clone type, because this is usually a reference to a
		// global type that whould be destroyed if this message buffer type is
		// destroyed. The global type might be "in use" somewhere else)
		pParameter->ReplaceType(pType->Clone());
		// call FlattenElement again to apply flattening technology (tm)
		// again
		FlattenElement(pParameter, pStruct);

		CCompiler::VerboseD("CBEMsgBufferType::%s pointer and user type fixed, returns\n",
			__func__);
		return;
	}

	CCompiler::VerboseD("CBEMsgBufferType::%s returns\n", __func__);

	/// FIXME: implement me
}

/** \brief flatten a parameter with constructed type
 *  \param pParameter the current element
 *  \param pStack the current declarator stack
 *  \param pStruct the struct to add members to
 */
void
CBEMsgBufferType::FlattenConstructedElement(CBETypedDeclarator *pParameter,
	CDeclStack* pStack,
	CBEStructType *pStruct)
{
	assert (pParameter);

	CCompiler::VerboseI("CBEMsgBufferType::%s(%s, stack) called\n",
		__func__, pParameter->m_Declarators.First()->GetName().c_str());

	if (pParameter->GetType()->IsSimpleType())
	{
		if (!pParameter->IsVariableSized())
		{
			CCompiler::VerboseD("CBEMsgBufferType::%s returns (no var member)\n",
				__func__);
			return;
		}

		CCompiler::Verbose("CBEMsgBufferType::%s found var sized member\n",
			__func__);

		// FIXME: found the variable sized member, create new member of fixed
		// size
		CBETypedDeclarator *pMember = pParameter->Clone();
		pStruct->m_Members.Add(pMember);
		// replace declarator
		CBENameFactory *pNF = CBENameFactory::Instance();
		string sName = pNF->GetLocalVariableName(pStack);
		CBEDeclarator *pDecl = pMember->m_Declarators.First();
		pDecl->SetName(sName);

		CCompiler::Verbose("CBEMsgBufferType::%s member %s added to struct\n",
			__func__, sName.c_str());

		FlattenElement(pMember, pStruct);

		CCompiler::VerboseD("CBEMsgBufferType::%s returns\n", __func__);
		return;
	}

	CBEType *pType = pParameter->GetType();
	while (pType->IsOfType(TYPE_USER_DEFINED))
		pType = dynamic_cast<CBEUserDefinedType*>(pType)->GetRealType();

	CBEUnionType *pUnion = dynamic_cast<CBEUnionType*>(pType);
	if (pUnion)
	{
		CCompiler::Verbose("CBEMsgBufferType::%s member has union type\n", __func__);

		vector<CBEUnionCase*>::iterator iter;
		for (iter = pUnion->m_UnionCases.begin();
			iter != pUnion->m_UnionCases.end();
			iter++)
		{
			pStack->push_back((*iter)->m_Declarators.First());
			FlattenConstructedElement(*iter, pStack, pStruct);
			pStack->pop_back();
		}

		CCompiler::VerboseD("CBEMsgBufferType::%s returns at union\n",
			__func__);
		return;
	}

	CBEStructType *pParamStruct = dynamic_cast<CBEStructType*>(pType);
	if (pParamStruct)
	{
		CCompiler::Verbose("CBEMsgBufferType::%s member has struct type\n", __func__);

		vector<CBETypedDeclarator*>::iterator iter;
		for (iter = pParamStruct->m_Members.begin();
			iter != pParamStruct->m_Members.end();
			iter++)
		{
			pStack->push_back((*iter)->m_Declarators.First());
			FlattenConstructedElement((*iter), pStack, pStruct);
			pStack->pop_back();
		}

		CCompiler::VerboseD("CBEMsgBufferType::%s returns at struct\n", __func__);
		return;
	}

	// done
	CCompiler::VerboseD("CBEMsgBufferType::%s returns (done nothing)\n", __func__);
}

/** \brief get iterator pointing at start of payload elements
 *  \param pStruct the struct to check
 *  \return an iterator pointing to start of payload or end of members vector
 *
 * Payload is everything, which is not required by the communication platform
 * in the message. This includes opcode and exception.
 *
 * To implement the skipping of the members that are directly used as
 * parameters, this could return the start of the parameters to be really
 * marshalled. Disadvantage is, that MarshalWordMember cannot use this method
 * anymore, because it would need to access these members.
 */
vector<CBETypedDeclarator*>::iterator CBEMsgBufferType::GetStartOfPayload(CBEStructType* pStruct)
{
	CBEMsgBuffer *pMsgBuffer = GetSpecificParent<CBEMsgBuffer>();
	assert(pMsgBuffer);
	// GetPayloadOffset is in bytes
	int nPayloadOffset = pMsgBuffer->GetPayloadOffset();
	CCompiler::Verbose("CBEMsgBufferType::%s called. start at offset %d\n",
		__func__, nPayloadOffset);
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = pStruct->m_Members.begin();
		iter != pStruct->m_Members.end() && nPayloadOffset > 0;
		iter++)
	{
		CCompiler::Verbose("CBEMsgBufferType::%s taking off %d the size of %s (%d)\n", __func__,
			nPayloadOffset, (*iter)->m_Declarators.First()->GetName().c_str(),
			(*iter)->GetSize());
		nPayloadOffset -= (*iter)->GetSize();
	}
	CCompiler::Verbose("CBEMsgBufferType::%s returning iterator pointing at %s\n", __func__,
		((iter != pStruct->m_Members.end()) &&  (*iter)) ?
		(*iter)->m_Declarators.First()->GetName().c_str() : "(begin)");
	return iter;
}


/** \brief adds a struct to the union
 *  \param pStruct the struct to add as is
 *  \param nType the type of the struct
 *  \param sFunctionName name of function
 *  \param sClassName name of class
 *  \return true on success
 *
 * Have to clone struct, so parent relationship and deletion works properly.
 * Otherwise we would have to introduce special case handling in CBEUnionCase.
 */
void CBEMsgBufferType::AddStruct(CBEStructType *pStruct, CMsgStructType nType, std::string sFunctionName,
	std::string sClassName)
{
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	// clone struct type
	CBEStructType *pType = pStruct->Clone();
	CBEUnionCase *pCase = pCF->GetNewUnionCase();
	pType->SetParent(pCase);
	m_UnionCases.Add(pCase);
	// get name of struct
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sTag = pNF->GetMessageBufferStructName(nType, sFunctionName, sClassName);
	CCompiler::Verbose("CBEMsgBufferType::AddStruct(%p, %d, %s, %s) create struct with name %s\n",
		pStruct, (int)nType, sFunctionName.c_str(), sClassName.c_str(), sTag.c_str());
	// create union case
	pCase->CreateBackEnd(pType, sTag, 0, false);
	delete pType; /* cloned in CBETypedDeclarator::CreateBackEnd */
}

/** \brief checks if a parameter is a string and if it's size parameter is \
 *         present
 *  \param pParameter the parameter to test
 *  \param pFunction the respective function
 *  \param pStruct the struct to add the members to
 *  \param pStack the declarator stack
 */
void
CBEMsgBufferType::CheckElementForString(CBETypedDeclarator *pParameter,
	CBEFunction *pFunction,
	CBEStructType *pStruct,
	CDeclStack* pStack)
{
	assert(pParameter);
	assert(pStruct);
	// if the parameter is not a string or one of the attributes is set, then
	// we depart
	CCompiler::Verbose("CBEMsgBufferType::%s(%s,%s,struct,stack) (%s,%s) called\n",
		__func__, pParameter->m_Declarators.First()->GetName().c_str(),
		pFunction ? pFunction->GetName().c_str() : "",
		pParameter->IsString() ? "STRING" : "",
		pParameter->m_Attributes.Find(ATTR_LENGTH_IS) ? "LENGTH" : "");

	if (!pParameter->IsString() ||
		pParameter->m_Attributes.Find(ATTR_LENGTH_IS))
		return;
	// check if size_is attribute is our own, then this might be called for
	// both directions, and we have to add the size variable to the other
	// struct as well. First check if variable of size_is is length variable
	// and then check if we have this length variable as member. If not, pass.
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetLocalSizeVariableName(pStack);

	CCompiler::Verbose("CBEMsgBufferType::%s size name should be %s\n",
		__func__, sName.c_str());

	CBEAttribute *pSizeAttr = pParameter->m_Attributes.Find(ATTR_SIZE_IS);
	if (pSizeAttr)
	{
		CCompiler::Verbose("CBEMsgBufferType::%s param has size attr\n",
			__func__);
		// if size_is attribute is not our local variable, return
		if (!pSizeAttr->m_Parameters.Find(sName))
		{
			CCompiler::Verbose("CBEMsgBufferType::%s %s is not the param's size attr -> returning\n",
				__func__, sName.c_str());
			return;
		}
		// if local variable is already a member, return
		if (pStruct->m_Members.Find(sName))
		{
			CCompiler::Verbose("CBEMsgBufferType::%s %s is already member -> returning\n",
				__func__, sName.c_str());
			return;
		}
	}

	CBETypedDeclarator *pSizeVar, *pMember;
	// add local variable
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	if (pFunction)
	{
		CCompiler::Verbose("CBEMsgBufferType::%s has function %s\n", __func__,
			pFunction->GetName().c_str());
		// because the declarator really has to be the parameter's
		// declarator, we first get the parameter from the
		// function again and then use it's declarator
		CBETypedDeclarator *pTrueParameter = pFunction->FindParameter(pStack);
		// if no true parameter found, it still might be the return variable
		if (!pTrueParameter)
		{
			pTrueParameter = pFunction->GetReturnVariable();
			string sDeclName;
			if (!pStack->empty())
				sDeclName = pStack->front().pDeclarator->GetName();
			if (sDeclName.empty() ||
				!pTrueParameter->m_Declarators.Find(sDeclName))
				pTrueParameter = 0;
		}
		assert(pTrueParameter);
		// only create init string if required for marshalling. Otherwise this
		// local variable is overwritten by unmarshaling code anyways and the
		// init string may use a char* which is not set yet.
		// required if
		// (pFunction->IsClientSide() && pParam->FindAttr(IN)) ||
		// (pFunction->IsServerSide() && pParam->FindAttr(OUT))
		//
		// we use the top-most parameter, because the pParameter might be the
		// member of a struct without a separate directional attribute.
		CBETypedDeclarator *pOrigParameter = 0;
		if (!pStack->empty())
			pOrigParameter = pFunction->FindParameter(
				pStack->front().pDeclarator->GetName());
		bool bIn = pOrigParameter ? pOrigParameter->m_Attributes.Find(ATTR_IN) :
			pParameter->m_Attributes.Find(ATTR_IN);
		bool bOut = pOrigParameter ? pOrigParameter->m_Attributes.Find(ATTR_OUT) :
			pParameter->m_Attributes.Find(ATTR_OUT);
		string sInitStr;
		if ((pTrueParameter &&
				pFunction->DoMarshalParameter(pTrueParameter, true)) ||
			(pOrigParameter &&
			 pFunction->DoMarshalParameter(pOrigParameter, true)))
		{
			sInitStr = CreateInitStringForString(pFunction, pStack);
		}
		// only add if it does not already exist
		pSizeVar = pFunction->m_LocalVariables.Find(sName);
		if (!pSizeVar)
		{
			pFunction->AddLocalVariable(TYPE_MWORD, true, 0, sName, 0,
				sInitStr);
			pSizeVar = pFunction->m_LocalVariables.Find(sName);
		}

		CCompiler::Verbose("CBEMsgBufferType::%s size var %s added to func %s\n",
			__func__, sName.c_str(), pFunction->GetName().c_str());

		// add direction attributes to size variable
		CBEAttribute *pAttr;
		if (bIn)
		{
			pAttr = pCF->GetNewAttribute();
			pAttr->CreateBackEnd(ATTR_IN);
			pSizeVar->m_Attributes.Add(pAttr);
		}
		if (bOut)
		{
			pAttr = pCF->GetNewAttribute();
			pAttr->CreateBackEnd(ATTR_OUT);
			pSizeVar->m_Attributes.Add(pAttr);
		}
		// set this local variable as size_is attribute of parameter
		pAttr = pCF->GetNewAttribute();
		pAttr->SetParent(pParameter);
		// declarator has to be cloned, otherwise it will be destroyed twice
		CBEDeclarator *pNew = pSizeVar->m_Declarators.First()->Clone();
		pAttr->CreateBackEndIs(ATTR_SIZE_IS, pNew);
		CCompiler::Verbose("CBEMsgBufferType::%s: size_is attribute %s added to %s\n", __func__,
			pNew->GetName().c_str(),
			pParameter->m_Declarators.First()->GetName().c_str());
		pParameter->m_Attributes.Add(pAttr);
		pAttr = pAttr->Clone();
		pTrueParameter->m_Attributes.Add(pAttr);
		CCompiler::Verbose("CBEMsgBufferType::%s size_is attribute added to member and true param\n",
			__func__);
		// clone member
		pMember = pSizeVar->Clone();
		// add member
		pStruct->m_Members.Add(pMember);

		CCompiler::Verbose("CBEMsgBufferType::%s size var (%s) added to struct\n",
			__func__, pMember->m_Declarators.First()->GetName().c_str());
	}
	else
	{
		CCompiler::Verbose("CBEMsgBufferType::%s no function\n",
			__func__);

		pSizeVar = pCF->GetNewTypedDeclarator();
		pSizeVar->SetParent(pStruct);
		// create type
		CBEType *pType = pCF->GetNewType(TYPE_MWORD);
		pType->SetParent(pSizeVar);
		pType->CreateBackEnd(true, 0, TYPE_MWORD);
		// construct variable by hand
		pSizeVar->CreateBackEnd(pType, sName);
		// add direction attributes to size variable
		CBEAttribute *pAttr;
		if (pParameter->m_Attributes.Find(ATTR_IN))
		{
			pAttr = pCF->GetNewAttribute();
			pAttr->CreateBackEnd(ATTR_IN);
			pSizeVar->m_Attributes.Add(pAttr);
		}
		if (pParameter->m_Attributes.Find(ATTR_OUT))
		{
			pAttr = pCF->GetNewAttribute();
			pAttr->CreateBackEnd(ATTR_OUT);
			pSizeVar->m_Attributes.Add(pAttr);
		}
		// add member
		pStruct->m_Members.Add(pSizeVar);
		CCompiler::Verbose("CBEMsgBufferType::%s size var (%s) added to struct\n",
			__func__, pSizeVar->m_Declarators.First()->GetName().c_str());
		// set local variable as size attribute
		pAttr = pCF->GetNewAttribute();
		pAttr->SetParent(pParameter);
		// declarator has to be cloned, otherwise it would be destroyed twice
		CBEDeclarator *pNew = pSizeVar->m_Declarators.First()->Clone();
		pAttr->CreateBackEndIs(ATTR_SIZE_IS, pNew);
		CCompiler::Verbose("CBEMsgBufferType::%s: size_is attributbute %s added to %s\n", __func__,
			pNew->GetName().c_str(),
			pParameter->m_Declarators.First()->GetName().c_str());
		pParameter->m_Attributes.Add(pAttr);
		CCompiler::Verbose("CBEMsgBufferType::%s added size attr to %s\n",
			__func__, pParameter->m_Declarators.First()->GetName().c_str());
	}

	CCompiler::Verbose("CBEMsgBufferType::%s returns\n", __func__);
}

/** \brief creates the init string for a string parameter
 *  \param pFunction the function the parameter belongs to
 *  \param pStack the declarator stack
 *
 * Because the string might be member of a union, we have to iterate the
 * declarator stack and check if the member is a union. If so, we look for a
 * switch variable. If one exists, we have to extend the string-size
 * evaluation by a test if the member is really used.
 */
std::string CBEMsgBufferType::CreateInitStringForString(CBEFunction *pFunction, CDeclStack* pStack)
{
	string sUnionStrPre, sUnionStrSuf;
	// if stack is longer than one element, then there might be a member of a
	// constructed type
	if (pStack->size() > 1)
	{
		CDeclStack vStack;
		// get parameter
		CBETypedDeclarator *pParameter = 0;
		// (no need to test for valid pointer: there are at least two elements
		// in stack)
		pParameter = pFunction->FindParameter(
			pStack->front().pDeclarator->GetName());
		vStack.push_back(pStack->front());
		// at least parameter sould exist
		assert(pParameter);
		// now: check if there is a next element in stack. Then check if
		// parameter's type is constructed (alias of constructed) - should be,
		// otherwise stack wouldn't make sense
		CDeclStack::iterator iter = pStack->begin() + 1;
		for (; iter != pStack->end(); iter++)
		{
			CBEType *pType = pParameter->GetType();
			while (dynamic_cast<CBEUserDefinedType*>(pType))
				pType = static_cast<CBEUserDefinedType*>(pType)->GetRealType();

			CBEStructType *pStruct = dynamic_cast<CBEStructType*>(pType);
			CBEUnionType *pUnion = dynamic_cast<CBEUnionType*>(pType);
			assert(pStruct || pUnion);

			// check IDL union before struct, because IDL union is derived
			// from struct:
			// get switch variable and try to find the value for the next
			// member
			if (CreateInitStringForStringIDLUnion(pParameter, sUnionStrPre,
					sUnionStrSuf, iter, vStack))
				continue;
			// now check: with a struct, we simply get the next member
			if (pStruct)
			{
				pParameter = pStruct->m_Members.Find(iter->pDeclarator->GetName());
				// should exist
				assert(pParameter);
				// add to decl stack
				vStack.push_back(*iter);
				continue;
			}
			// with a union, we issues a big fat warning and terminate the
			// loop: there is nothing reasonable we can do, if we cannot
			// determine for sure if string is used or not
			if (pUnion)
			{
				string sParam;
				CDeclaratorStackLocation::WriteToString(sParam, pStack, true);
				CMessages::Warning(
					"The string %s in function %s is member of a union with descriminator.\n",
					sParam.c_str(), pFunction->GetName().c_str());
				CMessages::Warning(
					"I cannot determine it's size safely. Please ensure that it is either NULL or\n"
					"a valid pointer to a string.\n");
				sUnionStrPre = sUnionStrSuf = string();
				break;
			}
		}
	}
	// string is '(<var>) ? strlen(<var>) : 0'
	string sInitStr = sUnionStrPre + "(";
	CDeclaratorStackLocation::WriteToString(sInitStr, pStack, true);
	sInitStr += ") ? (_dice_strlen(";
	CDeclaratorStackLocation::WriteToString(sInitStr, pStack, true);
	sInitStr += ")+1) : 0" + sUnionStrSuf;
	return sInitStr;
}

/** \brief create an init string for a string in an IDL union
 *  \param pParameter the parameter with the IDL union as type
 *  \retval sUnionStrPre the init string prefix
 *  \retval sUnionStrSuf the init string suffix
 *  \param iter the iterator pointing to the current union member
 *  \param vStack the declarator stack
 *  \return true if we modified something
 */
bool CBEMsgBufferType::CreateInitStringForStringIDLUnion(CBETypedDeclarator*& pParameter,
	std::string& sUnionStrPre, std::string& sUnionStrSuf, CDeclStack::iterator& iter,
	CDeclStack& vStack)
{
	CBEType *pType = pParameter->GetType();
	while (dynamic_cast<CBEUserDefinedType*>(pType))
		pType = static_cast<CBEUserDefinedType*>(pType)->GetRealType();
	CBEIDLUnionType *pIDLUnion = dynamic_cast<CBEIDLUnionType*>(pType);
	if (!pIDLUnion)
		return false;

	CBETypedDeclarator *pSwitchVar = pIDLUnion->GetSwitchVariable();
	assert (pSwitchVar);
	CBETypedDeclarator *pUnionVar = pIDLUnion->GetUnionVariable();
	assert(pUnionVar);
	// current iterator should point to union variable
	assert(pUnionVar->m_Declarators.Find(iter->pDeclarator->GetName()));

	// build switch var string part
	string sSwitchVar;
	vStack.push_back(pSwitchVar->m_Declarators.First());
	CDeclaratorStackLocation::WriteToString(sSwitchVar, &vStack,
		false);
	vStack.pop_back();

	CBEUnionType *pUnion = dynamic_cast<CBEUnionType*>(pUnionVar->GetType());
	// find member
	iter++;
	CBEUnionCase *pCase =
		pUnion->m_UnionCases.Find(iter->pDeclarator->GetName());
	assert(pCase);
	int nCount = 0;
	vector<CBEExpression*>::iterator iL;
	for (iL = pCase->m_Labels.begin();
		iL != pCase->m_Labels.end();
		iL++)
	{
		string sCompare = "(" + sSwitchVar + " == ";
		(*iL)->WriteToStr(sCompare);
		sCompare += ")";

		if (nCount == 0)
			sUnionStrPre = sCompare;
		else
			sUnionStrPre += " || " + sCompare;

		nCount++;
	}
	if (nCount > 1)
		sUnionStrPre = "(" + sUnionStrPre + ")";
	sUnionStrPre += " ? (";
	sUnionStrSuf = ") : 0" + sUnionStrSuf;

	pParameter = pCase;
	// add to decl stack
	vStack.push_back(*iter);
	return true;
}

/** \brief checks if a new variable sized element has its size attributes set
 *  \param pParameter the new parameter representing the element
 *  \param pFunction the respective function
 *  \param pStruct the struct where the member comes from
 *  \param pStack the stack with the declarators
 */
void
CBEMsgBufferType::CheckConstructedElementForVariableSize(
	CBETypedDeclarator *pParameter,
	CBEFunction *pFunction,
	CBEStructType *pStruct,
	CDeclStack* pStack)
{
	CCompiler::Verbose("CBEMsgBufferType::%s(%s,%s,struct,stack) called\n",
		__func__, pParameter->m_Declarators.First()->GetName().c_str(),
		pFunction ? pFunction->GetName().c_str() : "");

	CBEType *pType = pParameter->GetType();
	// when entering here, the parameter is variable sized, so we have to check
	// if it is a constructed type
	if (!pType->IsConstructedType())
		return;

	while (dynamic_cast<CBEUserDefinedType*>(pType))
		pType = static_cast<CBEUserDefinedType*>(pType)->GetRealType();
	// FIXME: outsource struct and union
	// check which sort of constructed type

	if (dynamic_cast<CBEStructType*>(pType))
	{
		CCompiler::Verbose("CBEMsgBufferType::%s type is struct\n",
			__func__);

		CBEStructType *pStructType = static_cast<CBEStructType*>(pType);
		// now iterate the members and perform the same tests
		vector<CBETypedDeclarator*>::iterator iter;
		for (iter = pStructType->m_Members.begin();
			iter != pStructType->m_Members.end();
			iter++)
		{
			if (!(*iter)->IsVariableSized())
				continue;

			// add the member to the declarator stack
			pStack->push_back((*iter)->m_Declarators.First());
			// check if string
			CheckElementForString(*iter, pFunction, pStruct, pStack);
			// if constructed type, check for variable sized members
			CheckConstructedElementForVariableSize(*iter, pFunction,
				pStruct, pStack);
			// get member from stack again
			pStack->pop_back();
		}

		CCompiler::Verbose("CBEMsgBufferType::%s returns\n", __func__);
		return;
	}

	if (dynamic_cast<CBEUnionType*>(pType))
	{
		CCompiler::Verbose("CBEMsgBufferType::%s type is union\n", __func__);

		CBEUnionType *pUnion = static_cast<CBEUnionType*>(pType);
		// check each union case
		vector<CBEUnionCase*>::iterator iter;
		for (iter = pUnion->m_UnionCases.begin();
			iter != pUnion->m_UnionCases.end();
			iter++)
		{
			if (!(*iter)->IsVariableSized())
				continue;

			// push the member to the stack
			pStack->push_back((*iter)->m_Declarators.First());
			// check if string
			CheckElementForString(*iter, pFunction, pStruct, pStack);
			// if constructed type, check for variable sized members
			CheckConstructedElementForVariableSize(*iter, pFunction,
				pStruct, pStack);
			// now get the last element from the stack again
			pStack->pop_back();
		}

		CCompiler::Verbose("CBEMsgBufferType::%s returns\n", __func__);
		return;
	}

	// ignore enums
	CCompiler::Verbose("CBEMsgBufferType::%s nothing done, return\n", __func__);
}

