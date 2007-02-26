/**
 *	\file	dice/src/be/BEReplyWaitFunction.cpp
 *	\brief	contains the implementation of the class CBEReplyWaitFunction
 *
 *	\date	01/14/2002
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

#include "be/BEReplyWaitFunction.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEOpcodeType.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEAttribute.h"
#include "be/BEComponent.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"
#include "be/BEMsgBufferType.h"
#include "be/BEUserDefinedType.h"

#include "fe/FETypeSpec.h"
#include "fe/FEAttribute.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"

IMPLEMENT_DYNAMIC(CBEReplyWaitFunction);

CBEReplyWaitFunction::CBEReplyWaitFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CBEReplyWaitFunction, CBEOperationFunction);
}

CBEReplyWaitFunction::CBEReplyWaitFunction(CBEReplyWaitFunction & src):CBEOperationFunction
    (src)
{
    IMPLEMENT_DYNAMIC_BASE(CBEReplyWaitFunction, CBEOperationFunction);
}

/**	\brief destructor of target class */
CBEReplyWaitFunction::~CBEReplyWaitFunction()
{

}

/**	\brief writes the variable declarations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The variable declarations of the call function includes the opcode. Do not define
 * message buffer - it has been defined outside this function.
 */
void CBEReplyWaitFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    m_pReturnVar->WriteZeroInitDeclaration(pFile, pContext);
    // check for temp
    if (HasVariableSizedParameters() || HasArrayParameters())
    {
        String sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        String sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        pFile->PrintIndent("unsigned %s __attribute__ ((unused));\n", (const char*)sTmpVar);
        pFile->PrintIndent("unsigned %s __attribute__ ((unused));\n", (const char*)sOffsetVar);
    }
}

/**	\brief writes the variable initializations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * We cannot initialize the message buffer, because it may overwrite preset values.
 */
void CBEReplyWaitFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
}

/**	\brief writes the invocation of the message transfer
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation calls the underlying message trasnfer mechanisms
 */
void CBEReplyWaitFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{

}

/**	\brief writes the unmarshalling of the message
 *	\param pFile the file to write to
 *  \param nStartOffset the offset where to start unmarshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *	\param pContext the context of the write operation
 *
 * This implementation should unpack the out parameters from the returned message structure
 */
void CBEReplyWaitFunction::WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
}

/**	\brief clean up the mess
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * This implementation cleans up allocated memory inside this function
 */
void CBEReplyWaitFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{

}

/**	\brief creates the back-end representation of a reply and wait function
 *	\param pFEOperation the corresponding front-end operation
 *	\param pContext the context of the code generation
 *	\return true f successful
 *
 * The base class has to be called first, because:
 * - it sets the return type to the return type of the function
 * - it then calls AddParameters, which uses the return type
 * - after that resets the return type to the correct type of a reply-receive function
 */
bool CBEReplyWaitFunction::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    pContext->SetFunctionType(FUNCTION_REPLY_WAIT);
	// set target file name
	SetTargetFileName(pFEOperation, pContext);
    // set the name
    m_sName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);

    if (!CBEOperationFunction::CreateBackEnd(pFEOperation, pContext))
        return false;

    // return type -> set to opcode
    String sOpcodeVar = pContext->GetNameFactory()->GetOpcodeVariable(pContext);
    if (!SetReturnVar(pContext->GetClassFactory()->GetNewOpcodeType(), sOpcodeVar, pContext))
    {
        VERBOSE("CBERcvAnyFunction::CreateBE failed because return var could not be set\n");
        return false;
    }
    // add the message buffer variable (because we wait for it and return it)
    CFEInterface *pFEInterface = pFEOperation->GetParentInterface();
    ASSERT(pFEInterface);
    // AddMessageBuffer inits the counts of the message buffer with the
    // interface's values
    if (!AddMessageBuffer(pFEInterface, pContext))
        return false;

    return true;
}

/**	\brief adds the parameters of a front-end function to this function
 *	\param pFEOperation the front-end function
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * This implementation adds the return value to the parameter list. The return value is the
 * value returned by the component-function.
 *
 * Since this function is called before the rest of the above CreateBE function is executed, we
 * can assume, that the return variable is still the original function's return variable and
 * not the opcode return variable.
 */
bool CBEReplyWaitFunction::AddParameters(CFEOperation * pFEOperation, CBEContext * pContext)
{
    if (!GetReturnType()->IsVoid())
    {
        // create new parameter
        CBETypedDeclarator *pReturnParam = (CBETypedDeclarator*)(m_pReturnVar->Clone());
        CBEFunction::AddParameter(pReturnParam);
        AddSortedParameter(pReturnParam);
    }
    // call base class to add rest
    return CBEOperationFunction::AddParameters(pFEOperation, pContext);
}

/**	\brief adds a single parameter to this class
 *	\param pFEParameter the parameter to add
 *	\param pContext the context of the operation
 *	\return true if successful
 */
