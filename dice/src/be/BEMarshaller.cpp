/**
 *  \file    dice/src/be/BEMarshaller.cpp
 *  \brief   contains the implementation of the class CBEMarshaller
 *
 *  \date    11/18/2004
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "BEMarshaller.h"
#include "BEInterfaceFunction.h"
#include "BEOperationFunction.h"
#include "BEMarshalFunction.h"
#include "BEMarshalExceptionFunction.h"
#include "BEUnmarshalFunction.h"
#include "BEReplyFunction.h"
#include "BESndFunction.h"
#include "BEMsgBuffer.h"
#include "BEMsgBufferType.h"
#include "BEStructType.h"
#include "BEDeclarator.h"
#include "BEFile.h"
#include "BENameFactory.h"
#include "BEContext.h"
#include "BEAttribute.h"
#include "BEUnionCase.h"
#include "BEIDLUnionType.h"
#include "BEExpression.h"
#include "BEUserDefinedType.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"
#include "Compiler.h"
#include <stdexcept>
#include <cassert>
#include <iosfwd>
#include <iostream>
#include <sstream>

CBEMarshaller::CBEMarshaller()
: CBEObject()
{
	m_pFunction = 0;
}

/** destroys the object */
CBEMarshaller::~CBEMarshaller()
{ }

/** \brief adds local variables to a function
 *  \param pFunction the function to add the variables to
 */
void CBEMarshaller::AddLocalVariable(CBEFunction*)
{ }

/** \brief retrieve a reference to the struct member of the message buffer
 *  \param nType the type of the message buffer struct
 *  \return a reference to the struct iff any is found
 */
CBEStructType*
CBEMarshaller::GetStruct(CMsgStructType& nType)
{
	CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
	assert(pFunction);
	return GetStruct(pFunction, nType);
}

/** \brief retrieve a reference to the struct member of a function's msgbuf
 *  \param pFunction the function to get the message buffer from
 *  \param nType the type of the message buffer struct
 *  \return a reference to the struct iff any is found
 */
CBEStructType*
CBEMarshaller::GetStruct(CBEFunction *pFunction,
	CMsgStructType& nType)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMarshaller::%s(%s, %d) called\n",
		__func__, pFunction->GetName().c_str(), (int)nType);

	// check function's type to get name
	string sFuncName = pFunction->GetOriginalName();
	string sClassName = pFunction->GetSpecificParent<CBEClass>()->GetName();
	// if name is empty, get generic struct
	if (sFuncName.empty())
	{
		nType = CMsgStructType::Generic;
		sClassName = string();
	}
	// get the message buffer type
	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	assert(pMsgBuffer);
	// get type and convert it to message buffer type
	CBEMsgBufferType *pType = pMsgBuffer->GetType(pFunction);
	// get struct from type
	return pType->GetStruct(sFuncName, sClassName, nType);
}

/** \brief get the message buffer suited for the given function
 *  \param pFunction the function to get the message buffer for
 *  \return a reference to the message buffer or NULL if none available
 */
CBEMsgBuffer*
CBEMarshaller::GetMessageBuffer(CBEFunction *pFunction)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s(%s) called\n", __func__,
		pFunction->GetName().c_str());
	// get the message buffer type
	if (dynamic_cast<CBEInterfaceFunction*>(pFunction) ||
		dynamic_cast<CBEUnmarshalFunction*>(pFunction) ||
		dynamic_cast<CBEMarshalFunction*>(pFunction) ||
		dynamic_cast<CBEMarshalExceptionFunction*>(pFunction))
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s return class' message buffer\n", __func__);
		CBEClass *pClass = pFunction->GetSpecificParent<CBEClass>();
		assert(pClass);
		return pClass->GetMessageBuffer();
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s return function's message buffer\n", __func__);
	return pFunction->GetMessageBuffer();
}

/** \brief write the marshaling code for a whole function
 *  \param pFile the file to marshal to
 *  \param nType the direction to marshal
 */
void
CBEMarshaller::MarshalFunction(CBEFile& pFile,
	CMsgStructType nType)
{
	// get function
	CBEFunction *pFunction = GetSpecificParent<CBEFunction>();
	assert(pFunction);
	MarshalFunction(pFile, pFunction, nType);
}

/** \brief write the marshaling code for a whole function
 *  \param pFile the file to write to
 *  \param pFunction the function to write the marshaling code for
 *  \param nType the direction to marshal
 *
 * This method simply iterates the parameters of the function, tests if they
 * should be marshaled and calls the marshal method. We do NOT use the
 * members, because there are members, which do not have parameters associated
 * with them (resulting in errors when looking for the parameter) or
 * parameters requiring "special treatment".
 */
void
CBEMarshaller::MarshalFunction(CBEFile& pFile,
	CBEFunction *pFunction,
	CMsgStructType nType)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s(%s, %s, %d) called\n", __func__,
		pFile.GetFileName().c_str(), pFunction->GetName().c_str(), (int)nType);

	// get struct
	CBEStructType *pStruct = GetStruct(pFunction, nType);
	// if the function has no struct, then it should not be marshalled. E.g.
	// wait-any
	if (!pStruct)
	{
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s returns (no struct)\n", __func__);
		return;
	}

	// set member variables
	m_bMarshal = nType == pFunction->GetSendDirection();
	m_pFunction = pFunction;
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s %smarshals function %s for dir %d\n", __func__,
		m_bMarshal ? "" : "un", pFunction->GetName().c_str(),
		(int)nType);

	// to maintain the order of the members in the struct, we iterate
	// the struct members, get the respective parameter or local variable
	// and then marshal it.
	// get the message buffer type
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEMarshaller::%s get message buffer for function %s\n", __func__,
		pFunction->GetName().c_str());
	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	assert(pMsgBuffer);
	CBEMsgBufferType *pType = pMsgBuffer->GetType(pFunction);
	// get start of payload
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = pType->GetStartOfPayload(pStruct);
		iter != pStruct->m_Members.end();
		iter++)
	{
		// get respective parameter
		// or local variable
		string sName = (*iter)->m_Declarators.First()->GetName();
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s marshalling struct member %s (@ %p)\n", __func__,
			sName.c_str(), *iter);
		CBETypedDeclarator *pParameter = pFunction->FindParameter(sName);
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s marshalling parameter in func %s at %p\n", __func__,
			pFunction->GetName().c_str(), pParameter);
		if (!pParameter)
			pParameter = pFunction->m_LocalVariables.Find(sName);
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s marshalling parameter in func %s at %p (local)\n", __func__,
			pFunction->GetName().c_str(), pParameter);
		// check if this member should be skipped
		// Do this before skipping in case of missing parameter, because this
		// member may influence later skip decisions (other member come later
		// and DoSkipParameter increments m_nSkipSize).
		if (DoSkipParameter(pFunction, pParameter ? pParameter : *iter,
				nType))
			continue;

		// if the member does not have a corresponding parameter, it might be
		// some special member such as opcode or zero flexpage member. The
		// size members are found as local variables and can thus not be
		// cought by this rule
		if (!pParameter)
			pParameter = *iter;

		// now marshal parameter
		CDeclStack vStack;
		vStack.push_back(pParameter->m_Declarators.First());
		MarshalParameterIntern(pFile, pParameter, &vStack);
	}

	m_pFunction = 0;

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s(,,) returns\n", __func__);
}

