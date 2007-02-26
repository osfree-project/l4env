/**
 *	\file	dice/src/be/BEReplyAnyWaitAnyFunction.cpp
 *	\brief	contains the implementation of the class CBEReplyAnyWaitAnyFunction
 *
 *	\date	Wed Jun 12 2002
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

#include "be/BEReplyAnyWaitAnyFunction.h"
#include "be/BEContext.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEComponent.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"
#include "be/BEMsgBufferType.h"
#include "be/BEUserDefinedType.h"

#include "fe/FEInterface.h"

IMPLEMENT_DYNAMIC(CBEReplyAnyWaitAnyFunction);

CBEReplyAnyWaitAnyFunction::CBEReplyAnyWaitAnyFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CBEReplyAnyWaitAnyFunction, CBEInterfaceFunction);
}

/** destroys the object */
CBEReplyAnyWaitAnyFunction::~CBEReplyAnyWaitAnyFunction()
{
}

/** \brief initializes this function
 *  \param pFEInterface the front-end interface used as reference
 *  \param pContext the context of this creation
 *  \return true if successful
 */
bool CBEReplyAnyWaitAnyFunction::CreateBackEnd(CFEInterface * pFEInterface, CBEContext * pContext)
{
    pContext->SetFunctionType(FUNCTION_REPLY_ANY_WAIT_ANY);
	// set target file name
	SetTargetFileName(pFEInterface, pContext);
    // name of the function
    m_sName = pContext->GetNameFactory()->GetFunctionName(pFEInterface, pContext);

    if (!CBEInterfaceFunction::CreateBackEnd(pFEInterface, pContext))
        return false;

    // return type -> set to opcode
    // if return var is parameter do not delete it
    String sOpcodeVar = pContext->GetNameFactory()->GetOpcodeVariable(pContext);
    if (!SetReturnVar((CBEType*)pContext->GetClassFactory()->GetNewOpcodeType(), sOpcodeVar, pContext))
    {
        VERBOSE("CBEReplyAnyWaitAnyFunction::CreateBE failed because return var could not be set\n");
        return false;
    }
    // add parameters
    if (!AddMessageBuffer(pFEInterface, pContext))
        return false;

    return true;
}

/** \brief writes the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the operation
 *
 * Do NOT define message buffer - it has been defined outside this function.
 */
void CBEReplyAnyWaitAnyFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    m_pReturnVar->WriteZeroInitDeclaration(pFile, pContext);
}

/** \brief initializes the local variables
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * Do NOT initialize message buffer, this has been done outside this function.
 */
void CBEReplyAnyWaitAnyFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
}

/** \brief check if this parameter should be marshalled
 *  \param pParameter the parameter to test
 *  \param pContext the context of this operation
 *  \return if we need to marshal this parameter
 */
bool CBEReplyAnyWaitAnyFunction::DoMarshalParameter(CBETypedDeclarator * pParameter, CBEContext * pContext)
{
    return false; // no marsahlling
}

/** \brief writes the unmarshalling code
 *  \param pFile the file to write to
 *  \param nStartOffset the offset in the message buffer where to start unmarshalling
 *  \param bUseConstOffset true if nStartOffset should be used
 *  \param pContext the context of this operation
 */
void CBEReplyAnyWaitAnyFunction::WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext)
{
    WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
}

/** \brief writes the invocation code
 *  \param pFile the file to write to
 *  \param pContext the context the of the write operation
 */
void CBEReplyAnyWaitAnyFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    pFile->PrintIndent("/* invoke */\n");
}

/** \brief clean up the mess in this function
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEReplyAnyWaitAnyFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \param pContext the context of this write operation
 *  \return true if successful
 *
 * A reply-any-and-wait-any function is only written at the component's side.
 */
bool CBEReplyAnyWaitAnyFunction::DoWriteFunction(CBEFile * pFile, CBEContext * pContext)
{
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
		if (!IsTargetFile((CBEHeaderFile*)pFile))
			return false;
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEImplementationFile)))
		if (!IsTargetFile((CBEImplementationFile*)pFile))
			return false;
	return pFile->GetTarget()->IsKindOf(RUNTIME_CLASS(CBEComponent));
}

/** \brief write the message buffer parameter to the file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \param bComma true if a comma has to be written before the parameter
 */
void CBEReplyAnyWaitAnyFunction::WriteAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    assert(m_pMsgBuffer);
    if (bComma)
    {
        pFile->Print(",\n");
        pFile->PrintIndent("");
    }
    WriteParameter(pFile, m_pMsgBuffer, pContext);
    CBEInterfaceFunction::WriteAfterParameters(pFile, pContext, true);
}

/** \brief write the message buffer call parameter
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \param bComma true if a comma has to precede the declarators
 */
void CBEReplyAnyWaitAnyFunction::WriteCallAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    assert(m_pMsgBuffer);
    if (bComma)
    {
        pFile->Print(",\n");
        pFile->PrintIndent("");
    }
    if (m_pMsgBuffer->HasReference())
        pFile->Print("&");
    WriteCallParameter(pFile, m_pMsgBuffer, pContext);
    CBEInterfaceFunction::WriteCallAfterParameters(pFile, pContext, true);
}

/** \brief try to find a parameter by its type
 *  \param sTypeName the name of the type
 *  \return a reference to the found parameter
 */
CBETypedDeclarator * CBEReplyAnyWaitAnyFunction::FindParameterType(String sTypeName)
{
    if (m_pMsgBuffer)
    {
        CBEType *pType = m_pMsgBuffer->GetType();
        if (pType->IsKindOf(RUNTIME_CLASS(CBEUserDefinedType)))
        {
            if (((CBEUserDefinedType*)pType)->GetName() == sTypeName)
                return m_pMsgBuffer;
        }
        if (pType->HasTag(sTypeName))
            return m_pMsgBuffer;
    }
    return CBEInterfaceFunction::FindParameterType(sTypeName);
}

/** \brief adds the message buffer parameter to this function
 *  \param pFEInterface the front-end interface to use as reference
 *  \param pContext the context of the create process
 *  \return true if the create process was successful
 */
bool CBEReplyAnyWaitAnyFunction::AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext)
{
    // get class's message buffer
    CBEClass *pClass = GetClass();
    assert(pClass);
    // get message buffer type
    CBEMsgBufferType *pMsgBuffer = pClass->GetMessageBuffer();
    assert(pMsgBuffer);
    // msg buffer not initialized yet
    pMsgBuffer->InitCounts(pClass, pContext);
    // create own message buffer
    m_pMsgBuffer = pContext->GetClassFactory()->GetNewMessageBufferType();
    m_pMsgBuffer->SetParent(this);
    if (!m_pMsgBuffer->CreateBackEnd(pMsgBuffer, pContext))
    {
        delete m_pMsgBuffer;
        m_pMsgBuffer = 0;
        VERBOSE("%s failed because message buffer could not be created\n", __PRETTY_FUNCTION__);
        return false;
    }
    return true;
}

/** \brief gets the direction this function sends from
 *  \return DIRECTION_OUT
 *
 * Because the reply-any-wait-any function is only used at the server's side
 * this function only sends data to the clients, which is DIRECTION_OUT
 */
int CBEReplyAnyWaitAnyFunction::GetSendDirection()
{
    return DIRECTION_OUT;
}

/** \brief gets the direction this function receives from
 *  \return DIRECTION_IN
 *
 * Since this function is only used at the server's side it always
 * receives dtaa from a client, which is DIRECTION_IN.
 */
int CBEReplyAnyWaitAnyFunction::GetReceiveDirection()
{
    return DIRECTION_IN;
}

