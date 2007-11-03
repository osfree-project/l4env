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
#include "BENameFactory.h"
#include "BEClassFactory.h"
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
void CBEUnmarshalFunction::CreateBackEnd(CFEOperation * pFEOperation, bool bComponentSide)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEUnmarshalFunction::%s(%s) called\n", __func__,
		pFEOperation->GetName().c_str());
	// set target file name
	SetTargetFileName(pFEOperation);
	SetComponentSide(bComponentSide);
	SetFunctionName(pFEOperation, FUNCTION_UNMARSHAL);

	// add parameters
	CBEOperationFunction::CreateBackEnd(pFEOperation, bComponentSide);

	// set return type
	if (IsComponentSide())
		SetNoReturnVar();
	else
	{
		// set initializer of return variable to zero
		CBETypedDeclarator *pVariable = GetReturnVariable();
		if (pVariable)
			pVariable->SetDefaultInitString(string("0"));
	}
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
void CBEUnmarshalFunction::MsgBufferInitialization(CBEMsgBuffer *pMsgBuffer)
{
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL,
		"CBEUnmarshalFunction::%s called\n", __func__);
	CBEOperationFunction::MsgBufferInitialization(pMsgBuffer);
	// check return type (do test here because sometimes we like to call
	// AddReturnVariable depending on other constraint--return is parameter)
	CBEType *pType = GetReturnType();
	assert(pType);
	if (pType->IsVoid())
		return; // having a void return type is not an error
	// add return variable
	pMsgBuffer->AddReturnVariable(this);
	CCompiler::Verbose(PROGRAM_VERBOSE_NORMAL, "%s returns true\n", __func__);
}

/** \brief writes the variable initializations of this function
 *  \param pFile the file to write to
 *
 * This implementation should initialize the pointers of the out variables. Do
 * not initialize the message buffer - this may overwrite the values we try to
 * unmarshal.
 */
void CBEUnmarshalFunction::WriteVariableInitialization(CBEFile& /*pFile*/)
{ }

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *
 * This implementation does nothing, because the unmarshalling does not
 * contain a message transfer.
 */
void CBEUnmarshalFunction::WriteInvocation(CBEFile& /*pFile*/)
{ }

/** \brief writes a single parameter for the function call
 *  \param pFile the file to write to
 *  \param pParameter the parameter to be written
 *  \param bCallFromSameClass true if called from same class
 *
 * If the parameter is the message buffer we cast to this function's type,
 * because otherwise the compiler issues warnings.
 */
