/**
 *  \file    dice/src/be/BEUnmarshalFunction.cpp
 *  \brief   contains the implementation of the class CBEUnmarshalFunction
 *
 *  \date    01/20/2002
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

#include "BEUnmarshalFunction.h"
#include "BEContext.h"
#include "BEComponent.h"
#include "BEFile.h"
#include "BEType.h"
#include "BEDeclarator.h"
#include "BEAttribute.h"
#include "BEClient.h"
#include "BEComponent.h"
#include "BEImplementationFile.h"
#include "BEHeaderFile.h"
#include "BEUserDefinedType.h"
#include "BEMsgBuffer.h"
#include "BEClass.h"
#include "BETypedDeclarator.h"
#include "BESizes.h"
#include "BEMarshaller.h"
#include "Compiler.h"
#include "Error.h"
#include "TypeSpec-Type.h"
#include "fe/FEInterface.h"
#include "fe/FEOperation.h"
#include "fe/FETypedDeclarator.h"
#include <cassert>

CBEUnmarshalFunction::CBEUnmarshalFunction()
 : CBEOperationFunction(FUNCTION_UNMARSHAL)
{ }

CBEUnmarshalFunction::CBEUnmarshalFunction(CBEUnmarshalFunction & src)
 : CBEOperationFunction(src)
{ }

/** \brief destructor of target class */
CBEUnmarshalFunction::~CBEUnmarshalFunction()
{ }

/** \brief creates the back-end unmarshal function
 *  \param pFEOperation the corresponding front-end operation
 *  \return true if successful
 *
 * This function should only contain IN parameters if it is on the component's
 * side an OUT parameters if it is on the client's side.
 */
void
CBEUnmarshalFunction::CreateBackEnd(CFEOperation * pFEOperation)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
	"CBEUnmarshalFunction::%s(%s) called\n", __func__,
	pFEOperation->GetName().c_str());
    // set target file name
    SetTargetFileName(pFEOperation);
    SetFunctionName(pFEOperation, FUNCTION_UNMARSHAL);

    // add parameters
    CBEOperationFunction::CreateBackEnd(pFEOperation);

    // set return type
    if (IsComponentSide())
	SetReturnVar(false, 0, TYPE_VOID, string());
    // add message buffer 
    AddMessageBuffer(pFEOperation);
    // add marshaller and communication class
    CreateMarshaller();
    CreateCommunication();
    CreateTrace();

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, 
	"CBEUnmarshalFunction::%s returns\n", __func__);
}

/** \brief manipulate the message buffer
 *  \param pMsgBuffer the message buffer to initalize
 *  \return true on success
 */
void 
CBEUnmarshalFunction::MsgBufferInitialization(CBEMsgBuffer *pMsgBuffer)
{
    CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, 
	"CBEUnmarshalFunction::%s called\n", __func__);
    CBEOperationFunction::MsgBufferInitialization(pMsgBuffer);
    // in unmarshal function, the message buffer is a pointer to the server's
    // message buffer
    if (IsComponentSide())
	pMsgBuffer->m_Declarators.First()->SetStars(1);
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
	throw new error::create_error(exc);
    }
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
CBEUnmarshalFunction::WriteVariableInitialization(CBEFile * /*pFile*/)
{}

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *
 * This implementation does nothing, because the unmarshalling does not
 * contain a message transfer.
 */
void 
CBEUnmarshalFunction::WriteInvocation(CBEFile * /*pFile*/)
{}

/** \brief writes a single parameter for the function call
 *  \param pFile the file to write to
 *  \param pParameter the parameter to be written
 *  \param bCallFromSameClass true if called from same class
 *
 * If the parameter is the message buffer we cast to this function's type,
 * because otherwise the compiler issues warnings.
 */
