/**
 *    \file    dice/src/be/BEMsgBuffer.cpp
 *    \brief   contains the implementation of the class CBEMsgBuffer
 *
 *    \date    11/09/2004
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

#include "BEMsgBuffer.h"
#include "BEFunction.h" // required for DIRECTION_IN/_OUT
#include "BEMsgBufferType.h"
#include "BEInterfaceFunction.h"
#include "BEWaitFunction.h"
#include "BECallFunction.h"
#include "BESndFunction.h"
#include "BEUnmarshalFunction.h"
#include "BEMarshalFunction.h"
#include "BEMarshalExceptionFunction.h"
#include "BEReplyFunction.h"
#include "BEStructType.h"
#include "BEOpcodeType.h"
#include "BEDeclarator.h"
#include "BEAttribute.h"
#include "BEUserDefinedType.h"
#include "BEExpression.h"
#include "BEUnionCase.h"
#include "BENameFactory.h"
#include "BEClassFactory.h"
#include "BESizes.h"
#include "BERoot.h"
#include "Compiler.h"
#include "Error.h"
#include "Messages.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "TypeSpec-Type.h"
#include "BEFile.h"

#include <string>
#include <cassert>
#include <algorithm>

CBEMsgBuffer::CBEMsgBuffer()
: CBETypedef()
{}

CBEMsgBuffer::CBEMsgBuffer(CBEMsgBuffer* src)
: CBETypedef(src)
{}

/** destroys the object */
CBEMsgBuffer::~CBEMsgBuffer()
{}

/** \brief create a copy of this object
 *  \return a reference to the clone
 */
CBEMsgBuffer* CBEMsgBuffer::Clone()
{
	return new CBEMsgBuffer(this);
}

/** \brief checks if message buffer is variable sized for a given direction
 *  \param nType the type of the message buffer struct
 *  \return true if variable sized
 *
 * A message buffer is variable sized if one of it's structs has a variable
 * sized member.
 */
bool CBEMsgBuffer::IsVariableSized(CMsgStructType nType)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEMsgBuffer::%s(%d) called\n", __func__, (int)nType);
	if (CMsgStructType::Generic == nType)
	{
		return IsVariableSized(CMsgStructType::In) ||
			IsVariableSized(CMsgStructType::Out) ||
			IsVariableSized(CMsgStructType::Exc);
	}

	// get the struct
	CBEStructType *pStruct = GetStruct(nType);
	if (!pStruct)
		return false;
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEMsgBuffer::%s(%d) returns %s\n", __func__, (int)nType,
		(pStruct->GetSize() < 0) ? "true" : "false");
	return pStruct->GetSize() < 0;
}

/** \brief calculate the number of elements with a given type for a direction
 *  \param pFunction the function to count for
 *  \param nFEType the type to look for
 *  \param nType the type of the message buffer struct
 *  \return the number of elements of this given type for this direction
 *
 * If no direction is given, the maximum of all directions is calculated.
 * This method returns the number of elements of a certain type, *NOT* their
 * size.
 */
int CBEMsgBuffer::GetCount(CBEFunction *pFunction, int nFEType, CMsgStructType nType)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMsgBufferType::%s (%s, %d, %d)\n", __func__,
		pFunction ? pFunction->GetName().c_str() : "(none)", nFEType, (int)nType);

	if (CMsgStructType::Generic == nType)
	{
		int nSend = GetCount(pFunction, nFEType, CMsgStructType::In);
		int nRecv = GetCount(pFunction, nFEType, CMsgStructType::Out);
		int nExc = GetCount(pFunction, nFEType, CMsgStructType::Exc);
		return std::max(nRecv, std::max(nExc, nSend));
	}

	CBEMsgBufferType *pMsgType = GetType(pFunction);
	CBEStructType *pStruct = GetStruct(pFunction, nType);
	assert(pStruct);

	int nCount = std::count_if(pMsgType->GetStartOfPayload(pStruct),
		pStruct->m_Members.end(), TypeCount(nFEType));

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMsgBuffer::%s (%s, %d, %d) returns %d\n", __func__,
		pFunction ? pFunction->GetName().c_str() : "(none)", nFEType, (int)nType, nCount);
	return nCount;
}

/** \brief calculate the number of elements with a given type for a direction
 *  \param nFEType the type to look for
 *  \param nType the type of the message buffer struct
 *  \return the number of elements of this given type for this direction
 *
 * If no direction is given, the maximum of all directions is calculated.
 * This method returns the number of elements of a certain type, *NOT* their
 * size.
 */
int CBEMsgBuffer::GetCountAll(int nFEType, CMsgStructType nType)
{
	if (CMsgStructType::Generic == nType)
	{
		int nSend = GetCountAll(nFEType, CMsgStructType::In);
		int nRecv = GetCountAll(nFEType, CMsgStructType::Out);
		int nExc = GetCountAll(nFEType, CMsgStructType::Exc);
		return std::max(nRecv, std::max(nExc, nSend));
	}

	CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMsgBufferType::%s (%s, %d, %d)\n", __func__,
		pFunction ? pFunction->GetName().c_str() : "(none)", nFEType, (int)nType);

	CBEMsgBufferType *pMsgType = GetType(pFunction);
	// get struct
	int nMaxCount = 0;
	vector<CBEUnionCase*>::iterator iter;
	for (iter = pMsgType->m_UnionCases.begin();
		iter != pMsgType->m_UnionCases.end();
		iter++)
	{
		string sUnion = (*iter)->m_Declarators.First()->GetName();
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMsgBuffer::%s: checking union %s for type %d\n", __func__,
			sUnion.c_str(), (int)nType);
		CBEStructType *pStruct = static_cast<CBEStructType*>((*iter)->GetType());
		if (nType != GetStructType(pStruct))
			continue;

		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMsgBuffer::%s: counting members of type %d\n", __func__, nFEType);
		int nCount = std::count_if(pMsgType->GetStartOfPayload(pStruct),
			pStruct->m_Members.end(), TypeCount(nFEType));
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMsgBuffer::%s: union has %d members of type %d\n", __func__, nCount, nFEType);
		nMaxCount = std::max(nCount, nMaxCount);
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMsgBuffer::%s (%s, %d, %d) returns %d\n", __func__,
		pFunction ? pFunction->GetName().c_str() : "(none)", nFEType, (int)nType, nMaxCount);
	return nMaxCount;
}

/** \brief initialize the instance of this class
 *  \param pFEOperation the front-end operation to use as reference
 *  \return true on success
 */
void
CBEMsgBuffer::CreateBackEnd(CFEOperation *pFEOperation)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s(op: %s) called\n", __func__,
		pFEOperation->GetName().c_str());

	CBENameFactory *pNF = CBENameFactory::Instance();
	CBEType *pType = CreateType(pFEOperation);
	assert(pType);
	string sName = pNF->GetMessageBufferTypeName(pFEOperation);
	CBETypedef::CreateBackEnd(pType, sName, pFEOperation);
	delete pType; // is cloned in typed decl's create method

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s(op) returns\n", __func__);
}

/** \brief initialize instance of this class
 *  \param pFEInterface the front-end interface to use as reference
 *  \return true on success
 *
 * creates a message buffer for a class.
 */
void
CBEMsgBuffer::CreateBackEnd(CFEInterface *pFEInterface)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s(if: %s) called\n", __func__,
		pFEInterface->GetName().c_str());

	CBENameFactory *pNF = CBENameFactory::Instance();
	CBEType *pType = CreateType(pFEInterface);
	assert(pType);
	string sName = pNF->GetMessageBufferTypeName(pFEInterface);
	CBETypedef::CreateBackEnd(pType, sName, pFEInterface);
	delete pType; // is cloned in typed decl's create method

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s(if) returns\n", __func__);
}