/** \brief tests if this parameter should be marshalled
 *  \param pFunction the function the parameter belongs to
 *  \param pParameter the parameter (!) to be tested
 *  \param nType the direction of the marshalling
 *  \return true if this parameter should be skipped
 *
 * Test if the direction fits, if the parameter has the IGNORE attribute and
 * if the function allows the marshaling of this parameter.
 */
bool
CBEMarshaller::DoSkipParameter(CBEFunction* pFunction,
	CBETypedDeclarator *pParameter,
	CMsgStructType nType)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s(%s, %s, %d) called\n", __func__,
		pFunction->GetName().c_str(),
		pParameter->m_Declarators.First()->GetName().c_str(),
		(int)nType);
	if (!pParameter->IsDirection(nType))
	{
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s: wrong direction\n", __func__);
		return true;
	}
	if (pParameter->m_Attributes.Find(ATTR_IGNORE))
	{
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s: ignore\n", __func__);
		return true;
	}
	if (!pFunction->DoMarshalParameter(pParameter, m_bMarshal))
	{
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s: dont marshal\n", __func__);
		return true;
	}
	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s returns false\n", __func__);
	return false;
}

/** \brief marshals a single parameter
 *  \param pFile the file to marshal to
 *  \param pFunction the function the parameter belongs to
 *  \param pParameter the parameter to marshal
 *  \param bMarshal true if marshaling, false if unamrshaling
 */
void
CBEMarshaller::MarshalParameter(CBEFile& pFile,
	CBEFunction *pFunction,
	CBETypedDeclarator *pParameter,
	bool bMarshal)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s called for func %s and param %s (%s)\n",
		__func__, pFunction ? pFunction->GetName().c_str() : "(none)",
		pParameter ? pParameter->m_Declarators.First()->GetName().c_str() : "(none)",
		bMarshal ? "marshalling" : "unmarshalling");

	// get the message buffer type
	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	assert(pMsgBuffer);

	// set member variables
	m_bMarshal = bMarshal;
	m_pFunction = pFunction;

	CMsgStructType nType = CMsgStructType::Generic;
	if (bMarshal)
		nType = pFunction->GetSendDirection();
	else
		nType = pFunction->GetReceiveDirection();
	// get struct
	CBEStructType *pStruct = GetStruct(pFunction, nType);
	// there always should be a struct
	assert(pStruct);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s got %p and direction %d\n",
		__func__, pStruct, (int)nType);

	CDeclStack vStack;
	vStack.push_back(pParameter->m_Declarators.First());
	if (CMsgStructType::Generic == nType)
	{
		string sName = pParameter->m_Declarators.First()->GetName();
		// if direction has been changed to 0 then this is a generic struct
		// and we have to get the supposed position to marshal the parameter
		// to
		int nPosition = pMsgBuffer->GetMemberPosition(sName, nType);
		// write access to generic member
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s calling MarshalGenericMember for pos %d\n", __func__, nPosition);
		MarshalGenericMember(pFile, nPosition, pParameter, &vStack);
	}
	else
	{
		if (!DoSkipParameter(pFunction, pParameter, nType))
		{
			CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
				"CBEMarshaller::%s calling MarshalParameterIntern\n", __func__);
			MarshalParameterIntern(pFile, pParameter, &vStack);
		}
	}

	m_pFunction = 0;

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMarshaller::%s done.\n", __func__);
}

/** \brief marshals a value at the position of a parameter
 *  \param pFile the file to marshal to
 *  \param pFunction the function the parameter belongs to
 *  \param pParameter the parameter to marshal
 *  \param nValue the value to marshal
 */
void
CBEMarshaller::MarshalValue(CBEFile& pFile,
	CBEFunction *pFunction,
	CBETypedDeclarator *pParameter,
	int nValue)
{
	// get the message buffer type
	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	assert(pMsgBuffer);

	// set member variables
	m_bMarshal = true;
	m_pFunction = pFunction;

	CMsgStructType nType(pFunction->GetSendDirection());
	// get struct
	CBEStructType *pStruct = GetStruct(pFunction, nType);
	// there always should be a struct
	assert(pStruct);

	if (CMsgStructType::Generic == nType)
	{
		string sName = pParameter->m_Declarators.First()->GetName();
		// if direction has been changed to 0 then this is a generic struct
		// and we have to get the supposed position to marshal the parameter
		// to
		int nPosition = pMsgBuffer->GetMemberPosition(sName, nType);
		// write access to generic member
		MarshalGenericValue(pFile, nPosition, nValue);
	}
	else
	{
		if (!DoSkipParameter(pFunction, pParameter, nType))
		{
			// try to find respective member and assign
			pFile << "\t";
			WriteMember(pFile, nType, pMsgBuffer, pParameter, 0);
			pFile << " = " << nValue << ";\n";
		}
	}

	m_pFunction = 0;
}

/** \brief internal method to marshal a parameter
 *  \param pFile the file to write to
 *  \param pParameter the parameter to marshal
 *  \param pStack the declarator stack
 *
 * This method decides which strategy should be used to marshal the given
 * parameter. It also checks if there is a special treatment necessary.
 */
void
CBEMarshaller::MarshalParameterIntern(CBEFile& pFile, CBETypedDeclarator *pParameter,
	CDeclStack* pStack)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMarshaller::%s(%s) called\n", __func__,
		pParameter->m_Declarators.First()->GetName().c_str());

	if (MarshalSpecialMember(pFile, pParameter))
		return;
	// try to marshal strings
	if (MarshalString(pFile, pParameter, pStack))
		return;
	// try to marshal arrays (if it _does_ marshal an array it returns true)
	if (MarshalArray(pFile, pParameter, pStack))
		return;
	// try to marshal union
	// before struct, since IDL union is derived from struct
	if (MarshalUnion(pFile, pParameter, pStack))
		return;
	// try to marshal  struct
	if (MarshalStruct(pFile, pParameter, pStack))
		return;
	// FIXME test for enum

	// now this is a simple type:
	WriteAssignment(pFile, pParameter, pStack);

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMarshaller::%s(%s) returns\n",
		__func__, pParameter->m_Declarators.First()->GetName().c_str());
}

/** \brief marshals a member in the generic struct
 *  \param pFile the file to write to
 *  \param nPosition the position in the generic struct to marshal
 *  \param pParameter the parameter to marshal to this position
 *  \param pStack the declarator stack
 *
 * Do not use the GetMessageBuffer method of the marshaller, because it would
 * deliver the class' message buffer. But this message buffer cannot be used
 * to write access to the word sized members. We have to use the function's
 * message buffer.
 *
 * We do not adapt this method to handle arrays and structs, since the generic
 * struct should only contain an array of word sized members.
 */
void CBEMarshaller::MarshalGenericMember(CBEFile& pFile, int nPosition, CBETypedDeclarator *pParameter,
	CDeclStack* pStack)
{
	CBEMsgBuffer *pMsgBuffer = m_pFunction->GetMessageBuffer();
	assert(pMsgBuffer);

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s (%d, %s) called\n", __func__,
		nPosition, pParameter->m_Declarators.First()->GetName().c_str());

	pFile << "\t";
	if (m_bMarshal)
	{
		pMsgBuffer->WriteGenericMemberAccess(pFile, nPosition);
		pFile << " = ";
		WriteParameter(pFile, pParameter, pStack, false);
	}
	else
	{
		WriteParameter(pFile, pParameter, pStack, false);
		pFile << " = ";
		pMsgBuffer->WriteGenericMemberAccess(pFile, nPosition);
	}
	pFile << ";\n";

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s done.\n", __func__);
}