bool CBEReplyWaitFunction::AddParameter(CFETypedDeclarator * pFEParameter, CBEContext * pContext)
{
    if (!(pFEParameter->FindAttribute(ATTR_OUT)))
        return true;
    return CBEOperationFunction::AddParameter(pFEParameter, pContext);
}

/**	\brief calculates the size of the function's parameters
 *	\param nDirection the direction to count
 *  \param pContext the context of this calculation
 *	\return the size of the parameters
 *
 * The reply function has the return value as a parameter. The base class' GetSize function adds the size of the return
 * type (which is the opcode's type) to the sum of the parameters. Thus the opcode size is counted even though it
 * shouldn't. We have to subtract it from the calculated size.
 */
int CBEReplyWaitFunction::GetSize(int nDirection, CBEContext *pContext)
{
    int nSize = CBEOperationFunction::GetSize(nDirection, pContext);
    nSize -= GetReturnType()->GetSize();	// opcode's size
    return nSize;
}

/** \brief checks if this parameter should be marshalled or not
 *  \param pParameter the parameter to be checked
 *  \param pContext the context of this marshalling thing
 *  \return true if the parameter should be marshalled
 *
 * This function only marshals OUT parameters
 */
bool CBEReplyWaitFunction::DoMarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext)
{
    if (pParameter->FindAttribute(ATTR_OUT))
        return true;
    return false;
}

/** \brief test if this function should be written to the target file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if successful
 *
 * A reply-and wait function is only writte at the component's side
 */
bool CBEReplyWaitFunction::DoWriteFunction(CBEFile * pFile, CBEContext * pContext)
{
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEHeaderFile)))
		if (!IsTargetFile((CBEHeaderFile*)pFile))
			return false;
	if (pFile->IsKindOf(RUNTIME_CLASS(CBEImplementationFile)))
		if (!IsTargetFile((CBEImplementationFile*)pFile))
			return false;
	return pFile->GetTarget()->IsKindOf(RUNTIME_CLASS(CBEComponent));
}

/** \brief writes the message buffer parameter
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \param bComma true if a comma has to be written before the parameter
 */
void CBEReplyWaitFunction::WriteAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    ASSERT(m_pMsgBuffer);
    if (bComma)
    {
        pFile->Print(",\n");
        pFile->PrintIndent("");
    }
    WriteParameter(pFile, m_pMsgBuffer, pContext);
    CBEOperationFunction::WriteAfterParameters(pFile, pContext, true);
}

/** \brief write the message buffer call parameter
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \param bComma true if a comma has to proceed the declarators
 */
void CBEReplyWaitFunction::WriteCallAfterParameters(CBEFile * pFile, CBEContext * pContext, bool bComma)
{
    ASSERT(m_pMsgBuffer);
    if (bComma)
    {
        pFile->Print(",\n");
        pFile->PrintIndent("");
    }
    if (m_pMsgBuffer->HasReference())
    {
        if (m_bCastMsgBufferOnCall)
            m_pMsgBuffer->GetType()->WriteCast(pFile, true, pContext);
        pFile->Print("&");
    }
    WriteCallParameter(pFile, m_pMsgBuffer, pContext);
    CBEOperationFunction::WriteCallAfterParameters(pFile, pContext, true);
}

/** \brief tries to find a parameter by its type
 *  \param sTypeName the name of the type
 *  \return a reference to the parameter if found
 */
CBETypedDeclarator * CBEReplyWaitFunction::FindParameterType(String sTypeName)
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
    return CBEOperationFunction::FindParameterType(sTypeName);
}

/** \brief adds the message buffer parameter
 *  \param pFEInterface the respective front-end interface to use as reference
 *  \param pContext the context of the create process
 *  \return true if the creation was successful
 */
bool CBEReplyWaitFunction::AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext)
{
    // get class's message buffer
    CBEClass *pClass = GetClass();
    ASSERT(pClass);
    // get message buffer type
    CBEMsgBufferType *pMsgBuffer = pClass->GetMessageBuffer();
    ASSERT(pMsgBuffer);
    // msg buffer has to be initialized with correct values,
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
    // since we reply to a specific message, we have to set the correct counts
    m_pMsgBuffer->ZeroCounts(GetSendDirection());
    // since InitCounts uses MAX to determine counts, the receive direction will
    // have no effect
    m_pMsgBuffer->InitCounts(this, pContext);
    return true;
}

/** \brief get the direction this functions sends to
 *  \return DIRECTION_IN if sending to server, DIRECTION_OUT if sending to client
 *
 * The reply and wait function always sends data to the client and receives data
 * from the client. Therefore the send-direction is always DIRECTION_OUT.
 */
int CBEReplyWaitFunction::GetSendDirection()
{
    return DIRECTION_OUT;
}

/** \brief get the direction this function receives from
 *  \return DIRECTION_IN
 *
 * The reply-and-wait function always receives from the client, which is
 * DIRECTION_IN.
 */
int CBEReplyWaitFunction::GetReceiveDirection()
{
    return DIRECTION_IN;
}