/** \brief creates the type of the message buffer
 *  \param pFEObject the front-end object to use as reference
 *  \return the new type of the message buffer
 */
template<class T> CBEType*
CBEMsgBuffer::CreateType(T * pFEObject)
{
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEMsgBufferType *pType = pCF->GetNewMessageBufferType();
	pType->SetParent(this);
	pType->CreateBackEnd(pFEObject);
	return pType;
}

/** \brief try to find the real type of this message buffer and return it
 *  \param pFunction the function to the struct for
 *  \return reference to real type of this message buffer
 *
 * If we get no function, return the class' message buffer. otherwise the
 * function's message buffer. Even interface function might want to use their
 * own message buffer instead the class' one.
 */
CBEMsgBufferType* CBEMsgBuffer::GetType(CBEFunction *pFunction)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEMsgBuffer::GetType(%s) called\n", pFunction ? pFunction->GetName().c_str() : "(null)");

	CBEMsgBufferType *pType = 0;
	if (dynamic_cast<CBEInterfaceFunction*>(pFunction))
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
			"CBEMsgBuffer::GetType: interface function, get class' message buffer type\n");
		// the message buffer's class functin
		CBEClass *pClass = GetSpecificParent<CBEClass>();
		assert(pClass);
		pType = dynamic_cast<CBEMsgBufferType*>(
			static_cast<CBETypedef*>(pClass->GetMessageBuffer())->GetType());
	}
	else
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
			"CBEMsgBuffer::GetType: either operation function or null: get type of base class\n");
		CBEType *pBaseType = CBETypedef::GetType();
		CBEUserDefinedType *pUsrType = dynamic_cast<CBEUserDefinedType*>(pBaseType);
		if (pUsrType)
			pType = dynamic_cast<CBEMsgBufferType*>(pUsrType->GetRealType());
		else
			pType = dynamic_cast<CBEMsgBufferType*>(pBaseType);
	}
	assert(pType);
	return pType;
}

/** \brief creates an opcode variable for the message buffer
 *  \return a reference to the newly created opcode member
 */
CBETypedDeclarator*
CBEMsgBuffer::GetOpcodeVariable()
{
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEOpcodeType *pType = pCF->GetNewOpcodeType();
	pType->CreateBackEnd();
	string sName = CBENameFactory::Instance()->GetOpcodeVariable();
	CBETypedDeclarator *pOpcode = pCF->GetNewTypedDeclarator();
	assert(pOpcode);
	pOpcode->CreateBackEnd(pType, sName);
	delete pType; // cloned in CBETypedDeclarator::CreateBackEnd
	// add directional attribute so later checks when marshaling work
	CBEAttribute *pAttr = pCF->GetNewAttribute();
	pAttr->SetParent(pOpcode);
	pAttr->CreateBackEnd(ATTR_IN);
	pOpcode->m_Attributes.Add(pAttr);
	return pOpcode;
}

/** \brief creates an exception variable for the message buffer
 *  \return a reference to the newly created exception member
 */
CBETypedDeclarator*
CBEMsgBuffer::GetExceptionVariable()
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s called\n", __func__);

	CBEClassFactory *pCF = CBEClassFactory::Instance();
	// create type
	CBEType *pType = pCF->GetNewType(TYPE_MWORD);
	pType->CreateBackEnd(true, 0, TYPE_MWORD);
	// get name
	string sName = CBENameFactory::Instance()->
		GetExceptionWordVariable();
	// create var
	CBETypedDeclarator *pException = pCF->GetNewTypedDeclarator();
	pException->CreateBackEnd(pType, sName);
	delete pType; // its clone in CBETypedDeclarator::CreateBackEnd
	// add directional attribute, so the test if this should be marshalled
	// will work
	CBEAttribute *pAttr = pCF->GetNewAttribute();
	pAttr->SetParent(pException);
	pAttr->CreateBackEnd(ATTR_OUT);
	pException->m_Attributes.Add(pAttr);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s returns %p\n", __func__, pException);
	return pException;
}

/** \brief adds the return variable to the structs
 *  \param pFunction the function to add the return variable for
 *  \param pReturn the return variable to add
 *  \return true if successful
 *
 * The return variable is add only for the receiving direction and if the
 * return type of the function is not void.
 */
void CBEMsgBuffer::AddReturnVariable(CBEFunction *pFunction, CBETypedDeclarator *pReturn)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMsgBuffer::%s(%s, %s) called\n",
		__func__, pFunction->GetName().c_str(),
		pReturn ? pReturn->m_Declarators.First()->GetName().c_str() : "(null)");
	// get OUT structure
	CMsgStructType nType(CMsgStructType::Generic);
	if (pFunction->IsComponentSide())
		nType = pFunction->GetSendDirection();
	else
		nType = pFunction->GetReceiveDirection();
	assert(CMsgStructType::In == nType || CMsgStructType::Out == nType);
	CBEStructType *pStruct = GetStruct(pFunction, nType);
	assert(pStruct);
	// create exception
	if (!pReturn)
		pReturn = GetReturnVariable(pFunction);
	assert(pReturn);
	// do not add a return variable of type void
	if (pReturn->GetType()->IsVoid())
	{
		delete pReturn;
		return;
	}

	// because this method might be called more than once, we have to check
	// whether the return variable already has been added to the struct
	if (pStruct->m_Members.Find(pReturn->m_Declarators.First()->GetName()))
	{
		delete pReturn;
		return;
	}

	// add return var to struct. This method does all the initialization work,
	// such as replacing type for transmit_as attributes and adding size
	// members for variable sized return type
	CBEMsgBufferType *pType = pStruct->GetSpecificParent<CBEMsgBufferType>();
	assert(pType);
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEMsgBuffer::%s: add return to struct(%d) for function %s\n", __func__, (int)nType,
		pFunction->GetName().c_str());
	pType->AddElement(pStruct, pReturn);
}

/** \brief creates an return variable for the message buffer
 *  \param pFunction the function to add the return variable for
 *  \return a reference to the newly created return member
 */
CBETypedDeclarator*
CBEMsgBuffer::GetReturnVariable(CBEFunction *pFunction)
{
	// we try and clone the functions return variable
	CBETypedDeclarator *pReturn = pFunction->GetReturnVariable();
	assert(pReturn);
	return pReturn->Clone();
}

/** \brief adds platform specific members to this class
 *  \param pFunction the function to add the members for
 *
 * We can always initialize the platform specific members, becuase the are
 * added to all structs in the message union and thus add no memory overhead.
 */
void CBEMsgBuffer::AddPlatformSpecificMembers(CBEFunction *pFunction)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s(func: %s) called\n", __func__,
		pFunction->GetName().c_str());

	AddPlatformSpecificMembers(pFunction, CMsgStructType::In);
	AddPlatformSpecificMembers(pFunction, CMsgStructType::Out);
	// init exception struct only if exceptions
	if (!pFunction->m_Exceptions.empty())
		AddPlatformSpecificMembers(pFunction, CMsgStructType::Exc);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s: return true\n", __func__);
}

/** \brief adds platform specific member to this class for the class
 *  \param pClass the class to add the members for
 *  \return true if successful
 */