/** \brief marshals a value in the generic struct
 *  \param pFile the file to write to
 *  \param nPosition the position in the generic struct to marshal
 *  \param nValue the value to marshal to this position
 */
void CBEMarshaller::MarshalGenericValue(CBEFile& pFile, int nPosition, int nValue)
{
	if (!m_bMarshal)
		return;

	CBEMsgBuffer *pMsgBuffer = m_pFunction->GetMessageBuffer();
	assert(pMsgBuffer);

	pFile << "\t";
	pMsgBuffer->WriteGenericMemberAccess(pFile, nPosition);
	pFile << " = " << nValue << ";\n";
}

/** \brief test for and marshal special members, such as opcode or exception
 *  \param pFile the file to write to
 *  \param pMember the member to marshal
 *  \return true if this was an special parameter
 *
 * This implementation checks for opcode and exception member.
 */
bool CBEMarshaller::MarshalSpecialMember(CBEFile& pFile, CBETypedDeclarator *pMember)
{
	assert(pMember);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMarshaller::%s(%s) called\n",
		__func__, pMember->m_Declarators.First()->GetName().c_str());
	// check for opcode
	if (MarshalOpcode(pFile, pMember))
		return true;
	// check for exception
	if (MarshalException(pFile, pMember))
		return true;
	// return variable
	if (MarshalReturn(pFile, pMember))
		return true;

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMarshaller::%s(%s) returns false\n",
		__func__, pMember->m_Declarators.First()->GetName().c_str());
	return false;
}

/** \brief test for and marshal opcode
 *  \param pFile the file to write to
 *  \param pMember the member to marshal
 *  \return true if this was an special parameter
 */
bool CBEMarshaller::MarshalOpcode(CBEFile& pFile, CBETypedDeclarator *pMember)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMarshaller::%s(%s) called\n",
		__func__, pMember->m_Declarators.First()->GetName().c_str());

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetOpcodeVariable();
	// check name of member
	if (!pMember->m_Declarators.Find(sName))
		return false;

	// get message buffer
	CBEMsgBuffer *pMsgBuffer = m_pFunction->GetMessageBuffer();
	assert(pMsgBuffer);

	if (m_bMarshal)
	{
		// marshal opcode
		pFile << "\t";
		WriteMember(pFile, m_pFunction->GetSendDirection(), pMsgBuffer, pMember, 0);
		pFile << " = ";
		pFile << m_pFunction->GetOpcodeConstName();
		pFile << ";\n";
	}
	else
	{
		// unmarshal into opcode variable
		pFile << "\t" << sName << " = ";
		// access message buffer
		WriteMember(pFile, m_pFunction->GetReceiveDirection(), pMsgBuffer, pMember, 0);
		pFile << ";\n";
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMarshaller::%s(%s) returns true\n",
		__func__, pMember->m_Declarators.First()->GetName().c_str());
	return true;
}

/** \brief test for and marshal exception
 *  \param pFile the file to write to
 *  \param pMember the member to marshal
 *  \return true if this was an special parameter
 *
 * If unmarshalling, first test diretly in the message buffer if there was an
 * exception. Only then, store this exception in the environment.  This saves
 * us an indirect access in the nominal case.
 */
bool CBEMarshaller::MarshalException(CBEFile& pFile, CBETypedDeclarator *pMember)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMarshaller::%s(%s) called\n",
		__func__, pMember->m_Declarators.First()->GetName().c_str());

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetExceptionWordVariable();
	if (!pMember->m_Declarators.Find(sName))
		return false;

	// get message buffer
	CBEMsgBuffer *pMsgBuffer = m_pFunction->GetMessageBuffer();

	CBEDeclarator *pEnv = m_pFunction->GetEnvironment()->m_Declarators.First();
	string sEnvPtr;
	if (pEnv->GetStars() == 0)
		sEnvPtr = "&";
	sEnvPtr += pEnv->GetName();
	string sType = pNF->GetTypeName(TYPE_EXCEPTION, true);

	if (m_bMarshal)
	{
		// marshal exception
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_C))
		{
			pFile << "\t";
			WriteMember(pFile, m_pFunction->GetSendDirection(), pMsgBuffer, pMember, 0);
			pFile << " = ((" << sType << "){ _corba: { .major = " <<
				"DICE_EXCEPTION_MAJOR(" << sEnvPtr << "), .repos_id = " <<
				"DICE_EXCEPTION_MINOR(" << sEnvPtr << ") }})._raw;\n";
		}
		else if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
		{
			string sEnv = pEnv->GetName();
			if (pEnv->GetStars() == 0)
				sEnv += ".";
			else
				sEnv += "->";
			pFile << "\t";
			WriteMember(pFile, m_pFunction->GetSendDirection(), pMsgBuffer, pMember, 0);
			pFile << " = " << sEnv << "_exception._raw;\n";
		}
	}
	else
	{
		CMsgStructType nType(m_pFunction->GetReceiveDirection());
		// test if we really received an exception
		// if (env->major != CORBA_NO_EXCEPTION) => if (_exception != 0)
		pFile << "\tif (DICE_EXPECT_FALSE(";
		WriteMember(pFile, nType, pMsgBuffer, pMember, 0);
		pFile << " != 0))\n";
		pFile << "\t{\n";
		++pFile;
		// now assign the values
		if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_C))
		{
			// env->major = ((dice_CORBA_exception_type)exception).major
			// env->repos_id = ((dice_CORBA_exception_type)exception).repos_id
			pFile << "\tDICE_EXCEPTION_MAJOR(" << sEnvPtr << ") = ((" <<
				sType << "){ ._raw = ";
			// access message buffer
			WriteMember(pFile, nType, pMsgBuffer, pMember, 0);
			pFile << "})._corba.major;\n";
			pFile << "\tDICE_EXCEPTION_MINOR(" << sEnvPtr << ") = ((" <<
				sType << "){ ._raw = ";
			// access message buffer
			WriteMember(pFile, nType, pMsgBuffer, pMember, 0);
			pFile << "})._corba.repos_id;\n";
		}
		else if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
		{
			string sEnv = pEnv->GetName();
			if (pEnv->GetStars() == 0)
				sEnv += ".";
			else
				sEnv += "->";
			pFile << "\t" << sEnv << "_exception._raw = ";
			// access message buffer
			WriteMember(pFile, nType, pMsgBuffer, pMember, 0);
			pFile << ";\n";
		}

		// if exception, return
		m_pFunction->WriteReturn(pFile);
		--pFile << "\t}\n";
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMarshaller::%s(%s) returns true\n",
		__func__, pMember->m_Declarators.First()->GetName().c_str());
	return true;
}

/** \brief marshal the return value of the function
 *  \param pFile the file to write to
 *  \param pMember the member to marshal
 *  \return true if this was an special parameter
 */
