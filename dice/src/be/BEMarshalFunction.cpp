/**
 *  \file    dice/src/be/BEMarshalFunction.cpp
 *  \brief   contains the implementation of the class CBEMarshalFunction
 *
 *  \date    10/09/2003
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
#include "BEMarshalFunction.h"
#include "BEContext.h"
#include "BEType.h"
#include "BEMsgBuffer.h"
#include "BEDeclarator.h"
#include "BEUserDefinedType.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BEComponent.h"
#include "BEClass.h"
#include "BESrvLoopFunction.h"
#include "BESizes.h"
#include "BEClassFactory.h"
#include "BEMarshaller.h"
#include "Compiler.h"
#include "Error.h"
#include "TypeSpec-Type.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FETypedDeclarator.h"
#include <cassert>

CBEMarshalFunction::CBEMarshalFunction()
 : CBEOperationFunction(FUNCTION_MARSHAL)
{ }

CBEMarshalFunction::CBEMarshalFunction(CBEMarshalFunction & src)
: CBEOperationFunction(src)
{ }

/** \brief destructor of target class */
CBEMarshalFunction::~CBEMarshalFunction()
{ }

/** \brief creates the back-end marshal function
 *  \param pFEOperation the corresponding front-end operation
 *  \return true if successful
 *
 * This function should only contain OUT parameters if it is on the
 * component's side an IN parameters if it is on the client's side.
 */
void
CBEMarshalFunction::CreateBackEnd(CFEOperation * pFEOperation)
{
    // set target file name
    SetTargetFileName(pFEOperation);
    // set function name
    SetFunctionName(pFEOperation, FUNCTION_MARSHAL);

    // add parameters
    CBEOperationFunction::CreateBackEnd(pFEOperation);

    // add message buffer 
    // (called before return type replacement, so we can add the "old" return
    // type
    AddMessageBuffer(pFEOperation);
    
    // replace return type after parameters are added, so the parameters can
    // add the original return type as extra parameter
    // set return type
    if (IsComponentSide())
	SetReturnVar(false, 0, TYPE_VOID, string());

    // add marshaller and communication class
    CreateMarshaller();
    CreateCommunication();
    CreateTrace();
}

/** \brief manipulate the message buffer
 *  \param pMsgBuffer the message buffer to initialize
 *  \return true on success
 *
 * Add the return variable and set the message buffer parameter as reference.
 *
 * We have to use the return variable parameter, because the base class
 * function does not add the return variable to the struct if the return type
 * of the function is void, which is always true for marshal function.
 */
void 
CBEMarshalFunction::MsgBufferInitialization(CBEMsgBuffer *pMsgBuffer)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s called\n", __func__);
    CBEOperationFunction::MsgBufferInitialization(pMsgBuffer);
    // add return variable if we have a return parameter
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sReturn = pNF->GetReturnVariable();
    if (FindParameter(sReturn) &&
	!pMsgBuffer->AddReturnVariable(this))
    {
	string exc = string(__func__);
	exc += " failed, because return variable could not be added to message buffer.";
	throw new error::create_error(exc);
    }
    // in marshal function, the message buffer is a pointer to the server's
    // message buffer
    pMsgBuffer->m_Declarators.First()->SetStars(1);
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s returns true\n", __func__);
}

/** \brief writes the variable initializations of this function
 *  \param pFile the file to write to
 *
 * This implementation should initialize the pointers of the out variables. Do
 * not initialize the message buffer - this may overwrite the values we try to
 * unmarshal.
 */
void 
CBEMarshalFunction::WriteVariableInitialization(CBEFile& /*pFile*/)
{
}

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *
 * This implementation does nothing, because the unmarshalling does not
 * contain a message transfer.
 */
void
CBEMarshalFunction::WriteInvocation(CBEFile& /*pFile*/)
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
CBEMarshalFunction::WriteCallParameter(CBEFile& pFile,
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

/** \brief get exception variable
 *  \return a reference to the exception variable
 */
CBETypedDeclarator*
CBEMarshalFunction::GetExceptionVariable()
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

/** \brief adds the parameters of a front-end function to this function
 *  \return true if successful
 *
 * This implementation adds the return value to the parameter list. The return
 * value is the value returned by the component-function.
 *
 * Since this function is called before the rest of the above CreateBE
 * function is executed, we can assume, that the return variable is still the
 * original function's return variable and not the opcode return variable.
 */
void
CBEMarshalFunction::AddBeforeParameters()
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, 
	"CBEMarshalFunction::%s called\n", __func__);
    // call base class to add object
    CBEOperationFunction::AddBeforeParameters();
    
    if (!GetReturnType()->IsVoid())
    {
        // create new parameter
        CBETypedDeclarator *pReturn = GetReturnVariable();
        CBETypedDeclarator *pReturnParam = 
	    (CBETypedDeclarator*)(pReturn->Clone());
        m_Parameters.Add(pReturnParam);
    }
    
    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
	"CBEMarshalFunction::%s returns\n", __func__);
}

/** \brief add parameters after all other parameters
 *  \return true if successful
 *
 * We have to write the server's message buffer type and the local variable
 * name. We cannot use the local type, because it's not defined anywhere.
 * To write it correctly, we have to obtain the server's message buffer's
 * declarator, which is the name of the type.
 */