void CBEMsgBuffer::AddPlatformSpecificMembers(CBEClass *pClass)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s(class: %s) called\n", __func__,
		pClass->GetName().c_str());

	// iterate function groups of class, pick a function and use it to
	// initialize struct
	vector<CFunctionGroup*>::iterator iter;
	for (iter = pClass->m_FunctionGroups.begin();
		iter != pClass->m_FunctionGroups.end();
		iter++)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
			"CBEMsgBuffer::AddPlatformSpecificMembers: checking function group of operation %s\n",
			(*iter)->GetOperation()->GetName().c_str());
		// iterate the functions of the function group
		vector<CBEFunction*>::iterator iF;
		for (iF = (*iter)->m_Functions.begin();
			iF != (*iter)->m_Functions.end();
			iF++)
		{
			if (dynamic_cast<CBECallFunction*>(*iF) ||
				dynamic_cast<CBESndFunction*>(*iF) ||
				dynamic_cast<CBEWaitFunction*>(*iF))
			{
				AddPlatformSpecificMembers(*iF);
			}
		}
	}

	// also iterate the base classes
	vector<CBEClass*>::iterator iC = pClass->m_BaseClasses.begin();
	for (; iC != pClass->m_BaseClasses.end(); iC++)
		AddPlatformSpecificMembers(*iC);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s returns true\n", __func__);
}

/** \brief adds the platform specific members for a specific function and struct type
 *  \param pFunction the function to add the members for
 *  \param nType the type of the message buffer struct to add the members to
 */
void CBEMsgBuffer::AddPlatformSpecificMembers(CBEFunction *pFunction, CMsgStructType nType)
{
	CBEStructType *pStruct = GetStruct(pFunction, nType);
	assert(pStruct);
	AddPlatformSpecificMembers(pFunction, pStruct);
}

/** \brief adds platform specific members to this class
 *  \param pFunction the function to add the members for
 *  \param pStruct the struct to add to
 *
 * This implementaion adds the opcode member and the exception member to the
 * message buffer struct. The opcode is usually added in the IN struct, except
 * when the function is a wait function, then it is added to the struct of the
 * receiving direction.
 */
void CBEMsgBuffer::AddPlatformSpecificMembers(CBEFunction *pFunction, CBEStructType *pStruct)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s(%s,, %d) called\n", __func__,
		pFunction->GetName().c_str(), (int)GetStructType(pStruct));

	AddOpcodeMember(pFunction, pStruct);
	AddExceptionMember(pFunction, pStruct);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s returns true\n", __func__);
}

/** \brief adds platform specific opcode member
 *  \param pFunction the function to add the members for
 *  \param pStruct the struct to add to
 *  \return true if successful
 */
void CBEMsgBuffer::AddOpcodeMember(CBEFunction *pFunction, CBEStructType *pStruct)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s(%s, struct %d) called\n", __func__,
		pFunction->GetName().c_str(), (int)GetStructType(pStruct));

	CMsgStructType nOpType = pFunction->GetSendDirection();
	if (dynamic_cast<CBEWaitFunction*>(pFunction) ||
		dynamic_cast<CBEUnmarshalFunction*>(pFunction) ||
		dynamic_cast<CBEMarshalFunction*>(pFunction) ||
		dynamic_cast<CBEMarshalExceptionFunction*>(pFunction) ||
		dynamic_cast<CBEReplyFunction*>(pFunction))
		nOpType = pFunction->GetReceiveDirection();
	if (pFunction->m_Attributes.Find(ATTR_NOOPCODE) ||
	    nOpType != GetStructType(pStruct))
		return;
	CBETypedDeclarator *pOpcode = GetOpcodeVariable();
	pStruct->m_Members.Add(pOpcode);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s returns true\n", __func__);
}

/** \brief adds platform specific exception member
 *  \param pFunction the function to add the members for
 *  \param pStruct the struct to add to
 *  \return true if successful
 */
void CBEMsgBuffer::AddExceptionMember(CBEFunction *pFunction, CBEStructType *pStruct)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s(%s, struct %d) called\n", __func__,
		pFunction->GetName().c_str(), (int)GetStructType(pStruct));

	CMsgStructType nExType = pFunction->GetReceiveDirection();
	if (dynamic_cast<CBEUnmarshalFunction*>(pFunction) ||
		dynamic_cast<CBEMarshalFunction*>(pFunction) ||
		dynamic_cast<CBEMarshalExceptionFunction*>(pFunction) ||
		dynamic_cast<CBEReplyFunction*>(pFunction) ||
		dynamic_cast<CBEWaitFunction*>(pFunction))
		nExType = pFunction->GetSendDirection();
	// because the exception can be in two structs: out and exc
	if (pFunction->m_Attributes.Find(ATTR_NOEXCEPTIONS) ||
		(CMsgStructType::Out != GetStructType(pStruct) &&
		 CMsgStructType::Exc != GetStructType(pStruct)))
		return;
	// we get the OUT direction, because we need an "absolute" direction,
	// in contrary to the relative direction of GetReceiveDirection
	CBETypedDeclarator *pException = GetExceptionVariable();
	pStruct->m_Members.Add(pException);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s returned true\n", __func__);
}

/** \brief sorts the parameters in the message buffer structs
 *  \param pFunction the funtion owning the message buffer
 *  \return true if Sort succeeded
 *
 *  This method propagates the call to invoke a \a Sort method on all
 *  structs.
 */
void CBEMsgBuffer::Sort(CBEFunction *pFunction)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called for function %s\n", __func__,
		pFunction->GetName().c_str());

	// IN
	CBEStructType *pStruct = GetStruct(pFunction, CMsgStructType::In);
	assert(pStruct);
	Sort(pStruct);
	// OUT
	pStruct = GetStruct(pFunction, CMsgStructType::Out);
	assert(pStruct);
	Sort(pStruct);
	// EXC
	if (!pFunction->m_Exceptions.empty())
	{
		pStruct = GetStruct(pFunction, CMsgStructType::Exc);
		assert(pStruct);
		Sort(pStruct);
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s returns true\n", __func__);
}

/** \brief sorts the parameters in the message buffer structs
 *  \param pClass the class to which that message buffer belongs
 *  \return true if success
 */
void CBEMsgBuffer::Sort(CBEClass *pClass)
{
	// iterate function groups of class, pick a function and use it to
	// initialize struct
	vector<CFunctionGroup*>::iterator iter;
	for (iter = pClass->m_FunctionGroups.begin();
		iter != pClass->m_FunctionGroups.end();
		iter++)
	{
		// iterate the functions of the function group
		vector<CBEFunction*>::iterator iF;
		for (iF = (*iter)->m_Functions.begin();
			iF != (*iter)->m_Functions.end();
			iF++)
		{
			if ((dynamic_cast<CBECallFunction*>(*iF) != 0) ||
				(dynamic_cast<CBESndFunction*>(*iF) != 0) ||
				(dynamic_cast<CBEWaitFunction*>(*iF) != 0))
			{
				Sort(*iF);
			}
		}
	}

	// sort base class' structs as well
	vector<CBEClass*>::iterator iC = pClass->m_BaseClasses.begin();
	for (; iC != pClass->m_BaseClasses.end(); iC++)
		Sort(*iC);
}

/** \brief sorts one struct in the message buffer
 *  \param pStruct the struct to sort
 *  \return true if sorting was successful
 *
 * This implementation sorts the "simple" payload (no platform specific
 * sorting) and then moves the opcode and the excetion member to the front,
 * where they are expected.  Because only one of them can be in a struct, we
 * can move both to the absolute front, since one of the invocations will
 * fail.
 */
void CBEMsgBuffer::Sort(CBEStructType *pStruct)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called for struct\n", __func__);

	// sort payload
	SortPayload(pStruct);

	CBENameFactory *pNF = CBENameFactory::Instance();
	// find opcode
	string sName = pNF->GetOpcodeVariable();
	pStruct->m_Members.Move(sName, 0);

	// find exception
	sName = pNF->GetExceptionWordVariable();
	pStruct->m_Members.Move(sName, 0);

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s returns true\n", __func__);
}

