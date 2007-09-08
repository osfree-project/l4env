/**
 *  \file    dice/src/be/BEReplyFunction.cpp
 *  \brief   contains the implementation of the class CBEReplyFunction
 *
 *  \date    06/01/2002
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
#include "BEReplyFunction.h"
#include "BETypedDeclarator.h"
#include "BEContext.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BEComponent.h"
#include "BEUserDefinedType.h"
#include "BEDeclarator.h"
#include "BEMsgBuffer.h"
#include "BESizes.h"
#include "BEClass.h"
#include "BEMarshaller.h"
#include "Compiler.h"
#include "Error.h"
#include "fe/FEOperation.h"
#include "fe/FETypedDeclarator.h"
#include "TypeSpec-Type.h"
#include <cassert>

CBEReplyFunction::CBEReplyFunction()
: CBEOperationFunction(FUNCTION_REPLY)
{}

CBEReplyFunction::CBEReplyFunction(CBEReplyFunction & src)
: CBEOperationFunction(src)
{}

/** \brief destructor of target class */
CBEReplyFunction::~CBEReplyFunction()
{}

/** \brief writes the variable initializations of this function
 *  \param pFile the file to write to
 *
 * This implementation cannot initialize the message buffer, because we might
 * overwrite preset values, such as the specific communication partner.
 */
void CBEReplyFunction::WriteVariableInitialization(CBEFile& /*pFile*/)
{}

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *
 * This implementation calls the underlying message trasnfer mechanisms
 */
void CBEReplyFunction::WriteInvocation(CBEFile& /*pFile*/)
{}

/** \brief writes the unmarshalling of the message
 *  \param pFile the file to write to
 *
 * This implementation unmarshals nothing because we expect no answer.
 */
void CBEReplyFunction::WriteUnmarshalling(CBEFile& /*pFile*/)
{}

/** \brief clean up the mess
 *  \param pFile the file to write to
 *
 * This implementation cleans up allocated memory inside this function
 */
void CBEReplyFunction::WriteCleanup(CBEFile& /*pFile*/)
{}

/** \brief creates the back-end reply only function
 *  \param pFEOperation the corresponding front-end operation
 *  \return true if successful
 *
 * The base class has to be called first, because:
 * - it sets the return type to the return type of the function
 * - it then calls AddParameters, which uses the return type
 * - after that resets the return type to the correct type of a reply-receive
 * function
 *
 * The return type of a reply-receive function is void, because we don't
 * expect a new message or a reply from the client.
 */
void
CBEReplyFunction::CreateBackEnd(CFEOperation* pFEOperation)
{
    // set target file name
    SetTargetFileName(pFEOperation);
    // set function name
    SetFunctionName(pFEOperation, FUNCTION_REPLY);

    CBEOperationFunction::CreateBackEnd(pFEOperation);
    // need a message buffer, don't we?
    AddMessageBuffer(pFEOperation);
    AddLocalVariable(GetMessageBuffer());
    // set return type
    SetReturnVar(false, 0, TYPE_VOID, string());

    // add marshaller and communication class
    CreateMarshaller();
    CreateCommunication();
    CreateTrace();
}

/** \brief manipulate the message buffer
 *  \param pMsgBuffer the message buffer to initialize
 *  \return true on success
 */
void
CBEReplyFunction::MsgBufferInitialization(CBEMsgBuffer *pMsgBuffer)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);
    CBEOperationFunction::MsgBufferInitialization(pMsgBuffer);
    // check return type (do test here because sometimes we like to call
    // AddReturnVariable under different constraints--return parameter)
    CBEType *pType = GetReturnType();
    assert(pType);
    if (pType->IsVoid())
	return; // having a void return type is not an error
    // add return variable
    if (!pMsgBuffer->AddReturnVariable(this))
    {
	string exc = string(__func__);
	exc += " failed, because return variable could not be added to message buffer.";
	throw new error::create_error(exc);
    }
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s returns true\n", __func__);
}

/** \brief checks of this parameter is marshalled or not
 *  \param pParameter the parameter to be marshalled
 *  \param bMarshal true if marshaling, false if unmarshaling
 *  \return true if this parameter should be marshalled
 *
 * Only OUT parameters are marshalled by this function
 */