bool CBEMarshaller::MarshalReturn(CBEFile& pFile, CBETypedDeclarator *pMember)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMarshaller::%s(%s) called\n",
		__func__, pMember->m_Declarators.First()->GetName().c_str());

	CBENameFactory *pNF = CBENameFactory::Instance();
	// check if member is return variable
	string sName = pNF->GetReturnVariable();
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s try to find %s in param\n", __func__, sName.c_str());
	if (!pMember->m_Declarators.Find(sName))
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMarshaller::%s(%s) returns false\n",
			__func__, pMember->m_Declarators.First()->GetName().c_str());
		return false;
	}

	// the return value is not a parameter, but a local variable,
	// so we have to find that variable instead of the parameter
	CBETypedDeclarator *pParameter = m_pFunction->m_LocalVariables.Find(sName);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s Found return as local var in %s at %p\n", __func__,
		m_pFunction->GetName().c_str(), pParameter);
	// now, the local return variable can be of type void, for marshal
	// functions at the server side: we do indeed have a return variable as
	// parameter:
	if ((dynamic_cast<CBEMarshalFunction*>(m_pFunction) ||
			dynamic_cast<CBEMarshalExceptionFunction*>(m_pFunction) ||
			dynamic_cast<CBEReplyFunction*>(m_pFunction) ||
			dynamic_cast<CBESndFunction*>(m_pFunction)) &&
		(!pParameter || pParameter->GetType()->IsVoid()))
	{
		pParameter = m_pFunction->FindParameter(sName);
	}
	// if the assert traps here, then there is no local variable for
	// the return member.
	if (!pParameter)
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s No local return variable available in func %s\n",
			__func__, m_pFunction->GetName().c_str());
	assert(pParameter);

	CDeclStack stack;
	stack.push_back(pMember->m_Declarators.First());

	// try to marshal strings
	if (MarshalString(pFile, pParameter, &stack))
		return true;
	// try to marshal arrays (if it _does_ marshal an array it returns true)
	if (MarshalArray(pFile, pParameter, &stack))
		return true;
	// try to marshal union
	// before struct, since IDL union is derived from struct
	if (MarshalUnion(pFile, pParameter, &stack))
		return true;
	// try to marshal  struct
	if (MarshalStruct(pFile, pParameter, &stack))
		return true;

	// check type (transmit)
	// assignment
	WriteAssignment(pFile, pParameter, &stack);

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMarshaller::%s(%s) returns true\n",
		__func__, pMember->m_Declarators.First()->GetName().c_str());
	return true;
}

/** \brief writes the complete assigning, respecting the stack
 *  \param pFile the file to write to
 *  \param pParameter the parameter
 *  \param pStack the declarator stack
 */
void CBEMarshaller::WriteAssignment(CBEFile& pFile, CBETypedDeclarator *pParameter, CDeclStack* pStack)
{
	CBEMsgBuffer *pMsgBuffer = m_pFunction->GetMessageBuffer();
	CBETypedDeclarator *pMember = FindMarshalMember(pStack);
	if (!pMember)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s: could not find member for parameter %s\n",
			__func__, pParameter->m_Declarators.First()->GetName().c_str());
		DUMP_STACK(iter, pStack, __func__);
	}
	assert(pMember);
	// try to find respective member and assign
	pFile << "\t";
	if (m_bMarshal)
	{
		WriteMember(pFile, m_pFunction->GetSendDirection(), pMsgBuffer, pMember, pStack);
		pFile << " = ";
		// if type of member and parameter are different, cast to member type
		WriteParameter(pFile, pParameter, pStack, false);
	}
	else
	{
		WriteParameter(pFile, pParameter, pStack, false);
		pFile << " = ";
		WriteMember(pFile, m_pFunction->GetReceiveDirection(), pMsgBuffer, pMember, pStack);
	}
	pFile << ";\n";
}

/** \brief writes the access to a specific member in the message buffer
 *  \param pFile the file to write to
 *  \param nType the type of the message buffer type
 *  \param pMsgBuffer the message buffer containing the members
 *  \param pMember the member to access
 *  \param pStack set if a stack is to be used
 */
void CBEMarshaller::WriteMember(CBEFile& pFile, CMsgStructType nType, CBEMsgBuffer *pMsgBuffer,
	CBETypedDeclarator *pMember, CDeclStack* pStack)
{
	assert(pMember);
	assert(pMsgBuffer);

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEMarshaller::WriteMember(%s, %d, %p, %s, %p) called\n",
		pFile.GetFileName().c_str(), (int)nType, pMsgBuffer,
		pMember->m_Declarators.First()->GetName().c_str(), pStack);

	bool bMine = false;
	if (!pStack)
	{
		pStack = new CDeclStack();
		pStack->push_back(pMember->m_Declarators.First());
		bMine = true;
	}

	// variable sized members of constructed types can have an "alias" member
	// in the message buffer. We test for these variable sized members and try
	// to find the alias. If one exists, we have to construct a new stack.
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetLocalVariableName(pStack);
	CBETypedDeclarator *pAlias = pMsgBuffer->FindMember(sName, m_pFunction,
		nType);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s: Alias for %s at %p\n", __func__, sName.c_str(),
		pAlias);
	if (pAlias)
	{
		CDeclStack vStack;
		vStack.push_back(pAlias->m_Declarators.First());
		pMsgBuffer->WriteAccess(pFile, m_pFunction, nType, &vStack);
	}
	else
		pMsgBuffer->WriteAccess(pFile, m_pFunction, nType, pStack);

	if (bMine)
		delete pStack;

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEMarshaller::WriteMember finished\n");
}

/** \brief writes the access to a parameter
 *  \param pFile the file to write to
 *  \param pParameter the parameter to access
 *  \param pStack the declarator stack
 *  \param bPointer true if parameter should be used as pointer
 *
 * This method has to write the access to a single parameter.
 *
 * If parameter has transmit_as attribute, we have to cast the parameter to
 * this type. If the parameter's type is a constructed type, we have to cast
 * pointers.
 */
void CBEMarshaller::WriteParameter(CBEFile& pFile, CBETypedDeclarator *pParameter, CDeclStack* pStack,
	bool bPointer)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s (%s, %p, %s) called\n", __func__,
		pParameter ? pParameter->m_Declarators.First()->GetName().c_str() : "(none)",
		pStack, bPointer ? "true" : "false");

	// get the type
	CBEType *pType = pParameter->GetType();
	CBEAttribute *pAttr;
	CBEType *pCastType = pType;
	bool bCast = false;
	if ((pAttr = pParameter->m_Attributes.Find(ATTR_TRANSMIT_AS)) != 0)
	{
		pCastType = pAttr->GetAttrType();
		bCast = !pType->IsOfType(pCastType->GetFEType());
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMarshaller::%s cast? %s\n",
		__func__, bCast ? "yes" : "no");

	// get declarator
	CBEDeclarator *pDecl = pParameter->m_Declarators.First();
	int nStars = pDecl->GetStars();

	// if no constructed type, then cast the value
	if (!pType->IsConstructedType() &&
		!pType->IsPointerType() &&
		!pDecl->IsArray() &&
		bCast)
		pCastType->WriteCast(pFile, false);

	// if constructed type, then cast pointer
	if (pType->IsConstructedType() && bCast)
	{
		// dereference casted value
		if (!bPointer)
			pFile << "*";
		// cast pointer
		pCastType->WriteCast(pFile, true);
		// write further dereferencing with one star less
		nStars--;
		// put parenthesis around variable
		pFile << "(";
	}
	else if (bPointer)
		nStars--;

	// if type is pointer type, increase stars
	if (pType->IsPointerType())
		nStars++;

	// do NOT dereference here. This is done in
	// CDeclaratorStackLocation::Write

	// if stars is negative then we have to create reference
	if ((nStars < 0) && !pDecl->IsArray()) // can be at most -1
		pFile << "&(";
	// print name
	bool bReference = /* (nStars > (bHasRef ? 1 : 0)) || */
		(pType->IsConstructedType() && bCast) || bPointer;
	CDeclaratorStackLocation::Write(pFile, pStack, bReference);
	// we could have referenced a fixed size array -> nStars is < 0 and the
	// declarator is an array
	if ((nStars < 0) && !pDecl->IsArray())
	{
		// 	if (pDecl->IsArray())
		// 	    pFile << "[0]";
		// close opened parenthesis
		pFile << ")";
	}
	if (pType->IsConstructedType() && bCast)
		pFile << ")";
}

