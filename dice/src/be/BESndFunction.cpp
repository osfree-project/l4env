/**
 *  \file    dice/src/be/BESndFunction.cpp
 *  \brief   contains the implementation of the class CBESndFunction
 *
 *  \date    01/14/2002
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

#include "BESndFunction.h"
#include "BEContext.h"
#include "BEFile.h"
#include "BEType.h"
#include "BEMsgBuffer.h"
#include "BEClient.h"
#include "BEComponent.h"
#include "BEImplementationFile.h"
#include "BEHeaderFile.h"
#include "BESizes.h"
#include "BEAttribute.h"
#include "BEMarshaller.h"
#include "BEDeclarator.h"
#include "BENameFactory.h"
#include "BEClassFactory.h"
#include "Compiler.h"
#include "Error.h"
#include "TypeSpec-Type.h"
#include "fe/FEOperation.h"

CBESndFunction::CBESndFunction()
: CBEOperationFunction(FUNCTION_SEND)
{ }

/** \brief destructor of target class */
CBESndFunction::~CBESndFunction()
{ }

/** \brief writes the variable initializations of this function
 *  \param pFile the file to write to
 *
 * This implementation should initialize the message buffer and the pointers
 * of the out variables.
 */
void
CBESndFunction::WriteVariableInitialization(CBEFile& pFile)
{
    // init message buffer
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    pMsgBuffer->WriteInitialization(pFile, this, 0, CMsgStructType::Generic);
}

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *
 * This implementation calls the underlying message trasnfer mechanisms
 */
void CBESndFunction::WriteInvocation(CBEFile& pFile)
{
    pFile << "\t/* send */\n";
}

/** \brief writes the return statement
 *  \param pFile the file to write to
 *
 * This implementation is empty, because a send function does not return a
 * value.
 */
void CBESndFunction::WriteReturn(CBEFile& pFile)
{
    pFile << "\treturn;\n";
}

/** \brief creates the back-end send function
 *  \param pFEOperation the corresponding front-end operation
 *  \return true if successful
 */
void
CBESndFunction::CreateBackEnd(CFEOperation * pFEOperation, bool bComponentSide)
{
    // set target file name
    SetTargetFileName(pFEOperation);
    // set name
	SetComponentSide(bComponentSide);
    SetFunctionName(pFEOperation, FUNCTION_SEND);

    CBEOperationFunction::CreateBackEnd(pFEOperation, bComponentSide);
    // set return type
    SetReturnVar(false, 0, TYPE_VOID, string());
    // need a message buffer, don't we?
    AddMessageBuffer(pFEOperation);
    AddLocalVariable(GetMessageBuffer());
    // add marshaller and communication class
    CreateMarshaller();
    CreateCommunication();
    CreateTrace();
}

/** \brief adds the parameters of a front-end function to this function
 *  \return true if successful
 *
 * This implementation adds the return value to the parameter list. The return
 * value is the value returned by the component-function.
 */
void
CBESndFunction::AddBeforeParameters()
{
    CBEOperationFunction::AddBeforeParameters();

    if (!GetReturnType()->IsVoid())
    {
        CBETypedDeclarator *pReturn =
	    (CBETypedDeclarator*)GetReturnVariable()->Clone();
	DIRECTION_TYPE nDir = GetSendDirection();
	ATTR_TYPE nAttr = (nDir == DIRECTION_IN) ? ATTR_IN : ATTR_OUT;
	if (!pReturn->m_Attributes.Find(nAttr))
	{
	    CBEAttribute *pAttr = pReturn->m_Attributes.Find((nAttr == ATTR_IN) ?
		ATTR_OUT : ATTR_IN);
	    pAttr->CreateBackEnd(nAttr);
	}
	m_Parameters.Add(pReturn);
    }
}

/** \brief test if this function should be written to the target file
 *  \param pFile the file to write to
 *  \return true if successful
 *
 * A send function is written at the client's side if the IN attribute is set,
 * and at the component's side if the OUT attribute is set. And of course, only
 * if the target file is suitable.
 *
 * A send function is not written if it has a uuid-range attribute.
 */
bool CBESndFunction::DoWriteFunction(CBEHeaderFile* pFile)
{
    if (!IsTargetFile(pFile))
        return false;
	if (m_Attributes.Find(ATTR_UUID_RANGE))
		return false;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) && (m_Attributes.Find(ATTR_IN)))
        return true;
    if (pFile->IsOfFileType(FILETYPE_COMPONENT) && (m_Attributes.Find(ATTR_OUT)))
        return true;
    return false;
}