bool
CBEReplyFunction::DoMarshalParameter(CBETypedDeclarator * pParameter,
	bool bMarshal)
{
    if (!CBEOperationFunction::DoMarshalParameter(pParameter, bMarshal))
	return false;
    if (!bMarshal)
	return false;
    if (pParameter->m_Attributes.Find(ATTR_OUT))
        return true;
    return false;
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return true if should be written
 *
 * A reply-only function is written at the component's side only.
 */
bool
CBEReplyFunction::DoWriteFunction(CBEHeaderFile* pFile)
{
    if (!IsTargetFile(pFile))
        return false;
    return pFile->IsOfFileType(FILETYPE_COMPONENT);
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return true if should be written
 *
 * A reply-only function is written at the component's side only.
 */
bool
CBEReplyFunction::DoWriteFunction(CBEImplementationFile* pFile)
{
    if (!IsTargetFile(pFile))
        return false;
    return pFile->IsOfFileType(FILETYPE_COMPONENT);
}

/** \brief gets the direction this function sends data to
 *  \return depending on the communication side DIRECTION_IN or DIRECTION_OUT
 *
 * If this function is used at the server's side it sends data to the client,
 * which is DIRECTION_OUT.
 */
DIRECTION_TYPE CBEReplyFunction::GetSendDirection()
{
    return IsComponentSide() ? DIRECTION_OUT : DIRECTION_IN;
}

/** \brief gets the direction this function receives data from
 *  \return depending on communication side DIRECTION_IN or DIRECTION_OUT
 *
 * If this function is used at the server's side its DIRECTION_IN, which means
 * receiving data from the client.
 */
DIRECTION_TYPE CBEReplyFunction::GetReceiveDirection()
{
    return IsComponentSide() ? DIRECTION_IN : DIRECTION_OUT;
}

/** \brief adds the parameters before "normal" parameters
 *
 * This implementation adds the return value to the parameter list. The return
 * value is the value returned by the component-function.
 */
void
CBEReplyFunction::AddBeforeParameters()
{
    // call base class to add object
    CBEOperationFunction::AddBeforeParameters();

    if (!GetReturnType()->IsVoid())
    {
        CBETypedDeclarator *pReturn = GetReturnVariable();
        CBETypedDeclarator *pReturnParam =
	    static_cast<CBETypedDeclarator*>(pReturn->Clone());
        m_Parameters.Add(pReturnParam);
    }
}

/** \brief get exception variable
 *  \return a reference to the exception variable
 */
CBETypedDeclarator*
CBEReplyFunction::GetExceptionVariable()
{
    CBETypedDeclarator *pRet = CBEOperationFunction::GetExceptionVariable();
    if (pRet)
	return pRet;

    // if no parameter, then try to find it in the message buffer
    CBEMsgBuffer *pMsgBuf = GetMessageBuffer();
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s message buffer in class at %p\n",
	__func__, pMsgBuf);
    if (!pMsgBuf)
	return 0;
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sName = pNF->GetExceptionWordVariable();
    pRet = pMsgBuf->FindMember(sName, this, GetSendDirection());
    if (!pRet)
	pRet = pMsgBuf->FindMember(sName, this, GetReceiveDirection());
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s exception var %s at %p\n",
	__func__, sName.c_str(), pRet);

    return pRet;
}

/** \brief get the size of fixed size parameters
 *  \param nDirection the direction to check
 *  \return the number of bytes
 */
int CBEReplyFunction::GetFixedSize(DIRECTION_TYPE nDirection)
{
    int nSize = CBEOperationFunction::GetFixedSize(nDirection);
    if ((nDirection & DIRECTION_OUT) &&
        !m_Attributes.Find(ATTR_NOEXCEPTIONS))
        nSize += CCompiler::GetSizes()->GetExceptionSize();
    return nSize;
}

/** \brief get the size of parameters
 *  \param nDirection the direction to check
 *  \return the number of bytes
 */
int CBEReplyFunction::GetSize(DIRECTION_TYPE nDirection)
{
    int nSize = CBEOperationFunction::GetSize(nDirection);
    if ((nDirection & DIRECTION_OUT) &&
        !m_Attributes.Find(ATTR_NOEXCEPTIONS))
        nSize += CCompiler::GetSizes()->GetExceptionSize();
    return nSize;
}


/** \brief adds a single parameter to this function
 *  \param pFEParameter the parameter to add
 *
 * This function decides, which parameters to add and which not. The
 * parameters to reply are for component-to-client reply the OUT parameters.
 */
void
CBEReplyFunction::AddParameter(CFETypedDeclarator * pFEParameter)
{
    if (!(pFEParameter->m_Attributes.Find(ATTR_OUT)))
        // skip adding a parameter if it has no OUT
        return;
    CBEOperationFunction::AddParameter(pFEParameter);
}

