/**
 *  \file   dice/src/be/BEWaitAnyFunction.cpp
 *  \brief  contains the implementation of the class CBEWaitAnyFunction
 *
 *  \date   01/21/2002
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "BEWaitAnyFunction.h"
#include "BEContext.h"
#include "BEFile.h"
#include "BEOpcodeType.h"
#include "BEMsgBuffer.h"
#include "BEDeclarator.h"
#include "BEComponent.h"
#include "BEImplementationFile.h"
#include "BEHeaderFile.h"
#include "BEUserDefinedType.h"
#include "BENameFactory.h"
#include "BEClassFactory.h"
#include "Trace.h"
#include "BEAttribute.h"
#include "Compiler.h"
#include "Error.h"
#include "TypeSpec-Type.h"
#include "fe/FEInterface.h"
#include <cassert>

CBEWaitAnyFunction::CBEWaitAnyFunction(bool bOpenWait, bool bReply)
    : CBEInterfaceFunction(bOpenWait ?
	(bReply ? FUNCTION_REPLY_WAIT : FUNCTION_WAIT_ANY) :
	(bReply ? FUNCTION_REPLY_RECV : FUNCTION_RECV_ANY))
{
	m_bOpenWait = bOpenWait;
	m_bReply = bReply;
	/* reply is only allowed with open wait */
	if (bReply)
		assert(bOpenWait);
}

/** \brief destructor of target class */
CBEWaitAnyFunction::~CBEWaitAnyFunction()
{ }

/** \brief creates the wait-any function for the given interface
 *  \param pFEInterface the respective front-end interface
 *  \return true if successful
 *
 * A function which waits for any message from any sender, does return the
 * opcode of the received message and has as a parameter a reference to the
 * message buffer.
 */
void
CBEWaitAnyFunction::CreateBackEnd(CFEInterface * pFEInterface, bool bComponentSide)
{
	FUNCTION_TYPE nFunctionType = FUNCTION_NONE;
	if (m_bOpenWait)
	{
		if (m_bReply)
			nFunctionType = FUNCTION_REPLY_WAIT;
		else
			nFunctionType = FUNCTION_WAIT_ANY;
	}
	else
		nFunctionType = FUNCTION_RECV_ANY;
	// set target file name
	SetTargetFileName(pFEInterface);
	// name of the function
	SetComponentSide(bComponentSide);
	SetFunctionName(pFEInterface, nFunctionType);

	CBEInterfaceFunction::CreateBackEnd(pFEInterface, bComponentSide);
	// set source line number to last number of interface
	m_sourceLoc.setBeginLine(pFEInterface->m_sourceLoc.getEndLine());

	string exc = string(__func__);
	// return type -> set to opcode
	// if return var is parameter do not delete it
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sOpcodeVar = pNF->GetOpcodeVariable();
	SetReturnVar(CBEClassFactory::Instance()->GetNewOpcodeType(), sOpcodeVar);
	// set to zero init it
	CBETypedDeclarator *pReturn = GetReturnVariable();
	pReturn->SetDefaultInitString("0");

	AddMessageBuffer();
	// add marshaller and communication class
	CreateMarshaller();
	CreateCommunication();
	CreateTrace();

	// add parameters
	AddParameters();

	// if any of the interface's functions has the sched_donate attribute set,
	// we should be able to reply with shceduling donation as well. Search for
	// this attribute
	if (m_pClass && m_pClass->HasFunctionWithAttribute(ATTR_SCHED_DONATE))
	{
		CBEClassFactory *pCF = CBEClassFactory::Instance();
		CBEAttribute *pAttr = pCF->GetNewAttribute();
		m_Attributes.Add(pAttr);
		pAttr->CreateBackEnd(ATTR_SCHED_DONATE);
	}
}

/** \brief writes the variable initializations of this function
 *  \param pFile the file to write to
 *
 * This implementation should initialize the message buffer and the pointers of
 * the out variables. We init the message buffer, because we have nothing to
 * send and want the message buffer to be in a defined state.
 */
void
CBEWaitAnyFunction::WriteVariableInitialization(CBEFile& /*pFile*/)
{}

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *
 * This implementation calls the underlying message trasnfer mechanisms
 */
void
CBEWaitAnyFunction::WriteInvocation(CBEFile& pFile)
{
    pFile << "\t/* invoke */\n";
}

/** \brief writes the unmarshalling of the message
 *  \param pFile the file to write to
 *
 * This implementation should unpack the out parameters from the returned
 * message structure.
 *
 * This implementation unmarshals the "return variable", which is the opcode.
 */