/** \brief sorts the payload of a message buffer
 *  \param pStruct the struct to sort
 *  \return true if successful
 *
 * The payload is sorted by size.
 * - first comes everything with a fixed size, sorting elements with maximum
 *   of word size first (they are more likely to be on the stack as
 *   parameters).
 *   These elements are sorted in the order of word-size, half-word-size,
 *   byte-size to get size-aligned access.
 * - Then everything which is fixed sized and bigger than word-size, so
 *   we can use fixed offsets into the message buffer. This allows the
 *   target language compiler to optimize access into the message buffer.
 * - Last variable sized elements are inserted.
 *
 * To achieve this, we use some sort of bubble-sort:
 * -# start with first member
 * -# check if current member should be exchanged with its successor
 * -# if yes: exchange it and start all over again (iterators may have changed)
 * -# if no: set successor a current
 * -# if no sorting is done we are finished, otherwise start all over again
 *
 * It would be much simpler to create a new vector with sorted elements, but
 * then: how to swap it with struct's vector?
 */
void CBEMsgBuffer::SortPayload(CBEStructType *pStruct)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "%s called for struct\n", __func__);

	CBEMsgBufferType *pType = pStruct->GetSpecificParent<CBEMsgBufferType>();
	assert(pType);
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = pType->GetStartOfPayload(pStruct);
		iter != pStruct->m_Members.end(); )

	{
		vector<CBETypedDeclarator*>::iterator iterS = iter + 1;
		if (iterS == pStruct->m_Members.end())
			break;
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s compares %s and %s\n",
			__func__, (*iter)->m_Declarators.First()->GetName().c_str(),
			(*iterS)->m_Declarators.First()->GetName().c_str());
		if (DoExchangeMembers(*iter, *iterS))
		{
			pStruct->m_Members.Move(
				(*iterS)->m_Declarators.First()->GetName(),
				(*iter)->m_Declarators.First()->GetName());
			iter = pType->GetStartOfPayload(pStruct);
			// continue
		}
		else
			iter++;
		// continue
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "%s returns true\n", __func__);
}

/** \brief checks if two members of a struct should be exchanged
 *  \param pFirst the first member
 *  \param pSecond the second member - first's successor
 *  \return true if exchange
 *
 * - first test for variable size
 * - if both are of fixed size:
 *   order is:
 *   -# word size
 *   -# half-word size
 *   -# byte size
 *   -# larger than word size
 *
 * (f: first, s:second)
 * (w: word, hw: half-word, byte, ltw: larger than word)
 *
 * f    s: w    hw   byte ltw
 * w       F    F    F    F
 * hw      T    F    F    F
 * byte    T    T    F    F
 * ltw     T    T    T    F
 */
bool CBEMsgBuffer::DoExchangeMembers(CBETypedDeclarator *pFirst, CBETypedDeclarator *pSecond)
{
	assert(pFirst);
	assert(pSecond);
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEMsgBuffer::%s called with (%s, %s)\n",
		__func__, pFirst->m_Declarators.First()->GetName().c_str(),
		pSecond->m_Declarators.First()->GetName().c_str());
	// check variable size
	bool bVarFirst = pFirst->IsVariableSized();
	bool bVarSecond = pSecond->IsVariableSized();
	if (bVarFirst && !bVarSecond)
	{
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
			"CBEMsgBuffer::%s (%s, %s) returns true (1)\n", __func__,
			pFirst->m_Declarators.First()->GetName().c_str(),
			pSecond->m_Declarators.First()->GetName().c_str());
		return true;
	}
	// if both are variable sized, we have to check if one of them has a
	// constructed type. If so, it should be first, since the marshaller will
	// assign first the constructed type and then whatever member is of
	// variable size.
	bool bConstructedFirst = pFirst->GetType()->IsConstructedType();
	bool bConstructedSecond = pSecond->GetType()->IsConstructedType();
	if (bVarFirst && bVarSecond &&
		!bConstructedFirst && bConstructedSecond)
	{
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
			"CBEMsgBuffer::%s (%s, %s) returns true (2)\n", __func__,
			pFirst->m_Declarators.First()->GetName().c_str(),
			pSecond->m_Declarators.First()->GetName().c_str());
		return true;
	}
	// now: no matter what the first parameter is, if the second is a variable
	// sized parameter, leave it after the first (avoids unnecessary swapping)
	if (bVarSecond)
	{
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
			"CBEMsgBuffer::%s (%s, %s) returns true (3)\n", __func__,
			pFirst->m_Declarators.First()->GetName().c_str(),
			pSecond->m_Declarators.First()->GetName().c_str());
		return false;
	}
	// check fixed sizes (f = first, s = second)
	//
	// we like to have something like:
	// +------+-------+-------+---------+
	// | word | >word | <word | padding |
	// +------+-------+-------+---------+
	//
	//  \s|  <1 |  =1 |  >1
	// f \|     |     |
	// ---------------------
	// <1 | f<s | yes | yes
	//    |     |(f<s)|(f<s)
	// ---------------------
	// =1 |  no |  no |  no
	// ---------------------
	// >1 |  no | yes | f>s
	//    |     |(f>s)|
	int nWordSize = CCompiler::GetSizes()->GetSizeOfType(TYPE_MWORD);
	int nFirstSize = pFirst->GetSize();
	int nSecondSize = pSecond->GetSize();
	if (nFirstSize == nWordSize)
	{
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
			"CBEMsgBuffer::%s (%s, %s) returns false (4)\n", __func__,
			pFirst->m_Declarators.First()->GetName().c_str(),
			pSecond->m_Declarators.First()->GetName().c_str());
		return false;
	}
	if (nFirstSize > nWordSize &&
		nSecondSize < nWordSize)
	{
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
			"CBEMsgBuffer::%s (%s, %s) returns false (4b)\n", __func__,
			pFirst->m_Declarators.First()->GetName().c_str(),
			pSecond->m_Declarators.First()->GetName().c_str());
		return false;
	}
	if (nFirstSize > nWordSize)
	{
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEMsgBuffer::%s (%s, %s) returns %s (5)\n",
			__func__, pFirst->m_Declarators.First()->GetName().c_str(),
			pSecond->m_Declarators.First()->GetName().c_str(),
			(nFirstSize > nSecondSize) ? "true" : "false");
		return nFirstSize > nSecondSize;
	}
	// now first is smaller than word
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEMsgBuffer::%s (%s, %s) returns %s (6)\n",
		__func__, pFirst->m_Declarators.First()->GetName().c_str(),
		pSecond->m_Declarators.First()->GetName().c_str(),
		(nFirstSize < nSecondSize) ? "true" : "false");
	return nFirstSize < nSecondSize;
}

/** \brief writes the access to a member of the message buffer
 *  \param pFile the file to write to
 *  \param pFunction the function the member belongs to
 *  \param nType the type of the message buffer struct
 *  \param pStack the member stack to write
 *
 * Find the struct, then write
 * \<buf name\>.\<struct name\>.\<member\>.
 */
void CBEMsgBuffer::WriteAccess(CBEFile& pFile, CBEFunction *pFunction, CMsgStructType nType,
	CDeclStack* pStack)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEMsgBuffer::%s (%s, %s, %d, stack @ %p (%d)) called\n", __func__,
		pFile.GetFileName().c_str(),
		pFunction->GetName().c_str(),
		(int)nType,
		pStack, pStack ? (int)pStack->size() : 0);
	// struct
	WriteAccessToStruct(pFile, pFunction, nType);
	// actual member
	if (!pStack)
		return;

	CDeclStack::iterator iter = pStack->begin();
	for (; iter != pStack->end(); iter++)
	{
		pFile << "." << iter->pDeclarator->GetName();
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEMsgBuffer::WriteAccess finished\n");
}

