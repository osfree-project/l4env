/**
 *  \file    dice/src/be/BEWaitFunction.cpp
 *  \brief   contains the implementation of the class CBEWaitFunction
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

#include "BEWaitFunction.h"
#include "BEContext.h"
#include "BEFile.h"
#include "BEUserDefinedType.h"
#include "BEDeclarator.h"
#include "BEAttribute.h"
#include "BEClient.h"
#include "BEComponent.h"
#include "BEImplementationFile.h"
#include "BEHeaderFile.h"
#include "BEMsgBuffer.h"
#include "BEMarshaller.h"
#include "BESizes.h"
#include "Compiler.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"
#include "fe/FEOperation.h"
#include "fe/FETypedDeclarator.h"
#include <cassert>

CBEWaitFunction::CBEWaitFunction(bool bOpenWait)
    : CBEOperationFunction(bOpenWait ? FUNCTION_WAIT : FUNCTION_RECV)
{
    m_bOpenWait = bOpenWait;
}

CBEWaitFunction::CBEWaitFunction(CBEWaitFunction & src)
: CBEOperationFunction(src)
{
    m_bOpenWait = src.m_bOpenWait;
}

/** \brief destructor of target class */
CBEWaitFunction::~CBEWaitFunction()
{
}

/** \brief writes the variable initializations of this function
 *  \param pFile the file to write to
 *
 * This implementation should initialize the message buffer and the pointers 
 * of the out variables.
 */
void 
CBEWaitFunction::WriteVariableInitialization(CBEFile * pFile)
{
    // initialize message buffer
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    pMsgBuffer->WriteInitialization(pFile, this, 0, CMsgStructType::Generic);
}

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *
 * This implementation calls the underlying message trasnfer mechanisms
 */
void CBEWaitFunction::WriteInvocation(CBEFile * pFile)
{
    *pFile << "\t/* invoke */\n";

    WriteOpcodeCheck(pFile);
}

/** \brief creates the back-end wait function
 *  \param pFEOperation the corresponding front-end operation
 *  \return true if successful
 */
void 
CBEWaitFunction::CreateBackEnd(CFEOperation * pFEOperation)
{
    FUNCTION_TYPE nFunctionType = FUNCTION_NONE;
    if (m_bOpenWait)
	nFunctionType = FUNCTION_WAIT;
    else
	nFunctionType = FUNCTION_RECV;
    SetFunctionName(pFEOperation, nFunctionType);
    
    // set target file name
    SetTargetFileName(pFEOperation);

    CBEOperationFunction::CreateBackEnd(pFEOperation);

    // add message buffer as local variable
    AddMessageBuffer(pFEOperation);
    // then add as local variable
    AddLocalVariable(GetMessageBuffer());
    // add opcode as local variable
    CBETypedDeclarator *pOpcode = CreateOpcodeVariable();
    AddLocalVariable(pOpcode);
    // opcode var might not be used
    pOpcode->AddLanguageProperty(string("attribute"),
	string("__attribute__ ((unused))"));
    // add marshaller and communication class
    CreateMarshaller();
    CreateCommunication();
    CreateTrace();

    // set initializer of return variable to zero
    CBETypedDeclarator *pVariable = GetReturnVariable();
    if (pVariable)
        pVariable->SetDefaultInitString(string("0"));
}

/** \brief adds the OUT attribute to open wait CORBA_Object variable
 *  \return true if successful
 */
void
CBEWaitFunction::CreateObject()
{
    // call base
    CBEOperationFunction::CreateObject();

    if (!m_bOpenWait)
	return;

    // get object
    CBETypedDeclarator *pObj = GetObject();
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBEAttribute *pAttr = pCF->GetNewAttribute();
    pAttr->CreateBackEnd(ATTR_OUT);
    pObj->m_Attributes.Add(pAttr);
}

/** \brief checks if this parameter should be marshalled
 *  \param pParameter the parameter to be marshalled
 *  \param bMarshal true if marshaling, false if unmarshaling
 *  \return true if this parameter should be marshalled
 *
 * Return false if marshalling, because this function does only receive data.
 */
bool
CBEWaitFunction::DoMarshalParameter(CBETypedDeclarator * pParameter,
	bool bMarshal)
{
    if (bMarshal)
	return false;
    
    if (!CBEOperationFunction::DoMarshalParameter(pParameter, bMarshal))
	return false;

    if (IsComponentSide())
    {
        if (pParameter->m_Attributes.Find(ATTR_IN))
            return true;
    }
    else
    {
        if (pParameter->m_Attributes.Find(ATTR_OUT))
            return true;
    }
    return false;
}

/** \brief writes opcode check code
 *  \param pFile the file to write to
 */
