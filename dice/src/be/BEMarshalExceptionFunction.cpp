/**
 *  \file    dice/src/be/BEMarshalExceptionFunction.cpp
 *  \brief   contains the implementation of the class CBEMarshalExceptionFunction
 *
 *  \date    03/20/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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
#include "BEMarshalExceptionFunction.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BEComponent.h"
#include "BEUserDefinedType.h"
#include "BEType.h"
#include "BEMsgBuffer.h"
#include "BEClass.h"
#include "fe/FEOperation.h"
#include "TypeSpec-Type.h"
#include "Compiler.h"
#include <cassert>

CBEMarshalExceptionFunction::CBEMarshalExceptionFunction()
 : CBEOperationFunction(FUNCTION_MARSHAL_EXCEPTION)
{
}

CBEMarshalExceptionFunction::CBEMarshalExceptionFunction(CBEMarshalExceptionFunction & src)
: CBEOperationFunction(src)
{
}

/** \brief destructor of target class */
CBEMarshalExceptionFunction::~CBEMarshalExceptionFunction()
{
}

/** \brief creates the back-end marshal function for exceptions
 *  \param pFEOperation the corresponding front-end operation
 */
void
CBEMarshalExceptionFunction::CreateBackEnd(CFEOperation * pFEOperation)
{
    // set target file name
    SetTargetFileName(pFEOperation);
    // set function name
    SetFunctionName(pFEOperation, FUNCTION_MARSHAL_EXCEPTION);

    // add parameters
    CBEOperationFunction::CreateBackEnd(pFEOperation);

    // replace return variable. Do this before parameter initialisation, so it
    // will _not_ be added to the message buffer.
    if (IsComponentSide())
	SetReturnVar(false, 0, TYPE_VOID, string());

    // add message buffer
    AddMessageBuffer(pFEOperation);

    // add marshaller and communication class
    CreateMarshaller();
    CreateCommunication();
    CreateTrace();
}

/** \brief manipulate the message buffer
 *  \param pMsgBuffer the message buffer to initialize
 *  \return true on success
 *
 * Make the message buffer variable a reference
 */
void
CBEMarshalExceptionFunction::MsgBufferInitialization(CBEMsgBuffer *pMsgBuffer)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);
    CBEOperationFunction::MsgBufferInitialization(pMsgBuffer);
    // in marshal function, the message buffer is a pointer to the server's
    // message buffer
    pMsgBuffer->m_Declarators.First()->SetStars(1);
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s returns true\n", __func__);
}

/** \brief adds a single parameter to this function
 *  \param pFEParameter the parameter to add
 *
 * This function decides, which parameters to add and which not. In the
 * marshal_exc function we do not take any parameters, because we only marshal
 * the exception which is in the environment.
 *
 * \todo change this for C++ if we catch exceptions and hand them to this
 * function as parameters
 */
void
CBEMarshalExceptionFunction::AddParameter(CFETypedDeclarator * /*pFEParameter*/)
{}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return true if should be written
 *
 * A marshal function is written if client's side and IN or if component's side
 * and one of the parameters has an OUT or we have an exception to transmit.
 */
bool
CBEMarshalExceptionFunction::DoWriteFunction(CBEHeaderFile* pFile)
{
    if (!IsTargetFile(pFile))
        return false;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
        (m_Attributes.Find(ATTR_IN)))
        return true;
    if (pFile->IsOfFileType(FILETYPE_COMPONENT))
    {
        /* look for an OUT parameter */
	if (FindParameterAttribute(ATTR_OUT))
	    return true;
        /* look for return type */
        if (GetReturnType() &&
            !GetReturnType()->IsVoid())
            return true;
        /* check for exceptions */
        if (!m_Attributes.Find(ATTR_NOEXCEPTIONS))
            return true;
    }
    return false;
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return true if should be written
 *
 * A marshal function is written if client's side and IN or if component's side
 * and one of the parameters has an OUT or we have an exception to transmit.
 */
bool
CBEMarshalExceptionFunction::DoWriteFunction(CBEImplementationFile* pFile)
{
    if (!IsTargetFile(pFile))
        return false;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
        (m_Attributes.Find(ATTR_IN)))
        return true;
    if (pFile->IsOfFileType(FILETYPE_COMPONENT))
    {
        /* look for an OUT parameter */
	if (FindParameterAttribute(ATTR_OUT))
	    return true;
        /* look for return type */
        CBEType *pRetType = GetReturnType();
        if (pRetType && !pRetType->IsVoid())
            return true;
        /* check for exceptions */
        if (!m_Attributes.Find(ATTR_NOEXCEPTIONS))
            return true;
    }
    return false;
}

/** \brief gets the direction, which the marshal-parameters have
 *  \return if at client's side DIRECTION_IN, if at server's side DIRECTION_OUT
 *
 * Since this function ignores marshalling parameter this value should be
 * irrelevant
 */
DIRECTION_TYPE CBEMarshalExceptionFunction::GetSendDirection()
{
    return IsComponentSide() ? DIRECTION_OUT : DIRECTION_IN;
}