/** \brief wraps write access with stack
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param nType the type of the message buffer struct
 *  \param pMember the member of the message buffer struct
 *
 * This is an internal function to allow for flat member access.
 */
void CBEMsgBuffer::WriteAccess(CBEFile& pFile, CBEFunction *pFunction, CMsgStructType nType,
	CBETypedDeclarator* pMember)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEMsgBuffer::%s (%s, %s, %d, %s) called\n",
		__func__, pFile.GetFileName().c_str(), pFunction->GetName().c_str(), (int)nType,
		pMember->m_Declarators.First()->GetName().c_str());

	CDeclStack vStack;
	vStack.push_back(pMember->m_Declarators.First());

	WriteAccess(pFile, pFunction, nType, &vStack);

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEMsgBuffer::WriteAccess finished\n");
}

/** \brief write the part of the access to a member that contains the struct
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param nType the type of the message buffer struct
 */
void CBEMsgBuffer::WriteAccessToStruct(CBEFile& pFile, CBEFunction *pFunction, CMsgStructType nType)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEMsgBuffer::%s (%s, %s, %d) called\n", __func__,
		pFile.GetFileName().c_str(), pFunction->GetName().c_str(), (int)nType);

	CBENameFactory *pNF = CBENameFactory::Instance();
	// get struct name
	// if direction is zero, do not use a function name
	string sFuncName = (CMsgStructType::Generic == nType) ?
		string() : pFunction->GetOriginalName();
	string sClassName = (CMsgStructType::Generic == nType) ? string() :
		pFunction->GetSpecificParent<CBEClass>()->GetName();
	// if name is empty, than this is a generic function: get generic struct
	if (sFuncName.empty())
	{
		nType = CMsgStructType::Generic;
		sClassName = string();
	}
	string sStructName = pNF->GetMessageBufferStructName(nType, sFuncName,
		sClassName);
	// write access to message buffer
	CBETypedDeclarator *pMsgBufParam = GetVariable(pFunction);
	// if we have a parameter as message buffer, we have to check that's
	// parameter references. If not, check our own
	bool bHasReference = pMsgBufParam->HasReference();
	string sName = pMsgBufParam->m_Declarators.First()->GetName();
	if (bHasReference)
		sName += "->";
	else
		sName += ".";
	pFile << sName << sStructName;

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEMsgBuffer::WriteAccessToStruct finished\n");
}

/** \brief writes the access to the variable pointing to the message buffer
 *  \param pFunction the function to write the message buffer for
 *  \return reference to the parameter of the function that is the message
 *          buffer, NULL if no message buffer parameter
 */
CBETypedDeclarator* CBEMsgBuffer::GetVariable(CBEFunction *pFunction)
{
	// write access
	string sName = m_Declarators.First()->GetName();
	CBETypedDeclarator *pMsgBufParam = pFunction->FindParameter(sName);

	// if this message buffer is not member of a function, then it really is a
	// typedef and we have to look in the function for a parameter of that
	// type
	if (!GetSpecificParent<CBEFunction>())
	{
		pMsgBufParam = pFunction->FindParameterType(sName);
		if (pMsgBufParam)
			sName = pMsgBufParam->m_Declarators.First()->GetName();
		else
			CMessages::Error("No param of type %s in func %s\n",
				sName.c_str(), pFunction->GetName().c_str());
	}
	if (!pMsgBufParam)
		pMsgBufParam = this;
	assert(pMsgBufParam);
	return pMsgBufParam;
}

/** \brief writes the access to a member of a specific type
 *  \param pFile the file to write to
 *  \param pFunction the function to write the member for
 *  \param nType the type of the message buffer struct
 *  \param nFEType the type of the member
 *  \param nIndex the number (0-based) of the member of the specific type
 *
 * First we look for the specific member and then call the WriteAccess method
 * with the member.
 */
void CBEMsgBuffer::WriteMemberAccess(CBEFile& pFile, CBEFunction *pFunction, CMsgStructType nType,
	int nFEType, int nIndex)
{
	// get struct
	CBEStructType *pStruct = GetStruct(pFunction, nType);
	assert(pStruct);
	// iterate members and count to the wanted type (remember: 0-based)
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = pStruct->m_Members.begin();
		iter != pStruct->m_Members.end();
		iter++)
	{
		if ((*iter)->GetType()->GetFEType() != nFEType)
			continue;

		CBEDeclarator *pMemberDecl = (*iter)->m_Declarators.First();
		CBEExpression *pBound = pMemberDecl->m_Bounds.First();
		//
		// FIXME: this currently only supports one array dimension
		//
		int nArrayCount = (pBound) ? pBound->GetIntValue() : 1;
		int nCurArrayIndex = nIndex;

		while (nArrayCount--)
		{
			// check for array
			if (nIndex == 0)
			{
				// call WriteAccess
				WriteAccess(pFile, pFunction, nType, *iter);
				if (pBound)
					pFile << "[" << nCurArrayIndex << "]";
				return;
			}
			nIndex--;
		}
	}
}

/** \brief writes the access to a member of a specific type
 *  \param pFile the file to write to
 *  \param pFunction the function to write the member for
 *  \param nType the type of the message buffer struct
 *  \param nFEType the type of the member
 *  \param sIndex the string used as index into the array, not used if empty
 *
 * First we look for the specific member and then call the WriteAccess method
 * with the member.
 */
void CBEMsgBuffer::WriteMemberAccess(CBEFile& pFile, CBEFunction *pFunction, CMsgStructType nType,
	int nFEType, std::string sIndex)
{
	// get struct
	CBEStructType *pStruct = GetStruct(pFunction, nType);
	assert(pStruct);
	// iterate members and count to the wanted type (remember: 0-based)
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = pStruct->m_Members.begin();
		iter != pStruct->m_Members.end();
		iter++)
	{
		if ((*iter)->GetType()->GetFEType() != nFEType)
			continue;

		WriteAccess(pFile, pFunction, nType, *iter);
		if (!sIndex.empty())
			pFile << "[" << sIndex << "]";
		return;
	}
}

/** \brief write the access to a member of the generic struct
 *  \param pFile the file to write to
 *  \param nIndex the index into the word array of the generic struct
 *
 * This basically writes access to a member of the array at the specified
 * index.
 */
void
CBEMsgBuffer::WriteGenericMemberAccess(CBEFile& pFile,
	int nIndex)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sStructName = pNF->GetMessageBufferStructName(CMsgStructType::Generic,
		string(), string());
	// FIXME: build declarator stack
	// write access
	string sName = m_Declarators.First()->GetName();
	if (HasReference())
		sName += "->";
	else
		sName += ".";
	// get name of word sized member
	string sMember = pNF->GetWordMemberVariable();
	pFile << sName << sStructName << "." << sMember << "[" << nIndex << "]";
}

/** \brief retrieve a reference to one of the structs
 *  \param nType the type of the message buffer struct
 *  \return a reference to the struct or NULL if none found
 */
CBEStructType* CBEMsgBuffer::GetStruct(CMsgStructType nType)
{
	CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
	return GetStruct(pFunction, nType);
}

/** \brief return the offset where the payload starts
 *  \return the offset in bytes
 */
int CBEMsgBuffer::GetPayloadOffset()
{
	return 0;
}

/** \brief return iterator pointing t start of payload
 *  \param pStruct the struct to use
 *  \return iterator pointing to first element of payload or end if no payload
 */
vector<CBETypedDeclarator*>::iterator CBEMsgBuffer::GetStartOfPayload(CBEStructType *pStruct)
{
	CBEMsgBufferType *pType = pStruct->GetSpecificParent<CBEMsgBufferType>();
	assert(pType);
	return pType->GetStartOfPayload(pStruct);
}

