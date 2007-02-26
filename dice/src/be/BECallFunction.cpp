/**
 *    \file    dice/src/be/BECallFunction.cpp
 *    \brief   contains the implementation of the class CBECallFunction
 *
 *    \date    01/18/2002
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

#include "be/BECallFunction.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEAttribute.h"
#include "be/BEHeaderFile.h"
#include "be/BEImplementationFile.h"
#include "be/BEClient.h"
#include "be/BEMsgBufferType.h"

#include "TypeSpec-Type.h"
#include "Attribute-Type.h"
#include "fe/FEOperation.h"

CBECallFunction::CBECallFunction()
{
}

CBECallFunction::CBECallFunction(CBECallFunction & src)
 : CBEOperationFunction(src)
{
}

/** \brief destructor of target class */
CBECallFunction::~CBECallFunction()
{

}

/** \brief writes the variable declarations of this function
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * The variable declarations of the call function include the message buffer
 * for send and receive.  This implementation should initialize the message
 * buffer and the pointers of the out variables.  It sets the return variable
 * (if it exists) to a zero value.
 *
 * If we have variable sized array parameters, we need the temp offset
 * variable.
 */
void 
CBECallFunction::WriteVariableDeclaration(CBEFile * pFile, 
    CBEContext * pContext)
{
    VERBOSE("CBECallFunction::WriteVariableDeclaration called %s in %s\n",
        GetName().c_str(), pFile->GetFileName().c_str());
    // declare message buffer
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->WriteDefinition(pFile, false, pContext);
    // declare return variable
    WriteReturnVariableDeclaration(pFile, pContext);
    // check for temp
    CBENameFactory *pNF = pContext->GetNameFactory();
    if (HasVariableSizedParameters() || HasArrayParameters())
    {
        string sOffsetVar = pNF->GetOffsetVariable(pContext);
        string sTmpVar = pNF->GetTempOffsetVariable(pContext);
	*pFile << "\tunsigned " << sTmpVar << " __attribute__ ((unused));\n";
	*pFile << "\tunsigned " << sOffsetVar << " __attribute__ ((unused));\n";
    }
    if (!FindAttribute(ATTR_NOEXCEPTIONS))
        // declare local exception variable
        WriteExceptionWordDeclaration(pFile, false /* do not init variable*/, 
	    pContext);
}

/** \brief writes the variable initializations of this function
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void 
CBECallFunction::WriteVariableInitialization(CBEFile * pFile, 
    CBEContext * pContext)
{
    VERBOSE("CBECallFunction::WriteVariableInitialization called %s in %s\n",
        GetName().c_str(), pFile->GetFileName().c_str());
    // init message buffer
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->WriteInitialization(pFile, pContext);
}

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This implementation calls the underlying message trasnfer mechanisms
 */
void CBECallFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{

}

/** \brief writes the unmarshalling of the message
 *  \param pFile the file to write to
 *  \param nStartOffset the position in the message buffer to start with 
 *         unmarshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to i
 *         false if not possible
 *  \param pContext the context of the write operation
 *
 * This implementation should unpack the out parameters from the returned
 * message structure
 */
void 
CBECallFunction::WriteUnmarshalling(CBEFile * pFile, 
    int nStartOffset, 
    bool& bUseConstOffset, 
    CBEContext * pContext)
{
    VERBOSE("CBECallFunction::WriteUnmarshalling(%s) called\n", 
	GetName().c_str());

    // unmarshal exception first
    nStartOffset += WriteUnmarshalException(pFile, nStartOffset, bUseConstOffset, pContext);
    // test for exception and return
    WriteExceptionCheck(pFile, pContext); // resets exception
    // unmarshal return variable
    nStartOffset += WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
    // now unmarshal rest
    CBEOperationFunction::WriteUnmarshalling(pFile, nStartOffset, bUseConstOffset, pContext);

    VERBOSE("CBECallFunction::WriteUnmarshalling(%s) finished\n", GetName().c_str());
}

/** \brief clean up the mess
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This implementation cleans up allocated memory inside this function
 */
void CBECallFunction::WriteCleanup(CBEFile * pFile, CBEContext * pContext)
{
    VERBOSE("CBECallFunction::WriteCleanup(%s) called (finished)\n",
        GetName().c_str());
}

/** \brief creates the call function
 *  \param pFEOperation the front-end operation used as reference
 *  \param pContext the context of the write operation
 *  \return true if successful
 *
 * This implementation only sets the name of the function.
 */