void
CBEWaitFunction::WriteOpcodeCheck(CBEFile *pFile)
{
    /* if the noopcode option is set, we cannot check for the correct opcode */
    if (m_Attributes.Find(ATTR_NOOPCODE))
        return;

    // unmarshal opcode variable
    WriteMarshalOpcode(pFile, false);
   
    // now check if opcode in variable is our opcode
    string sSetFunc;
    if (((CBEUserDefinedType*)GetEnvironment()->GetType())->GetName() ==
        "CORBA_Server_Environment")
        sSetFunc = "CORBA_server_exception_set";
    else
        sSetFunc = "CORBA_exception_set";

    /* now check for correct opcode and set exception */
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sOpcode = pNF->GetOpcodeVariable();
    CBETypedDeclarator *pOpcode = m_LocalVariables.Find(sOpcode);
    CDeclStack vStack;
    vStack.push_back(pOpcode->m_Declarators.First());
    
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true);
    CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
    *pFile << "\tif (";
    pMsgBuffer->WriteAccess(pFile, this, GetReceiveDirection(), &vStack);
    *pFile << " != " << m_sOpcodeConstName << ")\n";
    *pFile << "\t{\n";
    pFile->IncIndent();
    string sException = pNF->GetCorbaEnvironmentVariable();
    *pFile << "\t" << sSetFunc << "(" << sException << ",\n";
    pFile->IncIndent();
    *pFile << "\tCORBA_SYSTEM_EXCEPTION,\n";
    *pFile << "\tCORBA_DICE_EXCEPTION_WRONG_OPCODE,\n";
    *pFile << "\t0);\n";
    pFile->DecIndent();
    WriteReturn(pFile);
    pFile->DecIndent();
    *pFile << "\t}\n";
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return true if should be written
 *
 * A wait function should be written at client's side if OUT attribute or
 * at component's side if IN attribute.
 */
bool CBEWaitFunction::DoWriteFunction(CBEHeaderFile * pFile)
{
    if (!IsTargetFile(pFile))
        return false;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
        (m_Attributes.Find(ATTR_OUT)))
        return true;
    if (pFile->IsOfFileType(FILETYPE_COMPONENT) &&
        (m_Attributes.Find(ATTR_IN)))
        return true;
    return false;
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return true if should be written
 *
 * A wait function should be written at client's side if OUT attribute or
 * at component's side if IN attribute.
 */
bool CBEWaitFunction::DoWriteFunction(CBEImplementationFile * pFile)
{
    if (!IsTargetFile(pFile))
        return false;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
        (m_Attributes.Find(ATTR_OUT)))
        return true;
    if (pFile->IsOfFileType(FILETYPE_COMPONENT) &&
        (m_Attributes.Find(ATTR_IN)))
        return true;
    return false;
}

/** \brief gets the direction for the sending data
 *  \return if at client's side DIRECTION_IN, else DIRECTION_OUT
 *
 * The wait function is used to receive a simple message (not RPC), so the
 * send direction depends on the position of the function:
 * At the server side, a wait function receives IN parameters, thus its send
 * direction is OUT. At the client side its vice versa.
 */
DIRECTION_TYPE CBEWaitFunction::GetSendDirection()
{
    return IsComponentSide() ? DIRECTION_OUT : DIRECTION_IN;
}

/** \brief gets the direction for the receiving data
 *  \return if at client's side DIRECTION_OUT, else DIRECTION_IN
 */
DIRECTION_TYPE CBEWaitFunction::GetReceiveDirection()
{
    return IsComponentSide() ? DIRECTION_IN : DIRECTION_OUT;
}

/** \brief adds a single parameter to this function
 *  \param pFEParameter the parameter to add
 *  \return true if successful
 *
 * This function decides, which parameters to add and which don't. The
 * parameters to unmarshal are for client-to-component transfer the IN
 * parameters and for component-to-client transfer the OUT and return
 * parameters. We depend on the information set in m_bComponentSide.
 *
 * For some parameters we have to add additional references:
 * # [in, string] parameter
 * Because every parameter is set inside this functions, all parameters should
 * be referenced.  Since OUT parameters are already referenced, we only need
 * to add an asterisk to IN parameters.  This function is only called when
 * iterating over the declarators of a typed declarator. Thus the direction of
 * a parameter can be found out by checking the attributes of the declarator's
 * parent.  To be sure, whether this parameter needs an additional star, we
 * check the existing number of stars.
 *
 * We also need to add an asterisk to the message buffer parameter.
 *
 * (The CORBA_Object parameter needs no additional reference, it is itself a
 * pointer.)
 */