/** \brief retrieve the reference to one of the structs
 *  \param pFunction the function to get the struct for
 *  \param nType the type of the message buffer struct
 *  \return a reference to the struct or NULL if none found
 */
CBEStructType* CBEMsgBuffer::GetStruct(CBEFunction *pFunction, CMsgStructType nType)
{
	assert(pFunction);
	// get struct name
	string sFuncName = pFunction->GetOriginalName();
	string sClassName = pFunction->GetSpecificParent<CBEClass>()->GetName();
	// if name is empty, get generic struct
	if (sFuncName.empty())
	{
		nType = CMsgStructType::Generic;
		sClassName = string();
	}
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEMsgBuffer::%s: func name: %s, class name: %s, dir %d\n",
		__func__, sFuncName.c_str(), sClassName.c_str(), (int)nType);
	// get type
	CBEMsgBufferType *pType = GetType(pFunction);
	// get struct
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEMsgBuffer::%s return struct of type at %p (msgbuf type)\n", __func__,
		pType);
	return pType->GetStruct(sFuncName, sClassName, nType);
}

/** \brief determine the type of a struct
 *  \param pStruct the struct to check
 *  \return the type of the struct
 */
CMsgStructType CBEMsgBuffer::GetStructType(CBEStructType *pStruct)
{
	// get union case
	CBEUnionCase *pUnion = pStruct->GetSpecificParent<CBEUnionCase>();
	assert(pUnion);
	string sStructName = pUnion->m_Declarators.First()->GetName();
	// check name of union case
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetMessageBufferStructName(CMsgStructType::In, string(), string());
	if (sStructName.substr(sStructName.length() - sName.length()) == sName)
		return CMsgStructType::In;
	sName = pNF->GetMessageBufferStructName(CMsgStructType::Out, string(), string());
	if (sStructName.substr(sStructName.length() - sName.length()) == sName)
		return CMsgStructType::Out;
	sName = pNF->GetMessageBufferStructName(CMsgStructType::Exc, string(), string());
	if (sStructName.substr(sStructName.length() - sName.length()) == sName)
		return CMsgStructType::Exc;

	// otherwise it must be generic
	return CMsgStructType::Generic;
}

/** \brief test if the message buffer has a certain property
 *  \param nProperty the property to test for
 *  \param nType the type of the message buffer struct
 *  \return true if property is available for direction
 *
 * This implementation always returns false, because no properties to check
 * here.
 */
bool CBEMsgBuffer::HasProperty(int /*nProperty*/, CMsgStructType /*nType*/)
{
	return false;
}

/** \brief writes the initialization of specific members
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param nType the type of the members to initialize
 *  \param nStructType the type of the message buffer struct
 *
 * Here we have to allocate the message buffer dynamically if it has variable
 * sized members.
 */
void CBEMsgBuffer::WriteInitialization(CBEFile& /*pFile*/, CBEFunction* /*pFunction*/,
	int /*nType*/, CMsgStructType /*nStructType*/)
{
	// FIXME find variable sized members
	// FIXME allocate memory for message buffer
}

/** \brief find a specific members of the message buffer
 *  \param sName the name of the member
 *  \param nType the type of the message buffer struct
 *  \return a reference to the member if found, NULL if none
 */
CBETypedDeclarator* CBEMsgBuffer::FindMember(std::string sName, CMsgStructType nType)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMsgBuffer::%s(%s, %d) called\n",
		__func__, sName.c_str(), (int)nType);

	CBEStructType *pStruct = GetStruct(nType);
	if (!pStruct)
		return (CBETypedDeclarator*)0;

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s return result of struct's FindMember function\n",
		__func__);
	return pStruct->m_Members.Find(sName);
}

/** \brief find a specific members of the message buffer
 *  \param sName the name of the member
 *  \param pFunction the function to use as reference
 *  \param nType the type of the message buffer struct
 *  \return a reference to the member if found, NULL if none
 */
CBETypedDeclarator* CBEMsgBuffer::FindMember(std::string sName, CBEFunction *pFunction, CMsgStructType nType)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMsgBuffer::%s(%s, %s, %d) called\n",
		__func__, sName.c_str(), pFunction->GetName().c_str(), (int)nType);

	CBEStructType *pStruct = GetStruct(pFunction, nType);
	if (!pStruct)
		return (CBETypedDeclarator*)0;

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMsgBuffer::%s return result of struct's FindMember function\n",
		__func__);
	return pStruct->m_Members.Find(sName);
}

/** \brief try to get the position of a member counting word sizes
 *  \param sName the name of the member
 *  \param nType the type of the message buffer struct
 *  \return the position (index) of the member, -1 if not found
 */
int CBEMsgBuffer::GetMemberPosition(std::string sName, CMsgStructType nType)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	// test opcode
	if (sName == pNF->GetOpcodeVariable())
		return 0;
	// test exception
	if (sName == pNF->GetExceptionWordVariable())
		return 0;

	// get struct
	CBEStructType *pStruct = GetStruct(nType);
	assert(pStruct);
	CBESizes *pSizes = CCompiler::GetSizes();
	int nPos = 0;
	// iterate
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = pStruct->m_Members.begin();
		iter != pStruct->m_Members.end();
		iter++)
	{
		if ((*iter)->m_Declarators.Find(sName))
			return nPos;

		// by adding the number of positions the current member uses, we set
		// the position to an index pointing just behind that current member
		nPos += pSizes->WordsFromBytes((*iter)->GetSize());
	}

	// not found
	return -1;
}

/** \brief get the member of payload at a specific position
 *  \param nType the type of the message buffer struct
 *  \param nIndex the index to get the member at
 *  \return a reference to the member or NULL
 */
CBETypedDeclarator* CBEMsgBuffer::GetMemberAt(CMsgStructType nType, int nIndex)
{
	CBESizes *pSizes = CCompiler::GetSizes();
	int nCurSize = 0, nMemberSize, nPosSize = pSizes->GetSizeOfType(TYPE_MWORD);
	// get struct
	CBEStructType *pStruct = GetStruct(nType);
	assert(pStruct);
	// try to find the member for the position
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = GetStartOfPayload(pStruct);
		iter != pStruct->m_Members.end();
		iter++)
	{
		// direction fits, since this is the struct of our desired direction
		nMemberSize = GetMemberSize(TYPE_MWORD, *iter, false);
		// if we cross the border of the parameter position we want, stop
		if (nIndex * nPosSize < (nCurSize + nMemberSize))
			return *iter;
		nCurSize += nMemberSize;
	}
	return 0;
}

/** \brief writes a dump of the message buffer
 *  \param pFile the file to write to
 *
 * Do we dump generic struct or specific struct? If generic: what do we do if
 * it does not exist? Specific: How do we handle single elements?
 *
 * We have to respect the -ftrace-msgbuf-dump-words settings
 */