bool CBECallFunction::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    VERBOSE("%s for operation %s called\n", __PRETTY_FUNCTION__,
        pFEOperation->GetName().c_str());

    // call CBEObject's CreateBackEnd method
    if (!CBEObject::CreateBackEnd(pFEOperation))
        return false;

    pContext->SetFunctionType(FUNCTION_CALL);
    // set target file name
    SetTargetFileName(pFEOperation, pContext);
    // set own name
    m_sName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);

    if (!CBEOperationFunction::CreateBackEnd(pFEOperation, pContext))
        return false;

    // add msg buffer
    // its the last, because it needs the existing BE parameters
    if (!AddMessageBuffer(pFEOperation, pContext))
        return false;

    VERBOSE("%s returns true\n", __PRETTY_FUNCTION__);
    return true;
}

/** \brief checks if this parameter has to be marshalled or not
 *  \param pParameter the parameter to be checked
 *  \param pContext the context of this marshalling
 *  \return true if this parameter is marshalled
 *
 * Only marshal those parameters with an IN attribute
 */
bool CBECallFunction::DoMarshalParameter(CBETypedDeclarator *pParameter, CBEContext *pContext)
{
    if (pParameter->FindAttribute(ATTR_IN))
        return true;
    return false;
}

/** \brief check if this parameter has to be unmarshalled
 *  \param pParameter the parameter to unmarshal
 *  \param pContext the context of this unmarshalling
 *  \return true if the parameter should be unmarshalled
 *
 * unmarshal all OUT parameters
 */
bool CBECallFunction::DoUnmarshalParameter(CBETypedDeclarator * pParameter, CBEContext * pContext)
{
    if (pParameter->FindAttribute(ATTR_OUT))
        return true;
    return false;
}

/** \brief checks if this function should be written
 *  \param pFile the target file to write to
 *  \param pContext the context of this write operation
 *  \return true if successful
 *
 * A call function is only written for a client file (it sould not have been
 * created if the attributes (IN,OUT) would not fit).
 */
bool CBECallFunction::DoWriteFunction(CBEHeaderFile * pFile, CBEContext * pContext)
{
    if (!IsTargetFile(pFile))
        return false;
    return pFile->IsOfFileType(FILETYPE_CLIENT);
}

/** \brief checks if this function should be written
 *  \param pFile the target file to write to
 *  \param pContext the context of this write operation
 *  \return true if successful
 *
 * A call function is only written for a client file (it sould not have been
 * created if the attributes (IN,OUT) would not fit).
 */
bool CBECallFunction::DoWriteFunction(CBEImplementationFile * pFile, CBEContext * pContext)
{
    if (!IsTargetFile(pFile))
        return false;
    return pFile->IsOfFileType(FILETYPE_CLIENT);
}

/** \brief calcualtes the size of this function
 *  \param nDirection the direction to calulate the size for
 *  \param pContext the context of the calculation
 *  \return the size of the function's parameters in bytes
 */
int CBECallFunction::GetSize(int nDirection, CBEContext * pContext)
{
    // get base class' size
    int nSize = CBEOperationFunction::GetSize(nDirection, pContext);
    if ((nDirection & DIRECTION_IN) &&
        !FindAttribute(ATTR_NOOPCODE))
        nSize += pContext->GetSizes()->GetOpcodeSize();
    if ((nDirection & DIRECTION_OUT) &&
        !FindAttribute(ATTR_NOEXCEPTIONS))
        nSize += pContext->GetSizes()->GetExceptionSize();
    return nSize;
}

/** \brief calculates the size of the fixed sized params of this function
 *  \param nDirection the direction to calc
 *  \param pContext the context of this counting
 *  \return the size of the params in bytes
 */
int CBECallFunction::GetFixedSize(int nDirection, CBEContext *pContext)
{
    int nSize = CBEOperationFunction::GetFixedSize(nDirection, pContext);
    if ((nDirection & DIRECTION_IN) &&
        !FindAttribute(ATTR_NOOPCODE))
        nSize += pContext->GetSizes()->GetOpcodeSize();
    if ((nDirection & DIRECTION_OUT) &&
        !FindAttribute(ATTR_NOEXCEPTIONS))
        nSize += pContext->GetSizes()->GetExceptionSize();
    return nSize;
}

/** \brief write the variable declaration for the return variable
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * We declare the return variable in a seperate function, because we might want to call
 * this function undependently from the other variable declarations.
 */
void CBECallFunction::WriteReturnVariableDeclaration(CBEFile *pFile, CBEContext *pContext)
{
    m_pReturnVar->WriteZeroInitDeclaration(pFile, pContext);
}