/** \brief marshal a member, which is an string
 *  \param pFile the file to write to
 *  \param pParameter the parameter to marshal
 *  \param pStack the declarator stack
 *  \return true if it marshalled something
 *
 * A string might have a size_is attribute. It can either be set by the user
 * by specifying it in the IDL or it is set by us when we added the size
 * member to the struct. We only set the SIZE_IS attribute, so testing this
 * for our size member is sufficient.
 */
bool CBEMarshaller::MarshalString(CBEFile& pFile, CBETypedDeclarator *pParameter, CDeclStack* pStack)
{
	if (!pParameter->IsString())
		return false;

	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s(%s) called\n", __func__,
		pParameter->m_Declarators.First()->GetName().c_str());

	// Because the size has be calculated, we write the size first and then do
	// a simple memcpy

	// get struct
	CMsgStructType nType(CMsgStructType::Generic);
	if (m_bMarshal)
		nType = m_pFunction->GetSendDirection();
	else
		nType = m_pFunction->GetReceiveDirection();
	assert(CMsgStructType::In == nType || CMsgStructType::Out == nType);

	CBEStructType *pStruct = GetStruct(m_pFunction, nType);
	assert(pStruct);

	// get name of size variable added by us
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sSize = pNF->GetLocalSizeVariableName(pStack);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s determined size var as %s\n", __func__,
		sSize.c_str());
	// get SIZE_IS attribute
	CBEAttribute *pSizeAttr = pParameter->m_Attributes.Find(ATTR_SIZE_IS);
	bool bOurSizeAttr = false;
	if (pSizeAttr)
	{
		CBEDeclarator *pDecl = pSizeAttr->m_Parameters.First();
		bOurSizeAttr = (pDecl->GetName() == sSize);
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s param has size attr %s%s\n",
			__func__, pDecl->GetName().c_str(),
			bOurSizeAttr ? " => our size attr" : "");
	}

	// only marshal size member if marshalling, because when unmarshalling
	// the struct at server side its members are all unmarshalled, which
	// includes the size member. So there is no need to unmarshal it
	// explicetly here.  However we need to marshal it explicetly, because not
	// all members might get marshalled.
	bool bComponentSide = m_pFunction->IsComponentSide();
	if (((!pSizeAttr && !pParameter->m_Attributes.Find(ATTR_LENGTH_IS)) ||
			bOurSizeAttr) &&
		!bComponentSide)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s have our size and no length and at client\n",
			__func__);
		// find size member
		CBETypedDeclarator *pSizeMember = pStruct->m_Members.Find(sSize);
		assert(pSizeMember);
		CBETypedDeclarator *pSizeVariable =
			m_pFunction->m_LocalVariables.Find(sSize);
		assert(pSizeVariable);
		CBEDeclarator *pSizeDecl = pSizeVariable->m_Declarators.First();

		// make sure size parameter will fit into max-is
		// if marshaling do this before marshaling, if unmarshaling after
		if (m_bMarshal && pParameter->m_Attributes.Find(ATTR_MAX_IS))
		{
			CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
				"CBEMarshaller::%s writing maxis check (marshal)\n",
				__func__);

			pFile << "\tif (DICE_EXPECT_FALSE(";
			pSizeDecl->WriteDeclaration(pFile);
			pFile << " > ";
			pParameter->WriteGetMaxSize(pFile, pStack, m_pFunction);
			// max size
			pFile << "))\n";
			++pFile << "\t";
			pSizeDecl->WriteDeclaration(pFile);
			pFile << " = ";
			pParameter->WriteGetMaxSize(pFile, pStack, m_pFunction);
			pFile << ";\n";
			--pFile;
		}

		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s calling marshal for size member %s.\n",
			__func__, pSizeDecl->GetName().c_str());
		// marshal size member first
		// create a stack with the local variable
		// FIXME can we use size member?
		CDeclStack vStack;
		vStack.push_back(pSizeDecl);
		MarshalParameterIntern(pFile, pSizeMember, &vStack);

		if (!m_bMarshal && !bOurSizeAttr &&
			pParameter->m_Attributes.Find(ATTR_MAX_IS))
		{
			CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
				"CBEMarshaller::%s writing maxis check (unmarshal)\n",
				__func__);

			pFile << "\tif (DICE_EXPECT_FALSE(";
			pSizeDecl->WriteDeclaration(pFile);
			pFile << " > ";
			pParameter->WriteGetMaxSize(pFile, pStack, m_pFunction);
			// max size
			pFile << "))\n";
			++pFile << "\t";
			pSizeDecl->WriteDeclaration(pFile);
			pFile << " = ";
			pParameter->WriteGetMaxSize(pFile, pStack, m_pFunction);
			pFile << ";\n";
			--pFile;
		}
	}

	// now memcpy string
	CBEMsgBuffer *pMsgBuffer = m_pFunction->GetMessageBuffer();
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s message buffer at %p\n", __func__, pMsgBuffer);

	CBETypedDeclarator *pMember = FindMarshalMember(pStack);
	if (m_bMarshal)
	{
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s writing memcpy (marshal)\n",
			__func__);

		pFile << "\tif (DICE_EXPECT_TRUE(";
		pMember->WriteGetSize(pFile, pStack, m_pFunction);
		pFile << " > 0))\n";
		++pFile << "\t_dice_memcpy (";
		WriteMember(pFile, nType, pMsgBuffer, pMember, pStack);
		pFile << ", ";
		WriteParameter(pFile, pParameter, pStack, true);
		pFile << ", ";
		pMember->WriteGetSize(pFile, pStack, m_pFunction);
		pFile << ");\n";
		--pFile;
	}
	else
	{
		// Ensure that size is no larger than maximum size if size is
		// declarator from size_is attribute.
		//
		// if (size > max) size = max;
		if ((pMember->m_Attributes.Find(ATTR_SIZE_IS) ||
				pMember->m_Attributes.Find(ATTR_LENGTH_IS)) &&
			pMember->m_Attributes.Find(ATTR_MAX_IS))
		{
			CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
				"CBEMarshaller::%s writing maxis check (unmarshal + size)\n",
				__func__);

			pFile << "\tif (DICE_EXPECT_FALSE(";
			pMember->WriteGetSize(pFile, pStack, m_pFunction);
			pFile << " > ";
			pMember->WriteGetMaxSize(pFile, pStack, m_pFunction);
			// max size
			pFile << "))\n";
			++pFile << "\t";
			pMember->WriteGetSize(pFile, pStack, m_pFunction);
			pFile << " = ";
			pMember->WriteGetMaxSize(pFile, pStack, m_pFunction);
			pFile << ";\n";
			--pFile;
		}

		// at server side directly reference into message buffer (requires
		// 0-termination)
		//
		// if (size > 0)
		// {
		//   string[size-1] = 0;
		//   param = string;
		// }
		// else
		//   param = 0;
		//
		// at client side allocate memory if necessary and copy content into
		// param
		//
		// if (size > 0)
		// {
		//   param = malloc(size);
		//   memcpy (param, buffer, size);
		// }
		// else
		//   param = 0;
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s writing mandatory zero termination\n",
			__func__);

		pFile << "\tif (DICE_EXPECT_TRUE(";
		pMember->WriteGetSize(pFile, pStack, m_pFunction);
		pFile << " > 0))\n";

		bool bPrealloc = false;
		if (pParameter->m_Attributes.Find(ATTR_PREALLOC_CLIENT) &&
			pFile.IsOfFileType(FILETYPE_CLIENT))
			bPrealloc = true;
		if (pParameter->m_Attributes.Find(ATTR_PREALLOC_SERVER) &&
			pFile.IsOfFileType(FILETYPE_COMPONENT))
			bPrealloc = true;
		pFile << "\t{\n";

		// zero terminate string in buffer
		++pFile << "\t";
		WriteMember(pFile, nType, pMsgBuffer, pMember, pStack);
		pFile << "[";
		pMember->WriteGetSize(pFile, pStack, m_pFunction);
		pFile << "] = 0;\n";

		if (bComponentSide)
		{
			// assing parameter reference into buffer
			pFile << "\t";
			WriteParameter(pFile, pParameter, pStack, true);
			pFile << " = ";
			WriteMember(pFile, nType, pMsgBuffer, pMember, pStack);
			pFile << ";\n";
		}
		else
		{
			if (!bPrealloc)
			{
				// allocate memory for client out string
				pFile << "\t";
				WriteParameter(pFile, pParameter, pStack, true);
				pFile << " = ";
				CBEContext::WriteMalloc(pFile, m_pFunction);
				pFile << "(";
				pMember->WriteGetSize(pFile, pStack, m_pFunction);
				pFile << ");\n";
			}
			// copy string to client parameter
			pFile << "\t_dice_memcpy (";
			WriteParameter(pFile, pParameter, pStack, true);
			pFile << ", ";
			WriteMember(pFile, nType, pMsgBuffer, pMember, pStack);
			pFile << ", ";
			pMember->WriteGetSize(pFile, pStack, m_pFunction);
			pFile << ");\n";
		}

		--pFile << "\t}\n";
		pFile << "\telse\n";

		++pFile << "\t";
		WriteParameter(pFile, pParameter, pStack, true);
		pFile << " = 0;\n";

		--pFile;
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s(%s) returns true\n", __func__,
		pParameter->m_Declarators.First()->GetName().c_str());
	return true;
}

