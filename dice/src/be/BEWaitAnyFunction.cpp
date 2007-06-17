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

CBEWaitAnyFunction::CBEWaitAnyFunction(CBEWaitAnyFunction & src)
: CBEInterfaceFunction(src)
{
    m_bOpenWait = src.m_bOpenWait;
    m_bReply = src.m_bReply;
}

/** \brief destructor of target class */
CBEWaitAnyFunction::~CBEWaitAnyFunction()
{

}

/** \brief creates the wait-any function for the given interface
 *  \param pFEInterface the respective front-end interface
 *  \return true if successful
 *
 * A function which waits for any message from any sender, does return the
 * opcode of the received message and has as a parameter a reference to the
 * message buffer.
 */
void
CBEWaitAnyFunction::CreateBackEnd(CFEInterface * pFEInterface)
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
    SetFunctionName(pFEInterface, nFunctionType);

    CBEInterfaceFunction::CreateBackEnd(pFEInterface);
    // set source line number to last number of interface
    SetSourceLine(pFEInterface->GetSourceLineEnd());

    string exc = string(__func__);
    // return type -> set to opcode
    // if return var is parameter do not delete it
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sOpcodeVar = pNF->GetOpcodeVariable();
    if (!SetReturnVar(CCompiler::GetClassFactory()->GetNewOpcodeType(),
	    sOpcodeVar))
    {
	exc += " failed because return var could not be created.";
	throw new error::create_error(exc);
    }
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
	CBEClassFactory *pCF = CCompiler::GetClassFactory();
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
 */
bool
CBEWaitAnyFunction::DoWriteFunction(CBEHeaderFile* pFile)
{
    if (!IsTargetFile(pFile))
	return false;
    if (m_bOpenWait)
	// this test is true for m_bReply (true or false)
	return pFile->IsOfFileType(FILETYPE_COMPONENT);
    else
	return CCompiler::IsOptionSet(PROGRAM_GENERATE_MESSAGE);
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
 */
bool
CBEWaitAnyFunction::DoWriteFunction(CBEImplementationFile* pFile)
{
    if (!IsTargetFile(pFile))
	return false;
    if (m_bOpenWait)
	// this test is true for m_bReply (true or false)
	return pFile->IsOfFileType(FILETYPE_COMPONENT);
    else
	return CCompiler::IsOptionSet(PROGRAM_GENERATE_MESSAGE);
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

/** \brief tries to find a parameter by its type
 *  \param sTypeName the name of the type
 *  \return a reference to the found parameter
 */
CBETypedDeclarator * CBEWaitAnyFunction::FindParameterType(string sTypeName)
{
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    if (pMsgBuffer)
    {
	CBEType *pType = pMsgBuffer->GetType();
	if (dynamic_cast<CBEUserDefinedType*>(pType))
	{
	    if (((CBEUserDefinedType*)pType)->GetName() == sTypeName)
		return pMsgBuffer;
	}
	if (pType->HasTag(sTypeName))
	    return pMsgBuffer;
    }
    return CBEInterfaceFunction::FindParameterType(sTypeName);
}

/** \brief gets the direction for the sending data
 *  \return if at client's side DIRECTION_IN, else DIRECTION_OUT
 *
 * Since this function ignores the send part, this value should be not
 * interesting
 */
DIRECTION_TYPE CBEWaitAnyFunction::GetSendDirection()
{
    return IsComponentSide() ? DIRECTION_OUT : DIRECTION_IN;
}

/** \brief gets the direction for the receiving data
 *  \return if at client's side DIRECTION_OUT, else DIRECTION_IN
 */
DIRECTION_TYPE CBEWaitAnyFunction::GetReceiveDirection()
{
    return IsComponentSide() ? DIRECTION_IN : DIRECTION_OUT;
}