void
CBEWaitAnyFunction::WriteUnmarshalling(CBEFile& pFile)
{
	bool bLocalTrace = false;
	if (!m_bTraceOn && m_pTrace)
	{
		m_pTrace->BeforeUnmarshalling(pFile, this);
		m_bTraceOn = bLocalTrace = true;
	}

	WriteMarshalReturn(pFile, false);

	if (bLocalTrace)
	{
		m_pTrace->AfterUnmarshalling(pFile, this);
		m_bTraceOn = false;
	}
}

/** \brief check if this parameter is marshalled
 *  \param pParameter the parameter to marshal
 *  \param bMarshal true if marshaling, false if unmarshaling
 *  \return true if if is marshalled
 *
 * Always return false, except the parameter is the return variable, which
 * should be unmarshaled if wait_any or reply_and_wait.
 */
bool
CBEWaitAnyFunction::DoMarshalParameter(CBETypedDeclarator * pParameter,
    bool bMarshal)
{
	CBETypedDeclarator *pReturn = GetReturnVariable();
	if (!bMarshal &&
		(pParameter == pReturn))
		return true;
	return false;
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return true if should be written
 *
 * A wait-any function is written at the component's side.
 *
 * A receive-any function is only written if the PROGRAM_GENERATE_MESSAGE
 * option is set. Then it is created for the client's as well as the
 * component's side.
 *
 * Because the component side determines the function name, we have to
 * differentiate based on IsComponentSide().
 */
bool CBEWaitAnyFunction::DoWriteFunction(CBEFile* pFile)
{
	if (!IsTargetFile(pFile))
		return false;
	if (m_bOpenWait)
		// this test is true for m_bReply (true or false)
		return pFile->IsOfFileType(FILETYPE_COMPONENT) &&
			IsComponentSide();
	// closed wait
	if (CCompiler::IsOptionSet(PROGRAM_GENERATE_MESSAGE))
	{
		if (pFile->IsOfFileType(FILETYPE_COMPONENT) &&
			IsComponentSide())
			return true;
		if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
			!IsComponentSide())
			return true;
	}
	return false;
}

/** \brief write a single parameter to the target file
 *  \param pFile the file to write to
 *  \param pParameter the parameter to write
 *  \param bUseConst true if param should be const
 *
 * Do not write const Object.
 */
void
CBEWaitAnyFunction::WriteParameter(CBEFile& pFile,
    CBETypedDeclarator * pParameter,
    bool bUseConst)
{
	if (pParameter == GetObject())
		CBEInterfaceFunction::WriteParameter(pFile, pParameter, false);
	else
		CBEInterfaceFunction::WriteParameter(pFile, pParameter, bUseConst);
}

/** \brief writes the definition of the function to the target file
 *  \param pFile the target file to write to
 *
 * If the given file is an implementation file, we write an inline prefix.
 * This allows the target compiler to optimize cross function calls when
 * inlining the unmarshal function into the dispatcher.
 *
 * We only use "inline", because this function might be used in a derived
 * interface's dispatch function. Thus it cannot be static inline. Simple
 * inline allows the target compiler to inline it locally, that is, into the
 * same interface's dispatch function and also provide an implementation for
 * external calls.
 */
void
CBEWaitAnyFunction::WriteFunctionDefinition(CBEFile& pFile)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEWaitAnyFunction::%s(%s) in %s called\n", __func__,
		pFile.GetFileName().c_str(), GetName().c_str());

	if (pFile.IsOfFileType(FILETYPE_COMPONENTIMPLEMENTATION) &&
		!CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
		pFile << "\tinline" << std::endl;

	CBEInterfaceFunction::WriteFunctionDefinition(pFile);
}

/** \brief tries to find a parameter by its type
 *  \param sTypeName the name of the type
 *  \return a reference to the found parameter
 */
CBETypedDeclarator * CBEWaitAnyFunction::FindParameterType(string sTypeName)
{
	CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
	if (pMsgBuffer && pMsgBuffer->HasType(sTypeName))
		return pMsgBuffer;
	return CBEInterfaceFunction::FindParameterType(sTypeName);
}

/** \brief gets the direction for the sending data
 *  \return if at client's side DIRECTION_IN, else DIRECTION_OUT
 *
 * Since this function ignores the send part, this value should be not
 * interesting
 */
CMsgStructType CBEWaitAnyFunction::GetSendDirection()
{
    return IsComponentSide() ? CMsgStructType::Out : CMsgStructType::In;
}

/** \brief gets the direction for the receiving data
 *  \return if at client's side DIRECTION_OUT, else DIRECTION_IN
 */
CMsgStructType CBEWaitAnyFunction::GetReceiveDirection()
{
    return IsComponentSide() ? CMsgStructType::In : CMsgStructType::Out;
}

/** \brief write the access specifier for the marshal function
 *  \param pFile the file to write to
 */
void CBEWaitAnyFunction::WriteAccessSpecifier(CBEHeaderFile& pFile)
{
	if (!CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
		return;

	--pFile << "\tprotected:\n";
	++pFile;
}