void CBEMsgBuffer::WriteDump(CBEFile& pFile)
{
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF))
		return;

	// if TRACE_MSGBUF is set a variable _i is specified
	// FIXME: make _i name factory string
	string sVar = string("_i");
	string sFunc;
	CCompiler::GetBackEndOption("trace-msgbuf-func", sFunc);
	CBEDeclarator *pDecl = m_Declarators.First();

	// cast message buffer to word buffer and start iterating
	pFile << "\tfor (" << sVar << "=0; " << sVar << "<";
	if (CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF_DWORDS))
		pFile << CCompiler::GetTraceMsgBufDwords();
	else
	{
		pFile << "sizeof(";
		if (pDecl->GetStars() > 0)
			pFile << "*";
		pFile << pDecl->GetName() << ")/4";
	}
	pFile << "; " << sVar << "++)\n";
	pFile << "\t{\n";
	// print four words in one line:
	// test for begin of wrap-around
	++pFile << "\tif (" << sVar << "%4 == 0)\n";
	++pFile << "\t" << sFunc << " (\"dwords[%d]: \", " << sVar << ");\n";

	// print current word
	--pFile << "\t" << sFunc << " (\"%08x \", ((unsigned long*)(";
	if (pDecl->GetStars() == 0)
		pFile << "&";
	pFile << pDecl->GetName() << "))[" << sVar << "]);\n";

	// print newline
	pFile << "\tif (" << sVar << "%4 == 3)\n";
	++pFile << "\t" << sFunc << " (\"\\n\");\n";

	// for loop end
	--(--pFile) << "\t}\n";
}

/** \brief the post-create (and post-sort) step during creation
 *  \param pFunction the function owning this message buffer
 *  \param pFEOperation the front-end reference operation
 *  \return true if successful
 */
void CBEMsgBuffer::PostCreate(CBEFunction *pFunction, CFEOperation *pFEOperation)
{
	AddGenericStruct(pFunction, pFEOperation);
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s calling Pad for func %s\n",
		__func__, pFunction->GetName().c_str());
	Pad();
}

/** \brief the post-create (and post-sort) step during creation
 *  \param pClass the class owning this message buffer
 *  \param pFEInterface the front-end reference interface
 *  \return true if successful
 */
void CBEMsgBuffer::PostCreate(CBEClass *pClass, CFEInterface *pFEInterface)
{
	AddGenericStruct(pClass, pFEInterface);
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s calling Pad for class %s\n",
		__func__, pClass->GetName().c_str());
	Pad();
}

/** \brief calculate the number of word sized members required for generic \
 *         server message buffer
 *  \return the number of word sized members
 *
 * Here we should calculate the maximum size of all the other structs and
 * use it as value.
 */
int CBEMsgBuffer::GetWordMemberCountFunction()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s() called\n", __func__);
	CBEType *pType = CBETypedef::GetType();
	if (!pType)
		return 0;
	int ret = pType->GetSize();
	if (ret < 0)
		ret = pType->GetMaxSize();
	// calculate words from bytes
	CBESizes *pSizes = CCompiler::GetSizes();
	ret = pSizes->WordRoundUp(ret);
	return ret;
}

/** \brief calculate the number of word sized members required for generic \
 *         server message buffer
 *  \return the number of word sized members
 *
 * Here we should calculate the maximum size of all the other structs and
 * use it as value.
 */
int CBEMsgBuffer::GetWordMemberCountClass()
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s() called\n", __func__);
	CBEType *pType = CBETypedef::GetType();
	if (!pType)
		return 0;
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s: type is %d\n", __func__,
		pType->GetFEType());
	int ret = pType->GetSize();
	if (ret < 0)
		ret = pType->GetMaxSize();
	// calculate words from bytes
	CBESizes *pSizes = CCompiler::GetSizes();
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s returns words-from-bytes(%d)\n",
		__func__, ret);
	ret = pSizes->WordsFromBytes(pSizes->WordRoundUp(ret));
	return ret;
}

/** \brief adds a generic structure to the message buffer
 *  \param pFunction the function which owns the message buffer
 *  \param pFEOperation the front-end operation to use as reference
 *  \return true if successful
 */
void CBEMsgBuffer::AddGenericStruct(CBEFunction *pFunction, CFEOperation *pFEOperation)
{
	// if either one has not the desired number of word sized memebers, we add
	// the short IPC struct
	// get any structure
	CBEStructType *pStruct = GetStruct(pFunction, CMsgStructType::In);
	assert(pStruct);
	// get message buffer type
	CBEMsgBufferType *pType =
		pStruct->GetSpecificParent<CBEMsgBufferType>();
	assert(pType);
	// add generic struct to type
	pType->AddGenericStruct(pFEOperation);
	pStruct = GetStruct(pFunction, CMsgStructType::Generic);
	assert(pStruct);
	// add the required members
	AddGenericStructMembersFunction(pStruct);
	// now we have to repeat the initialization steps (if they apply)
	AddPlatformSpecificMembers(pFunction, pStruct);
	Sort(pStruct);
}

/** \brief creates an array member of elements for the message buffer
 *  \param nFEType the type of the member
 *  \param bUnsigned true if type should be unsigned
 *  \param sName the name of the member
 *  \param nArray the number of elements
 *  \return a reference to the newly created opcode member
 */
CBETypedDeclarator* CBEMsgBuffer::GetMemberVariable(int nFEType, bool bUnsigned, std::string sName, int nArray)
{
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBEType *pType = pCF->GetNewType(nFEType);
	pType->CreateBackEnd(bUnsigned, 0, nFEType);
	CBETypedDeclarator *pWordMember = pCF->GetNewTypedDeclarator();
	pWordMember->CreateBackEnd(pType, sName);
	delete pType; // cloned in CBETypedDeclarator::CreateBackEnd
	// set array dimension
	if (nArray > 0)
	{
		CBEDeclarator *pDecl = pWordMember->m_Declarators.First();
		CBEExpression *pBound = pCF->GetNewExpression();
		pBound->CreateBackEnd(nArray);
		pDecl->AddArrayBound(pBound);
	}
	// add directional attribute so later checks when marshaling work
	CBEAttribute *pAttr = pCF->GetNewAttribute();
	pAttr->SetParent(pWordMember);
	pAttr->CreateBackEnd(ATTR_IN);
	pWordMember->m_Attributes.Add(pAttr);
	// OUT
	pAttr = pCF->GetNewAttribute();
	pAttr->SetParent(pWordMember);
	pAttr->CreateBackEnd(ATTR_OUT);
	pWordMember->m_Attributes.Add(pAttr);
	return pWordMember;
}

/** \brief creates an array member of word sized elements for the message buffer
 *  \param nNumber the number of elements
 *  \return a reference to the newly created member
 */
CBETypedDeclarator* CBEMsgBuffer::GetWordMemberVariable(int nNumber)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetWordMemberVariable();
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"%s: creating member %s with %d dimensions\n", __func__,
		sName.c_str(), nNumber);
	return GetMemberVariable(TYPE_MWORD, true, sName, nNumber);
}

/** \brief adds a generic struct to message buffer of class
 *  \param pClass the class the message buffer belongs to
 *  \param pFEInterface the front-end interface used as reference
 *  \return true on success
 */
void CBEMsgBuffer::AddGenericStruct(CBEClass *pClass, CFEInterface *pFEInterface)
{
	// get message buffer type
	CBEMsgBufferType *pType = GetType(0);
	// add generic struct to type
	pType->AddGenericStruct(pFEInterface);
	// get any function (server loop is not created yet) to get struct
	CBEFunction *pFunction = GetAnyFunctionFromClass(pClass);
	assert(pFunction);
	CBEStructType *pStruct = GetStruct(pFunction, CMsgStructType::Generic);
	assert(pStruct);
	// add the members
	AddGenericStructMembersClass(pStruct);
	// now we have to repeat the initialization steps (if they apply)
	AddPlatformSpecificMembers(pFunction, pStruct);
	Sort(pStruct);
}

/** \brief try to extract any function from a class
 *  \param pClass the class to get the function from
 *  \return a reference to the function if any found
 */
