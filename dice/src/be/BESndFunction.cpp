/**
 *    \file    dice/src/be/BESndFunction.cpp
 *    \brief   contains the implementation of the class CBESndFunction
 *
 *    \date    01/14/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2004
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

#include "be/BESndFunction.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEClient.h"
#include "be/BEComponent.h"
#include "be/BEImplementationFile.h"
#include "be/BEHeaderFile.h"
#include "be/BEMsgBufferType.h"

#include "TypeSpec-Type.h"
#include "fe/FEOperation.h"

CBESndFunction::CBESndFunction()
{
}

CBESndFunction::CBESndFunction(CBESndFunction & src):CBEOperationFunction(src)
{
}

/**    \brief destructor of target class */
CBESndFunction::~CBESndFunction()
{

}

/**    \brief writes the variable declarations of this function
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * The variable declarations of the call function include the message buffer for send and receive.
 */
void CBESndFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // define message buffer
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->WriteDefinition(pFile, false, pContext);
    // check for temp
    if (HasVariableSizedParameters(GetSendDirection()) ||
        HasArrayParameters(GetSendDirection()))
    {
        string sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        string sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        pFile->PrintIndent("unsigned %s __attribute__ ((unused));\n", sTmpVar.c_str());
        pFile->PrintIndent("unsigned %s __attribute__ ((unused));\n", sOffsetVar.c_str());
    }
}

/**    \brief writes the variable initializations of this function
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * This implementation should initialize the message buffer and the pointers of the out variables.
 */
void CBESndFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
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
void CBESndFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    pFile->PrintIndent("/* send */\n");
}

/**    \brief clean up the mess
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * This implementation cleans up allocated memory inside this function
 */
void CBESndFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{
    pFile->PrintIndent("/* clean up */\n");
}

/**    \brief writes the return statement
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * This implementation is empty, because a send function does not return a value.
 */
void CBESndFunction::WriteReturn(CBEFile * pFile, CBEContext * pContext)
{

}

/**    \brief creates the back-end send function
 *    \param pFEOperation the corresponding front-end operation
 *    \param pContext the context of the code generation
 *    \return true if successful
 */
bool CBESndFunction::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    pContext->SetFunctionType(FUNCTION_SEND);
    // set target file name
    SetTargetFileName(pFEOperation, pContext);
    // set name
    m_sName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);

    if (!CBEOperationFunction::CreateBackEnd(pFEOperation, pContext))
        return false;

    // set return type
    CBEType *pReturnType = pContext->GetClassFactory()->GetNewType(TYPE_VOID);
    pReturnType->SetParent(this);
    if (!pReturnType->CreateBackEnd(false, 0, TYPE_VOID, pContext))
    {
        delete pReturnType;
        return false;
    }
    CBEType *pOldType = m_pReturnVar->ReplaceType(pReturnType);
    delete pOldType;

    // need a message buffer, don't we?
    if (!AddMessageBuffer(pFEOperation, pContext))
        return false;

    return true;
}

/** \brief check if this parameter has to be unmarshalled
 *  \param pParameter the parameter to be unmarshalled
 *  \param pContext the context of this unmarshalling
 *  \return true if it should be unmarshalled
 *
 * This implementation is empty, because a send function does not receive any
 * parameters and thus does not need any unmarshalling.
 */
bool CBESndFunction::DoUnmarshalParameter(CBETypedDeclarator * pParameter, CBEContext *pContext)
{
    return false;
}

/** \brief test if this function should be written to the target file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if successful
 *
 * A send function is written at the client's side if the IN attribute is set,
 * and at the component's side if the OUT attribute is set. And of course, only
 * if the target file is suitable.
 */
bool CBESndFunction::DoWriteFunction(CBEHeaderFile * pFile, CBEContext * pContext)
{
    if (!IsTargetFile(pFile))
        return false;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) && (FindAttribute(ATTR_IN)))
        return true;
    if (pFile->IsOfFileType(FILETYPE_COMPONENT) && (FindAttribute(ATTR_OUT)))
        return true;
    return false;
}

/** \brief test if this function should be written to the target file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if successful
 *
 * A send function is written at the client's side if the IN attribute is set,
 * and at the component's side if the OUT attribute is set. And of course, only
 * if the target file is suitable.
 */
bool CBESndFunction::DoWriteFunction(CBEImplementationFile * pFile, CBEContext * pContext)
{
    if (!IsTargetFile(pFile))
        return false;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) && (FindAttribute(ATTR_IN)))
        return true;
    if (pFile->IsOfFileType(FILETYPE_COMPONENT) && (FindAttribute(ATTR_OUT)))
        return true;
    return false;
}

/** \brief return the direction, which this functions sends to
 *  \return DIRECTION_IN if sending to server, DIRECTION_OUT if sending to client
 */
int CBESndFunction::GetSendDirection()
{
    return IsComponentSide() ? DIRECTION_OUT : DIRECTION_IN;
}

/** \brief get the direction this function receives data from
 *  \return DIRECTION_IN if receiving from client, DIRECTION_OUT if receiving from server
 *
 * Since this function only sends data, the value should be superfluous.
 */
int CBESndFunction::GetReceiveDirection()
{
    return IsComponentSide() ? DIRECTION_IN : DIRECTION_OUT;
}

/** \brief calcualtes the size of this function
 *  \param nDirection the direction to calulate the size for
 *  \param pContext the context of the calculation
 *  \return the size of the function's parameters in bytes
 */
int CBESndFunction::GetSize(int nDirection, CBEContext * pContext)
{
    // get base class' size
    int nSize = CBEOperationFunction::GetSize(nDirection, pContext);
    if ((nDirection & DIRECTION_IN) &&
        !FindAttribute(ATTR_NOOPCODE))
        nSize += pContext->GetSizes()->GetOpcodeSize();
    return nSize;
}

/** \brief calculates the size of the fixed sized params of this function
 *  \param nDirection the direction to calc
 *  \param pContext the context of this counting
 *  \return the size of the params in bytes
 */
int CBESndFunction::GetFixedSize(int nDirection, CBEContext *pContext)
{
    int nSize = CBEOperationFunction::GetFixedSize(nDirection, pContext);
    if ((nDirection & DIRECTION_IN) &&
        !FindAttribute(ATTR_NOOPCODE))
        nSize += pContext->GetSizes()->GetOpcodeSize();
    return nSize;
}
