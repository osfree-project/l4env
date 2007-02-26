/**
 *	\file	dice/src/be/BEWaitAnyFunction.cpp
 *	\brief	contains the implementation of the class CBEWaitAnyFunction
 *
 *	\date	01/21/2002
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

#include "be/BEWaitAnyFunction.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEOpcodeType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEComponent.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"
#include "be/BEMsgBufferType.h"
#include "be/BEUserDefinedType.h"

#include "fe/FETypeSpec.h"
#include "fe/FEInterface.h"

IMPLEMENT_DYNAMIC(CBEWaitAnyFunction);

CBEWaitAnyFunction::CBEWaitAnyFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CBEWaitAnyFunction, CBEInterfaceFunction);
}

CBEWaitAnyFunction::CBEWaitAnyFunction(CBEWaitAnyFunction & src)
: CBEInterfaceFunction(src)
{
    IMPLEMENT_DYNAMIC_BASE(CBEWaitAnyFunction, CBEInterfaceFunction);
}

/**	\brief destructor of target class */
CBEWaitAnyFunction::~CBEWaitAnyFunction()
{

}

/**	\brief creates the wait-any function for the given interface
 *	\param pFEInterface the respective front-end interface
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * A function which waits for any message from any sender, does return the opcode of the
 * received message and has as a parameter a reference to the message buffer.
 */
bool CBEWaitAnyFunction::CreateBackEnd(CFEInterface * pFEInterface, CBEContext * pContext)
{
    pContext->SetFunctionType(FUNCTION_WAIT_ANY);
	// set target file name
	SetTargetFileName(pFEInterface, pContext);
    // name of the function
    m_sName = pContext->GetNameFactory()->GetFunctionName(pFEInterface, pContext);

    if (!CBEInterfaceFunction::CreateBackEnd(pFEInterface, pContext))
        return false;

    // return type -> set to opcode
    // if return var is parameter do not delete it
    String sOpcodeVar = pContext->GetNameFactory()->GetOpcodeVariable(pContext);
    if (!SetReturnVar(pContext->GetClassFactory()->GetNewOpcodeType(), sOpcodeVar, pContext))
    {
        VERBOSE("CBEWaitAnyFunction::CreateBE failed because return var could not be set\n");
        return false;
    }
    // add parameters
    if (!AddMessageBuffer(pFEInterface, pContext))
        return false;

    return true;
}

/**	\brief writes the variable declarations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * No message buffer variable - its a parameter.
 */
void CBEWaitAnyFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    m_pReturnVar->WriteZeroInitDeclaration(pFile, pContext);
}

/**	\brief writes the variable initializations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation should initialize the message buffer and the pointers of the out variables.
 * We init the message buffer, because we have nothing to send and want the message buffer to be
 * in a defined state.
 */
void CBEWaitAnyFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
}

/**	\brief writes the invocation of the message transfer
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation calls the underlying message trasnfer mechanisms
 */
void CBEWaitAnyFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    pFile->PrintIndent("/* invoke */\n");
}

/**	\brief writes the unmarshalling of the message
 *	\param pFile the file to write to
 *  \param nStartOffset the offset where to start unmarshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *	\param pContext the context of the write operation
 *
 * This implementation should unpack the out parameters from the returned message structure
 *
 * This implementation unmarshals the "return variable", which is the opcode.
 */
void CBEWaitAnyFunction::WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
}

/**	\brief clean up the mess
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation has nothing to clean up
 */
void CBEWaitAnyFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{
}

/** \brief check if this parameter is marshalled
 *  \param pParameter the parameter to marshal
 *  \param pContext the context of this marshalling thing
 *  \return true if if is marshalled
 *
 * Always return false, because this function does not marshal any parameters
 */
bool CBEWaitAnyFunction::DoMarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext)
{
    return false;
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if should be written
 *
 * A wait-any function is written at the component's side.
 */
bool CBEWaitAnyFunction::DoWriteFunction(CBEFile * pFile, CBEContext * pContext)
{
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
		if (!IsTargetFile((CBEHeaderFile*)pFile))
			return false;
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEImplementationFile)))
		if (!IsTargetFile((CBEImplementationFile*)pFile))
			return false;
	return pFile->GetTarget()->IsKindOf(RUNTIME_CLASS(CBEComponent));
}

/** \brief write the message buffer parameter
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \param bComma true if a comma has to be written before the parameter
 */
void CBEWaitAnyFunction::WriteAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    ASSERT(m_pMsgBuffer);
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
 *  \param bComma true if a comma has to be written before the declarators
 */
void CBEWaitAnyFunction::WriteCallAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    ASSERT(m_pMsgBuffer);
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

/** \brief tries to find a parameter by its type
 *  \param sTypeName the name of the type
 *  \return a reference to the found parameter
 */
CBETypedDeclarator * CBEWaitAnyFunction::FindParameterType(String sTypeName)
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
 *  \param pContext the context of the write process
 *  \return true if successful
 */
bool CBEWaitAnyFunction::AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext)
{
    // get class's message buffer
    CBEClass *pClass = GetClass();
    ASSERT(pClass);
    // get message buffer type
    CBEMsgBufferType *pMsgBuffer = pClass->GetMessageBuffer();
    ASSERT(pMsgBuffer);
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

/** \brief gets the direction for the sending data
 *  \return if at client's side DIRECTION_IN, else DIRECTION_OUT
 *
 * Since this function ignores the send part, this value should be not interesting
 */
int CBEWaitAnyFunction::GetSendDirection()
{
    return IsComponentSide() ? DIRECTION_OUT : DIRECTION_IN;
}

/** \brief gets the direction for the receiving data
 *  \return if at client's side DIRECTION_OUT, else DIRECTION_IN
 */
int CBEWaitAnyFunction::GetReceiveDirection()
{
    return IsComponentSide() ? DIRECTION_IN : DIRECTION_OUT;
}