/** \brief marshal a member, which is an array
 *  \param pFile the file to write to
 *  \param pParameter the parameter to marshal
 *  \param pStack the declarator stack
 *  \return true if it marshalled something
 *
 * The marshal function first checks if alias types have array dimensions. If
 * so, these dimensions are added to a temporary vector (to be removed later).
 * The stored array bounds are then added to the declarator.
 *
 * Then we check if the declarator has array dimensions (includes the ones
 * from above). If so an internal marshal-array function is called. After that
 * functions returns the temporary array dimensions are removed and the
 * function returns.
 *
 * Then we check if the declarator has stars, which might indicate unbound
 * array dimensions AND size or length attribute set. If so, the internal
 * array marshalling function is called.
 */
bool CBEMarshaller::MarshalArray(CBEFile& pFile, CBETypedDeclarator *pParameter, CDeclStack* pStack)
{
	// get array dimensions from user defined types
	vector<CBEExpression*> vBounds;
	CBEType *pType = pParameter->GetType();
	CBEAttribute *pAttr;
	if ((pAttr = pParameter->m_Attributes.Find(ATTR_TRANSMIT_AS)) != 0)
		pType = pAttr->GetAttrType();

	vector<CBEExpression*>::iterator iter;
	CBEUserDefinedType *pUserType = dynamic_cast<CBEUserDefinedType*>(pType);
	while (pUserType)
	{
		// we get the original type the old fashioned way, because we need the
		// typedef
		CBETypedef *pTypedef = FindTypedef(pUserType->GetName());
		assert(pTypedef);
		pType = pTypedef->GetType();
		if ((pAttr = pTypedef->m_Attributes.Find(ATTR_TRANSMIT_AS)) != 0)
			pType = pAttr->GetAttrType();
		CBEDeclarator *pAlias = pTypedef->m_Declarators.First();
		if (pAlias && pAlias->IsArray())
		{
			// copy elements (points to bounds) from pAlias to vBounds
			for (iter = pAlias->m_Bounds.begin();
				iter != pAlias->m_Bounds.end();
				iter++)
				vBounds.push_back(*iter);
		}

		pUserType = dynamic_cast<CBEUserDefinedType*>(pType);
	}

	CBEDeclarator *pDeclarator = pParameter->m_Declarators.First();
	for (iter = vBounds.begin(); iter != vBounds.end(); iter++)
		pDeclarator->AddArrayBound(*iter);

	// test if declarator is array. If the type is array then its dimensions
	// have been added to the declarator by the test above, so no need to test
	// that again.
	if (pDeclarator->IsArray())
	{
		MarshalArrayIntern(pFile, pParameter, pType, pStack);

		for (iter = vBounds.begin(); iter != vBounds.end(); iter++)
			pDeclarator->RemoveArrayBound(*iter);

		return true;
	}

	// check if variable sized array without "proper" array bounds
	if ((pDeclarator->GetStars() > 0) &&
		(pParameter->m_Attributes.Find(ATTR_SIZE_IS) ||
		 pParameter->m_Attributes.Find(ATTR_LENGTH_IS)))
	{
		MarshalArrayIntern(pFile, pParameter, pType, pStack);

		return true;
	}

	return false;
}

/** \brief internal marshalling function for arrays
 *  \param pFile the file to write to
 *  \param pParameter the parameter to marshal
 *  \param pType the type to marshal with
 *  \param pStack the currently active declarator stack
 */