CBEFunction* CBEMsgBuffer::GetAnyFunctionFromClass(CBEClass *pClass)
{
	CBEFunction *pFunction = pClass->m_Functions.First();
	if (pFunction)
		return pFunction;

	// try to get function from base classes (this class might be just an
	// empty class derived from other classes)
	vector<CBEClass*>::iterator iC = pClass->m_BaseClasses.begin();
	for (; iC != pClass->m_BaseClasses.end(); iC++)
	{
		if ((pFunction = GetAnyFunctionFromClass(*iC)))
			return pFunction;
	}

	return pFunction;
}

/** \brief adds the members of to the generic struct
 *  \param pStruct the struct to add the members to
 *  \return true if successful
 */
void CBEMsgBuffer::AddGenericStructMembersClass(CBEStructType *pStruct)
{
	// add the required number of members
	int nMembers = GetWordMemberCountClass();
	CBETypedDeclarator *pMember = GetWordMemberVariable(nMembers);
	assert(pMember);
	pStruct->m_Members.Add(pMember);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMsgBuffer::%s added word member\n", __func__);
}

/** \brief adds the members of to the generic struct
 *  \param pStruct the struct to add the members to
 *  \return true if successful
 */
void CBEMsgBuffer::AddGenericStructMembersFunction(CBEStructType *pStruct)
{
	// add the required number of members
	int nMembers = GetWordMemberCountFunction();
	CBETypedDeclarator *pMember = GetWordMemberVariable(nMembers);
	if (!pMember)
		throw error::create_error("word member variable could not be created");
	pStruct->m_Members.Add(pMember);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMsgBuffer::%s added word member\n", __func__);
}

/** \brief return the size of a specific struct
 *  \param pFunction the function for which to get the size
 *  \param nType the struct type to count
 *  \return the size of the respective struct
 */
int CBEMsgBuffer::GetSize(CBEFunction *pFunction, CMsgStructType nType)
{
	CBEStructType *pStruct = GetStruct(pFunction, nType);
	if (!pStruct)
		return 0;
	return pStruct->GetSize();
}

/** \brief returns the maximum size of a specific struct
 *  \param pFunction the function for which to get the size
 *  \param nType the type of the struct
 *  \retval nSize the size of the struct
 *  \return true if maximum could be evaluated
 */
bool CBEMsgBuffer::GetMaxSize(int& nSize, CBEFunction *pFunction, CMsgStructType nType)
{
	CBEStructType *pStruct = GetStruct(pFunction, nType);
	if (!pStruct)
		return false;
	nSize = pStruct->GetMaxSize();
	return nSize >= 0;
}

/** \brief gets the size of specific message buffer members
 *  \param nType the type of message buffer members requested (0 all)
 *  \return the number of elements of that specific type, or if type is 0, \
 *          the size of the message buffer in bytes
 *
 * This method only counts the payload.
 */
int CBEMsgBuffer::GetMemberSize(int nType)
{
	int nMaxSize = 0;
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMsgBuffer::%s(%d) called\n", __func__, nType);

	// iterate over structs of type and get refstring numbers of each struct
	CBEMsgBufferType *pType = GetType(0);

	int c = 0;
	vector<CBEUnionCase*>::iterator iterS;
	for (iterS = pType->m_UnionCases.begin();
		iterS != pType->m_UnionCases.end();
		iterS++)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CBEMsgBuffer::%s: %d. union\n", __func__,
			++c);
		CBEStructType *pStruct = dynamic_cast<CBEStructType*>((*iterS)->GetType());
		if (!pStruct)
			continue;

		int nSize = 0;

		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CBEMsgBuffer::%s: iterate members of struct\n",
			__func__);
		vector<CBETypedDeclarator*>::iterator iterP;
		for (iterP = GetStartOfPayload(pStruct);
			iterP != pStruct->m_Members.end();
			iterP++)
		{
			nSize += GetMemberSize(nType, *iterP, false);
		}

		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
			"CBEMsgBuffer::%s: size of current struct is %d (max is %d)\n", __func__, nSize,
			nMaxSize);

		if (nSize > nMaxSize)
			nMaxSize = nSize;
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMsgBuffer::%s returns %d\n", __func__, nMaxSize);
	return nMaxSize;
}

/** \brief gets the size of specific message buffer members
 *  \param nType the type of message buffer members requested (0 all)
 *  \param pFunction the function of the message buffer
 *  \param nStructType the type of the message buffer struct
 *  \param bMax determine maximum size rather than actual size
 *  \return the number of elements of that specific type, or if type is 0, \
 *          the size of the message buffer in bytes
 *
 * This method only counts the payload.
 */
int CBEMsgBuffer::GetMemberSize(int nType, CBEFunction *pFunction, CMsgStructType nStructType, bool bMax)
{
	int nSize = 0;
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMsgBuffer::%s (%d, %s, %d, %s) called\n",
		__func__, nType, pFunction->GetName().c_str(), (int)nStructType,
		bMax ? "true" : "false");

	CBEStructType *pStruct = GetStruct(pFunction, nStructType);
	assert(pStruct);
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CBEMsgBuffer::%s: struct at %p\n", __func__, pStruct);

	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = GetStartOfPayload(pStruct);
		iter != pStruct->m_Members.end();
		iter++)
	{
		int nMemberSize = GetMemberSize(nType, *iter, bMax);
		CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "CBEMsgBuffer::%s size of member %s is %d\n",
			__func__, (*iter)->m_Declarators.First()->GetName().c_str(), nMemberSize);
		// if member with variable size, return variable size
		if (nMemberSize < 0)
			return -1;
		nSize += nMemberSize;
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMsgBuffer::%s returns %d\n", __func__, nSize);
	return nSize;
}

/** \brief determine the size of a specific member
 *  \param nType the type requested to determine the size
 *  \param pMember the member to get the size for
 *  \param bMax true if the maximum size should be determined
 *  \return the size of the member
 */
int CBEMsgBuffer::GetMemberSize(int nType, CBETypedDeclarator *pMember, bool bMax)
{
	int nMemberSize = 0;
	// if we count anything (0) or in word size, then add the size of the
	// member in bytes
	// or: if the member is of the requested type, add its size
	if ((nType == 0) || (nType == TYPE_MWORD) ||
		pMember->GetType()->IsOfType(nType))
	{
		if (bMax)
		{
			pMember->GetMaxSize(nMemberSize);
		}
		else
		{
			nMemberSize = pMember->GetSize();
			if (nMemberSize < 0 && pMember->m_Attributes.Find(ATTR_STRING))
				nMemberSize = 1;
		}
	}

	return nMemberSize;
}

/** \brief pads elements of the message buffer
 *  \return false if an error occured
 *
 * The elements of the message buffer can be padded to fit requirements of the
 * underlying communication platform or for optimization purposes (if send
 * buffer should be reused as receive buffer).
 */
void CBEMsgBuffer::Pad()
{ }

/** \brief check if one member comes before another in the message buffer
 *  \param pFunction the function for which the message buffer is needed
 *  \param nType the type of the message buffer struct
 *  \param sName1 the name of the first member
 *  \param sName2 the name of the second member
 *  \return true if member sName1 comes before member sName2
 */
bool CBEMsgBuffer::IsEarlier(CBEFunction *pFunction, CMsgStructType nType,
	std::string sName1, std::string sName2)
{
	CBEStructType *pStruct = GetStruct(pFunction, nType);
	assert(pStruct);

	if (!pStruct->m_Members.Find(sName1) ||
		!pStruct->m_Members.Find(sName2))
		std::__throw_out_of_range("CBEMsgBuffer::IsEarlier");

	CStructMembers::iterator i1 = std::find_if(pStruct->m_Members.begin(),
		pStruct->m_Members.end(), MemFind(sName1));
	CStructMembers::iterator i2 = std::find_if(pStruct->m_Members.begin(),
		pStruct->m_Members.end(), MemFind(sName2));

	return (i2 - i1) > 0;
}