void CBEUnmarshalFunction::WriteCallParameter(CBEFile& pFile,
	CBETypedDeclarator * pParameter,
	bool bCallFromSameClass)
{
	// write own message buffer's name
	CBENameFactory *pNF = CBENameFactory::Instance();
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
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEUnmarshalFunction::AddParameter(%s) called\n",
		pFEParameter->m_Declarators.First()->GetName().c_str());

	ATTR_TYPE nDirection = IsComponentSide() ? ATTR_IN : ATTR_OUT;
	if (!(pFEParameter->m_Attributes.Find(nDirection)))
		return;
	CBEOperationFunction::AddParameter(pFEParameter);
	// retrieve the parameter
	CBETypedDeclarator* pParameter = m_Parameters.Find(pFEParameter->m_Declarators.First()->GetName());
	// base class can have decided to skip parameter
	if (!pParameter)
		return;
	// skip CORBA_Object
	if (pParameter == GetObject())
		return;
	if (!pParameter->m_Attributes.Find(nDirection))
		return;

	CBEType *pType = pParameter->GetTransmitType();
	CBEDeclarator *pDeclarator = pParameter->m_Declarators.First();
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEUnmarshalFunction::AddParameter: array dims in decl %d, array dims in type %d\n",
		pDeclarator->GetArrayDimensionCount(), pType->GetArrayDimensionCount());
	int nArrayDimensions = pDeclarator->GetArrayDimensionCount() + pType->GetArrayDimensionCount();

	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG,
		"CBEUnmarshalFunction::AddParameter: decl->stars %d, is ptr type %s, array dims diff %d\n",
		pDeclarator->GetStars(), pType->IsPointerType() ? "yes" : "no", nArrayDimensions);
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
	// should not be respected when determining additional reference
	CBEAttribute *pAttr;
	if ((pAttr = pParameter->m_Attributes.Find(ATTR_SIZE_IS)) != 0)
	{
		CDeclStack vStack;
		vStack.push_back(pParameter->m_Declarators.First());
		CBENameFactory *pNF = CBENameFactory::Instance();
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

/** \brief checks if this parameter should be marshalled or not
 *  \param pParameter the parameter to check
 *  \param bMarshal true if marshaling, false if unmarshaling
 *  \return true if this parameter should be marshalled
 *
 * Always return false for marshaling, because this function does only
 * unmarshal parameters. We also have to check for the opcode at the server's
 * side, because it is unmarshaled some other place.
 */
bool CBEUnmarshalFunction::DoMarshalParameter(CBETypedDeclarator *pParameter, bool bMarshal)
{
	if (bMarshal)
		return false;

	if (!CBEOperationFunction::DoMarshalParameter(pParameter, bMarshal))
		return false;

	if (IsComponentSide())
	{
		// get opcode's name
		CBENameFactory *pNF = CBENameFactory::Instance();
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
bool CBEUnmarshalFunction::DoWriteFunction(CBEFile* pFile)
{
	if (!IsTargetFile(pFile))
		return false;
	if (pFile->IsOfFileType(FILETYPE_CLIENT) &&
		(m_Attributes.Find(ATTR_OUT)))
		return true;
	if (pFile->IsOfFileType(FILETYPE_COMPONENT))
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
CBETypedDeclarator* CBEUnmarshalFunction::FindParameterType(string sTypeName)
{
	CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
	if (pMsgBuffer && pMsgBuffer->HasType(sTypeName))
		return pMsgBuffer;
	return CBEOperationFunction::FindParameterType(sTypeName);
}

/** \brief gets the direction, which the marshal-parameters have
 *  \return if at client's side DIRECTION_IN, if at server's side DIRECTION_OUT
 *
 * Since this function ignores marshalling parameter this value should be
 * irrelevant
 */
CMsgStructType CBEUnmarshalFunction::GetSendDirection()
{
	return IsComponentSide() ? CMsgStructType::Out : CMsgStructType::In;
}

/** \brief gets the direction of the unmarshal-parameters
 *  \return if at client's side DIRECTION_OUT, if at server's side DIRECTION_IN
 */
CMsgStructType CBEUnmarshalFunction::GetReceiveDirection()
{
	return IsComponentSide() ? CMsgStructType::In : CMsgStructType::Out;
}

/** \brief adds parameters after all other parameters
 *  \return true if successful
 */
void
CBEUnmarshalFunction::AddAfterParameters()
{
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBENameFactory *pNF = CBENameFactory::Instance();
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
CBETypedDeclarator* CBEUnmarshalFunction::GetExceptionVariable()
{
	CBETypedDeclarator *pRet = CBEOperationFunction::GetExceptionVariable();
	if (pRet)
		return pRet;

	// if no parameter, then try to find it in the message buffer
	CBEClass *pClass = GetSpecificParent<CBEClass>();
	assert(pClass);
	CBEMsgBuffer *pMsgBuf = IsComponentSide() ? pClass->GetMessageBuffer() : GetMessageBuffer();
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s message buffer in class at %p\n",
		__func__, pMsgBuf);
	if (!pMsgBuf)
		return 0;
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetExceptionWordVariable();
	pRet = pMsgBuf->FindMember(sName, this, GetSendDirection());
	if (!pRet)
		pRet = pMsgBuf->FindMember(sName, this, GetReceiveDirection());
	CCompiler::Verbose(PROGRAM_VERBOSE_DEBUG, "%s exception var %s at %p\n", __func__,
		sName.c_str(), pRet);

	return pRet;
}

/** \brief writes the definition of the function to the target file
 *  \param pFile the target file to write to
 *
 * If the given file is an implementation file, we write an inline prefix.
 * This allows the target compiler to optimize cross function calls when
 * inlining the unmarshal function into the dispatcher.
 *
 * We only use "inline", because this function might be used in a derived
 * interface's dispatch function. Thus it cannot be static inline. Simple
 * inline allows the target compiler to inline it locally, that is, into the
 * same interface's dispatch function and also provide an implementation for
 * external calls. Does not work for C++.
 */
void CBEUnmarshalFunction::WriteFunctionDefinition(CBEFile& pFile)
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEUnmarshalFunction::%s(%s) in %s called\n", __func__,
		pFile.GetFileName().c_str(), GetName().c_str());

	if (pFile.IsOfFileType(FILETYPE_IMPLEMENTATION) &&
		!CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
		pFile << "\tinline" << std::endl;

	CBEOperationFunction::WriteFunctionDefinition(pFile);
}

/** \brief write the access specifier for the unmarshal function
 *  \param pFile the file to write to
 */
void CBEUnmarshalFunction::WriteAccessSpecifier(CBEHeaderFile& pFile)
{
	if (!CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
		return;

	--pFile << "\tprotected:\n";
	++pFile;
}