void CBEMarshaller::MarshalArrayIntern(CBEFile& pFile, CBETypedDeclarator *pParameter, CBEType *pType,
	CDeclStack* pStack)
{
	CBEDeclarator *pDeclarator = pParameter->m_Declarators.First();

	// FIXME multidimensional arrays
	// FIXME integer array bounds vs. constants or variables

	bool bIsVarSized = (pDeclarator->GetSize() < 0) ||
		pParameter->m_Attributes.Find(ATTR_SIZE_IS) ||
		pParameter->m_Attributes.Find(ATTR_LENGTH_IS);
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEMarshaller::%s for %s is %svariable sized\n", __func__,
		pDeclarator->GetName().c_str(), bIsVarSized ? "" : "not ");
	// test if we can reference directly into the message buffer when
	// unmarshalling. The following preconditions have to be met:
	// - server side
	// - the array is variable sized
	// - the array has no array dimension which lets us conclude that the
	//   parameter has only stars to mark its variable size
	// - unmarshalling
	bool bRefMsgBuf = m_pFunction->IsComponentSide() && bIsVarSized &&
		(pDeclarator->GetArrayDimensionCount() == 0) && !m_bMarshal;
	if (bRefMsgBuf)
	{
		MarshalArrayInternRef(pFile, pParameter, pStack);
		return;
	}

	// make sure that size or length members are unmarshalled before array
	// itself
	if ((pParameter->m_Attributes.Find(ATTR_SIZE_IS) ||
			pParameter->m_Attributes.Find(ATTR_LENGTH_IS)) &&
		!m_bMarshal)
	{
		// get declarator
		CBEAttribute *pAttr = pParameter->m_Attributes.Find(ATTR_SIZE_IS);
		if (!pAttr)
			pAttr = pParameter->m_Attributes.Find(ATTR_LENGTH_IS);
		assert(pAttr);
		CBEDeclarator *pSizeDecl = 0;
		if (pAttr->IsOfType(ATTR_CLASS_IS))
			pSizeDecl = pAttr->m_Parameters.First();

		if (pSizeDecl)
		{
			// check if decl is before parameter in struct (then it will be
			// unmarshalled before array)
			CBEMsgBuffer *pMsgBuffer = m_pFunction->GetMessageBuffer();
			assert(pMsgBuffer);

			if (pMsgBuffer->IsEarlier(m_pFunction, m_pFunction->GetReceiveDirection(),
					pParameter->m_Declarators.First()->GetName(),
					pSizeDecl->GetName()))
			{
				// unmarshal size first
				CBETypedDeclarator *pSizeParam = m_pFunction->m_Parameters.Find(pSizeDecl->GetName());
				CDeclStack vStack;
				vStack.push_back(pSizeParam->m_Declarators.First());
				MarshalParameterIntern(pFile, pSizeParam, &vStack);
			}
		}
	}

	// make sure that the size value does not exceed the maximum
	if (pParameter->m_Attributes.Find(ATTR_SIZE_IS) ||
		pParameter->m_Attributes.Find(ATTR_LENGTH_IS))
	{
		if (pParameter->m_Attributes.Find(ATTR_MAX_IS))
		{
			pFile << "\tif (";
			pParameter->WriteGetSize(pFile, pStack, m_pFunction);
			pFile << " > ";
			pParameter->WriteGetMaxSize(pFile, pStack, m_pFunction);
			pFile << ")\n";
			++pFile << "\t";
			pParameter->WriteGetSize(pFile, pStack, m_pFunction);
			pFile << " = ";
			pParameter->WriteGetMaxSize(pFile, pStack, m_pFunction);
			pFile << ";\n";
			--pFile;
		}
		else
		{
			int nMax = 0;
			if (pDeclarator->GetArrayDimensionCount() > 0)
				nMax = pDeclarator->m_Bounds.First()->GetIntValue();
			if (nMax)
			{
				pFile << "\tif (";
				pParameter->WriteGetSize(pFile, pStack, m_pFunction);
				pFile << " > " << nMax << ")\n";
				++pFile << "\t";
				pParameter->WriteGetSize(pFile, pStack, m_pFunction);
				pFile << " = " << nMax << ";\n";
				--pFile;
			}
		}
	}

	// test if we have to allocate memory for the array. Following
	// preconditions apply:
	// - unmarshalling
	// - the array has no array dimensions which implies we have to allocate
	//   the necessary amount of memory for the array
	// - no [prealloc] attribute
	// - no direct reference into the message buffer
	// - is variable sized
	bool bPrealloc = false;
	if (pParameter->m_Attributes.Find(ATTR_PREALLOC_CLIENT) &&
		pFile.IsOfFileType(FILETYPE_CLIENT))
		bPrealloc = true;
	if (pParameter->m_Attributes.Find(ATTR_PREALLOC_SERVER) &&
		pFile.IsOfFileType(FILETYPE_COMPONENT))
		bPrealloc = true;
	bool bNeedMalloc = bIsVarSized && !m_bMarshal && !bRefMsgBuf &&
		(pDeclarator->GetArrayDimensionCount() == 0) && !bPrealloc;
	if (bNeedMalloc)
	{
		pFile << "\t";
		WriteParameter(pFile, pParameter, pStack, true);
		pFile << " = ";
		pType->WriteCast(pFile, true);
		CBEContext::WriteMalloc(pFile, m_pFunction);
		pFile << "(";
		pParameter->WriteGetSize(pFile, pStack, m_pFunction);
		if (pType->GetSize() > 1)
		{
			pFile << "*sizeof";
			pType->WriteCast(pFile, false);
		}
		pFile << ");\n";
	}

	CBEMsgBuffer *pMsgBuffer = m_pFunction->GetMessageBuffer();
	CBETypedDeclarator *pMember = FindMarshalMember(pStack);

	pFile << "\t_dice_memcpy (";
	if (m_bMarshal)
	{
		WriteMember(pFile, m_pFunction->GetSendDirection(), pMsgBuffer, pMember, pStack);
		pFile << ", ";
		WriteParameter(pFile, pParameter, pStack, true);
	}
	else
	{
		WriteParameter(pFile, pParameter, pStack, true);
		pFile << ", ";
		WriteMember(pFile, m_pFunction->GetReceiveDirection(), pMsgBuffer, pMember, pStack);
	}
	pFile << ", ";
	// only call WriteGetSize if this is variable sized.
	// A variable sized array has to have the size_is or length_is attribute
	// set
	if (bIsVarSized)
	{
		pParameter->WriteGetSize(pFile, pStack, m_pFunction);
		// we make the cast here, because WriteGetSize will not write the size
		// in bytes, also check the size of the direct type of the parameter,
		// because something like:
		// typedef char foo[20];
		// [size_if(len)] foo param[10];
		// will rely on len as parameter to memcpy and pType is the 'char'
		// type. Thus check for parameter type.
		if (pParameter->GetType() != pType &&
			pParameter->GetType()->GetSize() > 1)
		{
			pFile << "*sizeof";
			pParameter->GetType()->WriteCast(pFile, false);
		} else if (pType->GetSize() > 1)
		{
			pFile << "*sizeof";
			pType->WriteCast(pFile, false);
		}
	}
	else
	{
		// const sized array: determine size by iterating over array bounds
		// and calculating size
		int nBound = 1;
		vector<CBEExpression*>::iterator iter;
		for (iter = pDeclarator->m_Bounds.begin();
			iter != pDeclarator->m_Bounds.end();
			iter++)
		{
			nBound *= (*iter)->GetIntValue();
		}
		// now write the size of the array multiplied with the size of the
		// type
		if (nBound > 1)
			pFile << nBound;
		if ((nBound > 1) && (pType->GetSize() > 1))
			pFile << "*";
		if (pType->GetSize() > 1)
		{
			pFile << "sizeof";
			pType->WriteCast(pFile, false);
		}
	}
	pFile << ");\n";
}

/** \brief unmarshals an array by referencing directly into the message buffer
 *  \param pFile the file to write to
 *  \param pParameter the parameter to unmarshal
 *  \param pStack the declarator stack used so far for the message buffer
 *         memeber
 *
 * This method is only invoked if:
 * - server side
 * - the array is variable sized
 * - the array has no array dimension which lets us conclude that the
 *   parameter has only stars to mark its variable size
 * - unmarshalling
 */
void CBEMarshaller::MarshalArrayInternRef(CBEFile& pFile, CBETypedDeclarator *pParameter,
	CDeclStack* pStack)
{
	pFile << "\t";
	WriteParameter(pFile, pParameter, pStack, true);
	pFile << " = ";
	CBEMsgBuffer *pMsgBuffer = m_pFunction->GetMessageBuffer();
	CBETypedDeclarator *pMember = FindMarshalMember(pStack);
	WriteMember(pFile, m_pFunction->GetReceiveDirection(), pMsgBuffer, pMember, pStack);
	pFile << ";\n";
}

/** \brief marshal struct members (or parameters)
 *  \param pFile the file to write to
 *  \param pParameter the parameter to marshal from/to
 *  \param pStack the declarator stack
 *
 * Marshaling a struct is (mostly) a simple assignment. Exceptions include if
 * the struct includes pointers or other variable sized members and if the
 * parameter has a transmit_as attribute. Then the struct has to be cast to
 * that type. This can only be done if the parameter is a pointer. Then we
 * cast it to a pointer of the target type and then dereference it. This
 * casting is done in the WriteParameter method.
 */