void 
CBEUnmarshalFunction::WriteCallParameter(CBEFile * pFile,
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

/** \brief adds a single parameter to this function
 *  \param pFEParameter the parameter to add
 *
 * This function decides, which parameters to add and which don't. The
 * parameters to unmarshal are for client-to-component transfer the IN
 * parameters and for component-to-client transfer the OUT and return
 * parameters. We depend on the information set in m_bComponentSide.
 */
void
CBEUnmarshalFunction::AddParameter(CFETypedDeclarator * pFEParameter)
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
    ATTR_TYPE nDirection = IsComponentSide() ? ATTR_IN : ATTR_OUT;
    if (pParameter->m_Attributes.Find(nDirection))
    {
        CBEType *pType = pParameter->GetType();
        CBEAttribute *pAttr;
        if ((pAttr = pParameter->m_Attributes.Find(ATTR_TRANSMIT_AS)) != 0)
            pType = pAttr->GetAttrType();
	CBEDeclarator *pDeclarator = pParameter->m_Declarators.First();
        int nArrayDimensions = pDeclarator->GetArrayDimensionCount() - 
	    pType->GetArrayDimensionCount();
	// if there are no array dimensions, then we need to add a pointer
        if ((pDeclarator->GetStars() == 0) && !pType->IsPointerType() && 
	    (nArrayDimensions <= 0))
	{
	    pDeclarator->IncStars(1);
            return;
	}
	// checking string
        if (pParameter->m_Attributes.Find(ATTR_STRING) &&
	    pParameter->m_Attributes.Find(ATTR_IN) &&
	    !pParameter->m_Attributes.Find(ATTR_OUT) &&
	    ((pType->IsOfType(TYPE_CHAR) && pDeclarator->GetStars() < 2) ||
	     (pType->IsOfType(TYPE_CHAR_ASTERISK) && 
	      pDeclarator->GetStars() < 1)))
	{
	    pDeclarator->IncStars(1);
            return;
	}

	// check for size attribute and if found, check if it is ours, then it
	// should not be resprected when determining additional reference
	if ((pAttr = pParameter->m_Attributes.Find(ATTR_SIZE_IS)) != 0)
	{
	    CDeclStack vStack;
	    vStack.push_back(pParameter->m_Declarators.First());
	    CBENameFactory *pNF = CCompiler::GetNameFactory();
	    string sName = pNF->GetLocalSizeVariableName(&vStack);

	    // compare to size declarator -> if no such declarator is present,
	    // this is not one of our size attributes -> do the normal check
	    // with the array dimensions and return if true
	    if (!pAttr->m_Parameters.Find(sName) && nArrayDimensions <= 0)
	    {
		pDeclarator->IncStars(1);
		return;
	    }

	    // otherwise either it was one of our own size attributes or the
	    // array dimensions were > 0
	}
        if ((pParameter->m_Attributes.Find(ATTR_LENGTH_IS) ||
            pParameter->m_Attributes.Find(ATTR_MAX_IS)) &&
            (nArrayDimensions <= 0))
	{
	    pDeclarator->IncStars(1);
            return;
	}
    }
}

/** \brief checks if this parameter should be marshalled or not
 *  \param pParameter the parameter to check
 *  \param bMarshal true if marshaling, false if unmarshaling
 *  \return true if this parameter should be marshalled
 *
 * Always return false for marshaling, because this function does only
 * unmarshal parameters. We also have to check for the opcode at the server's
 * side, because it is unmarshaled some other place.
 */
