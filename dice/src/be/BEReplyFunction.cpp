/**
 *    \file    dice/src/be/BEReplyFunction.cpp
 *    \brief   contains the implementation of the class CBEReplyFunction
 *
 *    \date    06/01/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004
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
#include "be/BEReplyFunction.h"
#include "be/BETypedDeclarator.h"
#include "be/BEContext.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEMsgBufferType.h"
#include "be/BEComponent.h"
#include "be/BEUserDefinedType.h"
#include "be/BEDeclarator.h"

#include "fe/FEOperation.h"
#include "fe/FETypedDeclarator.h"
#include "TypeSpec-Type.h"

CBEReplyFunction::CBEReplyFunction()
{
}

CBEReplyFunction::CBEReplyFunction(CBEReplyFunction & src)
: CBEOperationFunction(src)
{
}

/**    \brief destructor of target class */
CBEReplyFunction::~CBEReplyFunction()
{
}

/**    \brief writes the variable declarations of this function
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * The variable declarations of the reply-only function includes the
 * message buffer, which is a parameter, DON'T declare it here!
 */
void CBEReplyFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // define message buffer
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->WriteDefinition(pFile, false, pContext);
    // check for temp
    if (HasVariableSizedParameters() || HasArrayParameters())
    {
        string sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        string sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        *pFile << "\tunsigned " << sTmpVar << " __attribute__ ((unused));\n";
        *pFile << "\tunsigned " << sOffsetVar << " __attribute__ ((unused));\n";
    }
    if (!FindAttribute(ATTR_NOEXCEPTIONS))
        // declare local exception variable
        WriteExceptionWordDeclaration(pFile, true, pContext);
}

/**    \brief writes the variable initializations of this function
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * This implementation cannot initialize the message buffer, because we might
 * overwrite preset values, such as the specific communication partner.
 */
void CBEReplyFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    // init message buffer
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->WriteInitialization(pFile, pContext);
}

/**    \brief writes the invocation of the message transfer
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * This implementation calls the underlying message trasnfer mechanisms
 */
void CBEReplyFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
}

/**    \brief writes the unmarshalling of the message
 *    \param pFile the file to write to
 *  \param nStartOffset the offset where to start unmarshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *    \param pContext the context of the write operation
 *
 * This implementation unmarshals nothing because we expect no answer.
 */
void CBEReplyFunction::WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
}

/**    \brief clean up the mess
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * This implementation cleans up allocated memory inside this function
 */
void CBEReplyFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{
}

/**    \brief creates the back-end reply only function
 *    \param pFEOperation the corresponding front-end operation
 *    \param pContext the context of the code generation
 *    \return true if successful
 *
 * The base class has to be called first, because:
 * - it sets the return type to the return type of the function
 * - it then calls AddParameters, which uses the return type
 * - after that resets the return type to the correct type of a reply-receive function
 *
 * The return type of a reply-receive function is void, because we don't expect a
 * new message or a reply from the client.
 */
bool CBEReplyFunction::CreateBackEnd(CFEOperation* pFEOperation, CBEContext* pContext)
{
    pContext->SetFunctionType(FUNCTION_REPLY);
    // set target file name
    SetTargetFileName(pFEOperation, pContext);
    // set name
    m_sName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);

    if (!CBEOperationFunction::CreateBackEnd(pFEOperation, pContext))
    {
        VERBOSE("%s failed because base function could not be created\n", __PRETTY_FUNCTION__);
        return false;
    }

    // set return type
    CBEType *pReturnType = pContext->GetClassFactory()->GetNewType(TYPE_VOID);
    pReturnType->SetParent(this);
    if (!pReturnType->CreateBackEnd(false, 0, TYPE_VOID, pContext))
    {
        VERBOSE("%s failed because return type could not be created\n", __PRETTY_FUNCTION__);
        delete pReturnType;
        return false;
    }
    CBEType *pOldType = m_pReturnVar->ReplaceType(pReturnType);
    delete pOldType;

    // need a message buffer, don't we?
    if (!AddMessageBuffer(pFEOperation, pContext))
    {
        VERBOSE("%s failed because message buffer could not be created\n", __PRETTY_FUNCTION__);
        return false;
    }

    return true;
}

/** \brief checks of this parameter is marshalled or not
 *  \param pParameter the parameter to be marshalled
 *  \param pContext the context of this marshalling thing
 *  \return true if this parameter should be marshalled
 *
 * Only OUT parameters are marshalled by this function
 */
bool CBEReplyFunction::DoMarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext)
{
    if (pParameter->FindAttribute(ATTR_OUT))
        return true;
    return false;
}