bool CBEMarshaller::MarshalStruct(CBEFile& pFile, CBETypedDeclarator *pParameter, CDeclStack* pStack)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s(%s) called\n", __func__,
		pParameter->m_Declarators.First()->GetName().c_str());

	CBEType *pType = pParameter->GetType();
	while (pType->IsOfType(TYPE_USER_DEFINED))
		pType = static_cast<CBEUserDefinedType*>(pType)->GetRealType();
	CBEStructType *pStruct = dynamic_cast<CBEStructType*>(pType);
	if (!pStruct)
	{
		CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s returns (no struct)\n", __func__);
		return false; // *no* struct
	}

	// first write assignment of struct
	WriteAssignment(pFile, pParameter, pStack);

	// iterate members and look for variable sized members
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = pStruct->m_Members.begin();
		iter != pStruct->m_Members.end();
		iter++)
	{
		if (!(*iter)->IsVariableSized())
			continue;

		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s marshalling member %s\n",
			__func__, (*iter)->m_Declarators.First()->GetName().c_str());

		// add to declarator stack
		pStack->push_back((*iter)->m_Declarators.First());
		MarshalParameterIntern(pFile, (*iter), pStack);
		pStack->pop_back();
	}

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s(%s) return true\n", __func__,
		pParameter->m_Declarators.First()->GetName().c_str());
	return true;
}

/** \brief marshal union members (or parameters)
 *  \param pFile the file to write to
 *  \param pParameter the parameter to marshal from/to
 *  \param pStack the declarator stack
 *
 * Marshaling a union is (mostly) a simple assignment. Exceptions include if
 * the parameter has a transmit_as attribute. Then the union has to be cast to
 * that type. Another exception is if this is an IDL style union, then the
 * member to marshal depends on the discriminator.
 *
 * We can get a pointer to the union parameter and cast this pointer to the
 * target type and then dereference it.  This is done in the WriteParameter
 * method/
 */
bool CBEMarshaller::MarshalUnion(CBEFile& pFile, CBETypedDeclarator *pParameter, CDeclStack* pStack)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "CBEMarshaller::%s(%s) called\n", __func__,
		pParameter->m_Declarators.First()->GetName().c_str());

	CBEType *pType = pParameter->GetType();
	while (pType->IsOfType(TYPE_USER_DEFINED))
		pType = static_cast<CBEUserDefinedType*>(pType)->GetRealType();
	CBEIDLUnionType *pUnion = dynamic_cast<CBEIDLUnionType*>(pType);
	if (!pUnion)
		return false; // *no* IDL union

	// marshal switch var
	// switch with switch var
	// iterate members and marshal each (don't forget the case statement)

	CBETypedDeclarator *pSwitchVar = pUnion->GetSwitchVariable();
	if (!pSwitchVar)
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s no switch var for param %s\n", __func__,
			pParameter->m_Declarators.First()->GetName().c_str());
	assert (pSwitchVar);
	pStack->push_back(pSwitchVar->m_Declarators.First());
	MarshalParameterIntern(pFile, pSwitchVar, pStack);

	// write switch statement
	pFile << "\tswitch (";
	CDeclaratorStackLocation::Write(pFile, pStack, false);
	pStack->pop_back();
	pFile << ")\n";
	pFile << "\t{\n";

	CBETypedDeclarator *pUnionVar = pUnion->GetUnionVariable();
	assert (pUnionVar);
	pStack->push_back(pUnionVar->m_Declarators.First());
	pStack->back().SetIndex(-3);

	CBEUnionType *pUnionType = dynamic_cast<CBEUnionType*>(
		pUnionVar->GetType());
	assert (pUnionType);
	vector<CBEUnionCase*>::iterator iterC;
	for (iterC = pUnionType->m_UnionCases.begin();
		iterC != pUnionType->m_UnionCases.end();
		iterC++)
	{
		if ((*iterC)->IsDefault())
			pFile << "\tdefault:\n";
		else
		{
			vector<CBEExpression*>::iterator iterL;
			for (iterL = (*iterC)->m_Labels.begin();
				iterL != (*iterC)->m_Labels.end();
				iterL++)
			{
				pFile << "\tcase ";
				(*iterL)->Write(pFile);
				pFile << ":\n";
			}
		}
		++pFile;

		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s marshalling case %s\n",
			__func__, (*iterC)->m_Declarators.First()->GetName().c_str());
		pStack->push_back((*iterC)->m_Declarators.First());
		MarshalParameterIntern(pFile, *iterC, pStack);
		pStack->pop_back();

		pFile << "\tbreak;\n";
		--pFile;
	}

	// remove the union name from the stack
	pStack->pop_back();

	pFile << "\t}\n";

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s(%s) returns true\n", __func__,
		pParameter->m_Declarators.First()->GetName().c_str());
	return true;
}

/** \brief tries to find a member to use for marshalling for a specific
 *         parameter stack
 *  \param pStack the parameter (or declarator) stack
 *  \return a reference to the respective member if found
 */
CBETypedDeclarator*
CBEMarshaller::FindMarshalMember(CDeclStack* pStack)
{
	CBETypedDeclarator *pMember = 0;

	// get struct
	CMsgStructType nType(CMsgStructType::Generic);
	if (m_bMarshal)
		nType = m_pFunction->GetSendDirection();
	else
		nType = m_pFunction->GetReceiveDirection();
	assert(CMsgStructType::In == nType || CMsgStructType::Out == nType || CMsgStructType::Exc == nType);
	CBEStructType *pStruct = GetStruct(m_pFunction, nType);
	assert(pStruct);

	// there should be at least one member
	assert(pStack && !pStack->empty());
	CDeclStack::iterator iter = pStack->begin();
	// get the member
	string sName = iter->pDeclarator->GetName();
	pMember = pStruct->m_Members.Find(sName);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s member in struct is %s (@ %p)\n", __func__,
		sName.c_str(), pMember);

	for (iter += 1; iter != pStack->end(); iter++)
	{
		// when we are here, there is at least one more element in stack,
		// which must then be an element of the current member. Thus check for
		// known constructed types
		CBEType *pType = pMember->GetType();
		while (dynamic_cast<CBEUserDefinedType*>(pType))
			pType = ((CBEUserDefinedType*)pType)->GetRealType();
		CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
			"CBEMarshaller::%s type of member %s is %d\n", __func__,
			pMember->m_Declarators.First()->GetName().c_str(),
			pType->GetFEType());

		sName = iter->pDeclarator->GetName();

		if (dynamic_cast<CBEStructType*>(pType))
		{
			CBEStructType *pStructType = static_cast<CBEStructType*>(pType);
			pMember = pStructType->m_Members.Find(sName);
			// oops member not found!
			if (!pMember)
				return pMember;
			continue;
		}
		if (dynamic_cast<CBEUnionType*>(pType))
		{
			CBEUnionType *pUnion = static_cast<CBEUnionType*>(pType);
			pMember = pUnion->m_UnionCases.Find(sName);
			// oops, no case contained the next declarator?!
			if (!pMember)
				return pMember;
			continue;
		}
	}

	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEMarshaller::%s returns member %s\n", __func__,
		pMember->m_Declarators.First()->GetName().c_str());
	return pMember;
}