bool
CBEUnmarshalFunction::DoMarshalParameter(CBETypedDeclarator * pParameter,
	bool bMarshal)
{
    if (bMarshal)
	return false;
    
    if (!CBEOperationFunction::DoMarshalParameter(pParameter, bMarshal))
	return false;

    if (IsComponentSide())
    {
	// get opcode's name
	CBENameFactory *pNF = CCompiler::GetNameFactory();
	string sOpcode = pNF->GetOpcodeVariable();
	// now check if this is the opcode parameter
	if (pParameter->m_Declarators.Find(sOpcode))
	    return false;
	// check other parameters
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

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return true if should be written
 *
 * An unmarshal function is written if client's side and OUT or if component's
 * side and one of the parameters has an IN.
 */
bool 
CBEUnmarshalFunction::DoWriteFunction(CBEHeaderFile * pFile)
{
    if (!IsTargetFile(pFile))
        return false;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
        (m_Attributes.Find(ATTR_OUT)))
        return true;
    if (dynamic_cast<CBEComponent*>(pFile->GetTarget()))
    {
	CBETypedDeclarator *pObj = GetObject();
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = m_Parameters.begin();
	    iter != m_Parameters.end();
	    iter++)
	{
	    /* skip CORBA_Object which has an [in] attribute */
	    if (*iter == pObj)
		continue;
            if ((*iter)->m_Attributes.Find(ATTR_IN))
                return true;
        }
    }
    return false;
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return true if should be written
 *
 * An unmarshal function is written if client's side and OUT or if component's
 * side and one of the parameters has an IN.
 */
bool 
CBEUnmarshalFunction::DoWriteFunction(CBEImplementationFile * pFile)
{
    if (!IsTargetFile(pFile))
        return false;
    if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
        (m_Attributes.Find(ATTR_OUT)))
        return true;
    if (dynamic_cast<CBEComponent*>(pFile->GetTarget()))
    {
	CBETypedDeclarator *pObj = GetObject();
	vector<CBETypedDeclarator*>::iterator iter;
	for (iter = m_Parameters.begin();
	    iter != m_Parameters.end();
	    iter++)
	{
	    /* skip CORBA_Object which has an [in] attribute */
	    if (*iter == pObj)
		continue;
            if ((*iter)->m_Attributes.Find(ATTR_IN))
                return true;
        }
    }
    return false;
}

/** \brief tries to find a parameter by its type
 *  \param sTypeName the name of the type
 *  \return a reference to the found parameter
 */
CBETypedDeclarator*
CBEUnmarshalFunction::FindParameterType(string sTypeName)
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
DIRECTION_TYPE CBEUnmarshalFunction::GetSendDirection()
{
    return IsComponentSide() ? DIRECTION_OUT : DIRECTION_IN;
}

/** \brief gets the direction of the unmarshal-parameters
 *  \return if at client's side DIRECTION_OUT, if at server's side DIRECTION_IN
 */
DIRECTION_TYPE CBEUnmarshalFunction::GetReceiveDirection()
{
    return IsComponentSide() ? DIRECTION_IN : DIRECTION_OUT;
}

/** \brief adds parameters after all other parameters
 *  \return true if successful
 */
void
CBEUnmarshalFunction::AddAfterParameters()
{
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    // create the msg buffer parameter
    CBETypedDeclarator *pParameter = pCF->GetNewTypedDeclarator();
    pParameter->SetParent(this);
    // get class' message buffer
    CBEClass *pClass = GetSpecificParent<CBEClass>();
    assert(pClass);
    CBEMsgBuffer *pSrvMsgBuffer = pClass->GetMessageBuffer();
    assert(pSrvMsgBuffer);
    string sTypeName = pSrvMsgBuffer->m_Declarators.First()->GetName();
    // write own message buffer's name
    string sName = pNF->GetMessageBufferVariable();
    // create declarator
    pParameter->CreateBackEnd(sTypeName, sName, 1);
    // set directional attribute (so HasAdditionalReference Check works)
    ATTR_TYPE nDirection = IsComponentSide() ? ATTR_IN : ATTR_OUT;
    CBEAttribute *pAttr = pCF->GetNewAttribute();
    pAttr->SetParent(pParameter);
    pAttr->CreateBackEnd(nDirection);
    pParameter->m_Attributes.Add(pAttr);
    // add it
    m_Parameters.Add(pParameter);

    // base class
    CBEOperationFunction::AddAfterParameters();
}

/** \brief get exception variable
 *  \return a reference to the exception variable
 */
CBETypedDeclarator*
CBEUnmarshalFunction::GetExceptionVariable()
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