void CBEWaitFunction::AddParameter(CFETypedDeclarator * pFEParameter)
{
    if (IsComponentSide())
    {
        if (!(pFEParameter->m_Attributes.Find(ATTR_IN)))
            return;
    }
    else
    {
        if (!(pFEParameter->m_Attributes.Find(ATTR_OUT)))
            return;
    }
    CBEOperationFunction::AddParameter(pFEParameter);
    // retrieve the parameter
    CBETypedDeclarator* pParameter = m_Parameters.Find(pFEParameter->m_Declarators.First()->GetName());
    // base class can have decided to skip parameter
    if (!pParameter)
	return;
    // skip CORBA_Object
    if (pParameter == GetObject())
	return;
    if (pParameter->m_Attributes.Find(ATTR_IN))
    {
	bool bAdd = false;
        CBEType *pType = pParameter->GetType();
        CBEAttribute *pAttr;
        if ((pAttr = pParameter->m_Attributes.Find(ATTR_TRANSMIT_AS)) != 0)
            pType = pAttr->GetAttrType();
	CBEDeclarator *pDeclarator = pParameter->m_Declarators.First();
        int nArrayDimensions = pDeclarator->GetArrayDimensionCount() - 
	    pType->GetArrayDimensionCount();
        if ((pDeclarator->GetStars() == 0) && (nArrayDimensions <= 0))
            bAdd = true;
        if ((pParameter->m_Attributes.Find(ATTR_STRING)) &&
             pType->IsOfType(TYPE_CHAR) &&
            (pDeclarator->GetStars() < 2))
            bAdd = true;
        if ((pParameter->m_Attributes.Find(ATTR_SIZE_IS) ||
            pParameter->m_Attributes.Find(ATTR_LENGTH_IS) ||
            pParameter->m_Attributes.Find(ATTR_MAX_IS)) &&
            (nArrayDimensions <= 0))
            bAdd = true;

	if (bAdd)
	    pDeclarator->IncStars(1);
    }
}

/** \brief calculates the size of the fixed sized params of this function
 *  \param nDirection the direction to calc
 *  \return the size of the params in bytes
 */
int CBEWaitFunction::GetSize(DIRECTION_TYPE nDirection)
{
    // get base class' size
    int nSize = CBEOperationFunction::GetSize(nDirection);
    if ((nDirection & DIRECTION_IN) &&
        !m_Attributes.Find(ATTR_NOOPCODE))
        nSize += CCompiler::GetSizes()->GetOpcodeSize();
    if ((nDirection & DIRECTION_OUT) &&
        !m_Attributes.Find(ATTR_NOEXCEPTIONS))
        nSize += CCompiler::GetSizes()->GetExceptionSize();
    return nSize;
}

/** \brief calculates the size of the fixed sized params of this function
 *  \param nDirection the direction to calc
 *  \return the size of the params in bytes
 */
int CBEWaitFunction::GetFixedSize(DIRECTION_TYPE nDirection)
{
    // get base class' size
    int nSize = CBEOperationFunction::GetFixedSize(nDirection);
    if ((nDirection & DIRECTION_IN) &&
        !m_Attributes.Find(ATTR_NOOPCODE))
        nSize += CCompiler::GetSizes()->GetOpcodeSize();
    if ((nDirection & DIRECTION_OUT) &&
        !m_Attributes.Find(ATTR_NOEXCEPTIONS))
        nSize += CCompiler::GetSizes()->GetExceptionSize();
    return nSize;
}

/** \brief manipulates the  message buffer
 *  \param pMsgBuffer the message buffer to initialize
 *  \return true on success
 *
 * This method is called twice: first during creation:
 * * Class::CreateBackEnd
 * * Class::CreateFunctionsNoClassDependency
 * * WaitFunction::CreateBackEnd
 * * Function::AddMessageBuffer
 * * WaitFunction::MsgBufferInitialization
 *
 * second during class initialization:
 * * Class::CreateBackEnd
 * * Class::AddMessageBuffer
 * * Class::MsgBufferInitialization
 * * WaitFunction::MsgBufferInitialization
 */
void
CBEWaitFunction::MsgBufferInitialization(CBEMsgBuffer *pMsgBuffer)
{
    CBEOperationFunction::MsgBufferInitialization(pMsgBuffer);
    // check return type (do test here because sometimes we like to call
    // AddReturnVariable depending on other constraint--return is parameter)
    CBEType *pType = GetReturnType();
    assert(pType);
    if (pType->IsVoid())
	return; // having a void return type is not an error
    // add return variable
    if (!pMsgBuffer->AddReturnVariable(this))
    {
	string exc = string(__func__);
	exc += " failed, because return variable could not be added to message buffer.";
	throw new CBECreateException(exc);
    }
}