/** \brief gets the direction of the unmarshal-parameters
 *  \return if at client's side DIRECTION_OUT, if at server's side DIRECTION_IN
 */
DIRECTION_TYPE CBEMarshalExceptionFunction::GetReceiveDirection()
{
    return IsComponentSide() ? DIRECTION_IN : DIRECTION_OUT;
}

/** \brief get exception variable
 *  \return a reference to the exception variable
 */
CBETypedDeclarator*
CBEMarshalExceptionFunction::GetExceptionVariable()
{
    CBETypedDeclarator *pRet = CBEOperationFunction::GetExceptionVariable();
    if (pRet)
	return pRet;

    // if no parameter, then try to find it in the message buffer
    CBEMsgBuffer *pMsgBuf = m_pClass->GetMessageBuffer();
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s message buffer in class at %p\n",
	__func__, pMsgBuf);
    if (!pMsgBuf)
	return 0;
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sName = pNF->GetExceptionWordVariable();
    pRet = pMsgBuf->FindMember(sName, this, GetSendDirection());
    if (!pRet)
	pRet = pMsgBuf->FindMember(sName, this, GetReceiveDirection());
    CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s exception var %s at %p\n", __func__,
	sName.c_str(), pRet);

    return pRet;
}

/** \brief tries to find a parameter by its type
 *  \param sTypeName the name of the type
 *  \return a reference to the found parameter
 */
CBETypedDeclarator*
CBEMarshalExceptionFunction::FindParameterType(string sTypeName)
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
    return CBEOperationFunction::FindParameterType(sTypeName);
}

/** \brief writes the variable initializations of this function
 *  \param pFile the file to write to
 *
 * This implementation should initialize the pointers of the out variables. Do
 * not initialize the message buffer - this may overwrite the values we try to
 * unmarshal.
 */
void
CBEMarshalExceptionFunction::WriteVariableInitialization(CBEFile& /*pFile*/)
{
}

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *
 * This implementation does nothing, because the unmarshalling does not
 * contain a message transfer.
 */
void
CBEMarshalExceptionFunction::WriteInvocation(CBEFile& /*pFile*/)
{
}

/** \brief writes a single parameter for the function call
 *  \param pFile the file to write to
 *  \param pParameter the parameter to be written
 *  \param bCallFromSameClass true if called from same class
 *
 * If the parameter is the message buffer we cast to this function's type,
 * because otherwise the compiler issues warnings.
 */
void
CBEMarshalExceptionFunction::WriteCallParameter(CBEFile& pFile,
    CBETypedDeclarator * pParameter,
    bool bCallFromSameClass)
{
    // write own message buffer's name
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sName = pNF->GetMessageBufferVariable();
    if (!bCallFromSameClass && pParameter->m_Declarators.Find(sName))
	pParameter->GetType()->WriteCast(pFile, pParameter->HasReference());
    // call base class
    CBEOperationFunction::WriteCallParameter(pFile, pParameter, bCallFromSameClass);
}

/** \brief add parameters after all other parameters
 *  \return true if successful
 *
 * We have to write the server's message buffer type and the local variable
 * name. We cannot use the local type, because it's not defined anywhere.
 * To write it correctly, we have to obtain the server's message buffer's
 * declarator, which is the name of the type.
 */
void CBEMarshalExceptionFunction::AddAfterParameters()
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEMarshalExceptionFunction::%s called\n", __func__);

    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    // get class' message buffer
    CBEClass *pClass = GetSpecificParent<CBEClass>();
    assert(pClass);
    // get its message buffer
    CBEMsgBuffer *pSrvMsgBuffer = pClass->GetMessageBuffer();
    assert(pSrvMsgBuffer);
    string sTypeName = pSrvMsgBuffer->m_Declarators.First()->GetName();
    // write own message buffer's name
    string sName = pNF->GetMessageBufferVariable();
    // create declarator
    CBETypedDeclarator *pParameter = pCF->GetNewTypedDeclarator();
    pParameter->SetParent(this);
    pParameter->CreateBackEnd(sTypeName, sName, 1);
    m_Parameters.Add(pParameter);

    CBEOperationFunction::AddAfterParameters();

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
	"CBEMarshalExceptionFunction::%s returns\n", __func__);
}

/** \brief checks if this parameter should be marshalled or not
 *  \param pParameter the parameter to check
 *  \param bMarshal true if marshaling, false if unmarshaling
 *  \return true if this parameter should be marshalled
 *
 * Return true if the at component's side, the parameter has an OUT attribute,
 * or if at client's side the parameter has an IN attribute.
 */
bool
CBEMarshalExceptionFunction::DoMarshalParameter(CBETypedDeclarator * pParameter,
	bool bMarshal)
{
    if (!bMarshal)
	return false;

    if (!CBEOperationFunction::DoMarshalParameter(pParameter, bMarshal))
	return false;

    if (IsComponentSide())
    {
        if (pParameter->m_Attributes.Find(ATTR_OUT))
            return true;
    }
    else
    {
        if (pParameter->m_Attributes.Find(ATTR_IN))
            return true;
    }
    return false;
}