/** \brief check if this parameter has to be unmarshalled
 *  \param pParameter the parameter to be unmarshalled
 *  \param pContext the context of this unmarshalling
 *  \return true if it should be unmarshalled
 *
 * This implementation is empty, because a reply function does not receive any
 * parameters and thus does not need any unmarshalling.
 */
bool CBEReplyFunction::DoUnmarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext)
{
    return false;
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if should be written
 *
 * A reply-only function is written at the component's side only.
 */
bool CBEReplyFunction::DoWriteFunction(CBEHeaderFile * pFile, CBEContext * pContext)
{
    if (!IsTargetFile(pFile))
        return false;
    return dynamic_cast<CBEComponent*>(pFile->GetTarget());
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if should be written
 *
 * A reply-only function is written at the component's side only.
 */
bool CBEReplyFunction::DoWriteFunction(CBEImplementationFile * pFile, CBEContext * pContext)
{
    if (!IsTargetFile(pFile))
        return false;
    return dynamic_cast<CBEComponent*>(pFile->GetTarget());
}

/** \brief gets the direction this function sends data to
 *  \return depending on the communication side DIRECTION_IN or DIRECTION_OUT
 *
 * If this function is used at the server's side it sends data to the client,
 * which is DIRECTION_OUT.
 */
int CBEReplyFunction::GetSendDirection()
{
    return IsComponentSide() ? DIRECTION_OUT : DIRECTION_IN;
}

/** \brief gets the direction this function receives data from
 *  \return depending on communication side DIRECTION_IN or DIRECTION_OUT
 *
 * If this function is used at the server's side its DIRECTION_IN, which means
 * receiving data from the client.
 */
int CBEReplyFunction::GetReceiveDirection()
{
    return IsComponentSide() ? DIRECTION_IN : DIRECTION_OUT;
}

/**    \brief adds the parameters of a front-end function to this function
 *    \param pFEOperation the front-end function
 *    \param pContext the context of the code generation
 *    \return true if successful
 *
 * This implementation adds the return value to the parameter list. The return value is the
 * value returned by the component-function.
 */
bool CBEReplyFunction::AddParameters(CFEOperation * pFEOperation, CBEContext * pContext)
{
    if (!GetReturnType()->IsVoid())
    {
        CBETypedDeclarator *pReturnParam = (CBETypedDeclarator*)m_pReturnVar->Clone();
        CBEFunction::AddParameter(pReturnParam);
        AddSortedParameter(pReturnParam);
    }
    // call base class to add rest
    return CBEOperationFunction::AddParameters(pFEOperation, pContext);
}

/** \brief writes the marshalling code for this function
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start marshalling in the message buffer
 *  \param bUseConstOffset true if the start offset can be used
 *  \param pContext the context of the write operation
 */
void CBEReplyFunction::WriteMarshalling(CBEFile* pFile,  int nStartOffset,  bool& bUseConstOffset,  CBEContext* pContext)
{
    nStartOffset += WriteMarshalException(pFile, nStartOffset, bUseConstOffset, pContext);
    // marshal rest
    CBEOperationFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
}

/** \brief get the size of fixed size parameters
 *  \param nDirection the direction to check
 *  \param pContext the context of the operation
 *  \return the number of bytes
 */
int CBEReplyFunction::GetFixedSize(int nDirection,  CBEContext* pContext)
{
    int nSize = CBEOperationFunction::GetFixedSize(nDirection, pContext);
    if ((nDirection & DIRECTION_OUT) &&
        !FindAttribute(ATTR_NOEXCEPTIONS))
        nSize += pContext->GetSizes()->GetExceptionSize();
    return nSize;
}

/** \brief get the size of parameters
 *  \param nDirection the direction to check
 *  \param pContext the context of the operation
 *  \return the number of bytes
 */
int CBEReplyFunction::GetSize(int nDirection,  CBEContext* pContext)
{
    int nSize = CBEOperationFunction::GetSize(nDirection, pContext);
    if ((nDirection & DIRECTION_OUT) &&
        !FindAttribute(ATTR_NOEXCEPTIONS))
        nSize += pContext->GetSizes()->GetExceptionSize();
    return nSize;
}


/** \brief adds a single parameter to this function
 *  \param pFEParameter the parameter to add
 *  \param pContext the context of the code generation
 *  \return true if successful
 *
 * This function decides, which parameters to add and which not. The parameters to reply are
 * for component-to-client reply the OUT parameters.
 */
bool CBEReplyFunction::AddParameter(CFETypedDeclarator * pFEParameter, CBEContext * pContext)
{
    if (!(pFEParameter->FindAttribute(ATTR_OUT)))
        // skip adding a parameter if it has no OUT
        return true;
    return CBEOperationFunction::AddParameter(pFEParameter, pContext);
}