void CBEMarshalFunction::AddAfterParameters()
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEMarshalFunction::%s called\n", __func__);
    
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
	"CBEMarshalFunction::%s returns\n", __func__);
}

/** \brief adds a single parameter to this function
 *  \param pFEParameter the parameter to add
 *
 * This function decides, which parameters to add and which not. The
 * parameters to marshal are for client-to-component transfer the IN
 * parameters and for component-to-client transfer the OUT and return
 * parameters. We depend on the information set in m_bComponentSide.
 */
void
CBEMarshalFunction::AddParameter(CFETypedDeclarator * pFEParameter)
{
    if (IsComponentSide())
    {
        // we transfer from the component to the client
        if (!(pFEParameter->m_Attributes.Find(ATTR_OUT)))
            // skip adding a parameter if it has no OUT
            return;
    }
    else
    {
        if (!(pFEParameter->m_Attributes.Find(ATTR_IN)))
            // skip parameter if it has no IN
            return;
    }
    return CBEOperationFunction::AddParameter(pFEParameter);
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
CBEMarshalFunction::DoMarshalParameter(CBETypedDeclarator * pParameter, 
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

/** \brief tries to find a parameter by its type
 *  \param sTypeName the name of the type
 *  \return a reference to the found parameter
 */
CBETypedDeclarator * CBEMarshalFunction::FindParameterType(string sTypeName)
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

/** \brief gets the direction, which the marshal-parameters have
 *  \return if at client's side DIRECTION_IN, if at server's side DIRECTION_OUT
 *
 * Since this function ignores marshalling parameter this value should be
 * irrelevant
 */
DIRECTION_TYPE CBEMarshalFunction::GetSendDirection()
{
    return IsComponentSide() ? DIRECTION_OUT : DIRECTION_IN;
}

/** \brief gets the direction of the unmarshal-parameters
 *  \return if at client's side DIRECTION_OUT, if at server's side DIRECTION_IN
 */
DIRECTION_TYPE CBEMarshalFunction::GetReceiveDirection()
{
    return IsComponentSide() ? DIRECTION_IN : DIRECTION_OUT;
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \return the size of the parameters
 *
 * The reply function has the return value as a parameter. The base class'
 * GetSize function adds the size of the return type to the sum of the
 * parameters. Thus the status code for the IPC is counted even though it
 * shouldn't. We have to subtract it from the calculated size.
 */
int CBEMarshalFunction::GetSize(DIRECTION_TYPE nDirection)
{
    int nSize = CBEOperationFunction::GetSize(nDirection);
    if ((nDirection & DIRECTION_OUT) &&
        !m_Attributes.Find(ATTR_NOEXCEPTIONS))
        nSize += CCompiler::GetSizes()->GetExceptionSize();
    return nSize;
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \return the size of the parameters
 */
int CBEMarshalFunction::GetFixedSize(DIRECTION_TYPE nDirection)
{
    int nSize = CBEOperationFunction::GetFixedSize(nDirection);
    if ((nDirection & DIRECTION_OUT) &&
        !m_Attributes.Find(ATTR_NOEXCEPTIONS))
        nSize += CCompiler::GetSizes()->GetExceptionSize();
    return nSize;
}

/** \brief do not count this function's return var
 *  \param nDirection the direction to count
 *  \return zero
 *
 * The return var (or the variable in m_pReturnValue is the IPC reply code,
 * which should not be counted as a parameter.
 */
int CBEMarshalFunction::GetMaxReturnSize(DIRECTION_TYPE /*nDirection*/)
{
    return 0;
}

/** \brief do not count this function's return var
 *  \param nDirection the direction to count
 *  \return zero
 *
 * The return var (or the variable in m_pReturnValue is the IPC reply code,
 * which should not be counted as a parameter.
 */
int CBEMarshalFunction::GetFixedReturnSize(DIRECTION_TYPE /*nDirection*/)
{
    return 0;
}

/** \brief do not count this function's return var
 *  \param nDirection the direction to count
 *  \return zero
 *
 * The return var (or the variable in m_pReturnValue is the IPC reply code,
 * which should not be counted as a parameter.
 */
int CBEMarshalFunction::GetReturnSize(DIRECTION_TYPE /*nDirection*/)
{
    return 0;
}

/** \brief writes the return statement of the function
 *  \param pFile the file to write to
 *
 * The Marshal function has no return value.
 */
void CBEMarshalFunction::WriteReturn(CBEFile& pFile)
{
    pFile << "\treturn;\n";
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return true if should be written
 *
 * A marshal function is written if client's side and IN or if component's side
 * and one of the parameters has an OUT or we have an exception to transmit.
 */
bool
CBEMarshalFunction::DoWriteFunction(CBEHeaderFile* pFile)
{
    if (!IsTargetFile(pFile))
        return false;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
        (m_Attributes.Find(ATTR_IN)))
        return true;
    if (dynamic_cast<CBEComponent*>(pFile->GetTarget()))
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
CBEMarshalFunction::DoWriteFunction(CBEImplementationFile* pFile)
{
    if (!IsTargetFile(pFile))
        return false;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
        (m_Attributes.Find(ATTR_IN)))
        return true;
    if (dynamic_cast<CBEComponent*>(pFile->GetTarget()))
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