/** \brief test if this function should be written to the target file
 *  \param pFile the file to write to
 *  \return true if successful
 *
 * A send function is written at the client's side if the IN attribute is set,
 * and at the component's side if the OUT attribute is set. And of course, only
 * if the target file is suitable.
 *
 * A send function is not written if it has a uuid-range attribute.
 */
bool CBESndFunction::DoWriteFunction(CBEImplementationFile* pFile)
{
    if (!IsTargetFile(pFile))
        return false;
	if (m_Attributes.Find(ATTR_UUID_RANGE))
		return false;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) && (m_Attributes.Find(ATTR_IN)))
        return true;
    if (pFile->IsOfFileType(FILETYPE_COMPONENT) && (m_Attributes.Find(ATTR_OUT)))
        return true;
    return false;
}

/** \brief return the direction, which this functions sends to
 *  \return DIRECTION_IN if sending to server, DIRECTION_OUT if sending to client
 */
DIRECTION_TYPE CBESndFunction::GetSendDirection()
{
    return IsComponentSide() ? DIRECTION_OUT : DIRECTION_IN;
}

/** \brief get the direction this function receives data from
 *  \return DIRECTION_IN if receiving from client, DIRECTION_OUT if receiving from server
 *
 * Since this function only sends data, the value should be superfluous.
 */
DIRECTION_TYPE CBESndFunction::GetReceiveDirection()
{
    return IsComponentSide() ? DIRECTION_IN : DIRECTION_OUT;
}

/** \brief calcualtes the size of this function
 *  \param nDirection the direction to calulate the size for
 *  \return the size of the function's parameters in bytes
 */
int CBESndFunction::GetSize(DIRECTION_TYPE nDirection)
{
    // get base class' size
    int nSize = CBEOperationFunction::GetSize(nDirection);
    if ((nDirection & GetSendDirection()) &&
        !m_Attributes.Find(ATTR_NOOPCODE))
        nSize += CCompiler::GetSizes()->GetOpcodeSize();
    return nSize;
}

/** \brief calculates the size of the fixed sized params of this function
 *  \param nDirection the direction to calc
 *  \return the size of the params in bytes
 */
int CBESndFunction::GetFixedSize(DIRECTION_TYPE nDirection)
{
    int nSize = CBEOperationFunction::GetFixedSize(nDirection);
    if ((nDirection & GetSendDirection()) &&
        !m_Attributes.Find(ATTR_NOOPCODE))
        nSize += CCompiler::GetSizes()->GetOpcodeSize();
    return nSize;
}

/** \brief marshals the return value
 *  \param pFile the file to write to
 *  \param bMarshal true if marshaling, false if unmarshaling
 *  \return the number of bytes used for the return value
 *
 * This function assumes that it is called before the marshalling of the other
 * parameters.
 */
int
CBESndFunction::WriteMarshalReturn(CBEFile& pFile,
    bool bMarshal)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s called\n", __func__, GetName().c_str());

    CBENameFactory *pNF = CBENameFactory::Instance();
    string sReturn = pNF->GetReturnVariable();
    CBETypedDeclarator *pReturn = FindParameter(sReturn);
    if (!pReturn)
        return 0;
    CBEType *pType = pReturn->GetType();
    if (pType->IsVoid())
        return 0;
    CBEMarshaller *pMarshaller = GetMarshaller();
    pMarshaller->MarshalParameter(pFile, this, pReturn, bMarshal);

    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s for %s returns\n", __func__, GetName().c_str());
    return pType->GetSize();
}

/** \brief manipulates the  message buffer
 *  \param pMsgBuffer the message buffer to initialize
 *  \return true on success
 */
void
CBESndFunction::MsgBufferInitialization(CBEMsgBuffer *pMsgBuffer)
{
    CBEOperationFunction::MsgBufferInitialization(pMsgBuffer);
    // check return type (do test here because sometimes we like to call
    // AddReturnVariable depending on other constraint--return is parameter)

    CBENameFactory *pNF = CBENameFactory::Instance();
    string sReturn = pNF->GetReturnVariable();
    CBETypedDeclarator *pReturn = FindParameter(sReturn);
    if (!pReturn)
        return;
    CBEType *pType = pReturn->GetType();
    if (pType->IsVoid())
        return;
    // add return variable
    if (!pMsgBuffer->AddReturnVariable(this, pReturn))
    {
	string exc = string(__func__);
	exc += " failed, because return variable could not be added to message buffer.";
	throw new error::create_error(exc);
    }
}

