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
#include "BENameFactory.h"
#include "BEClassFactory.h"
#include "fe/FEOperation.h"
#include "fe/FEIdentifier.h"
#include "TypeSpec-Type.h"
#include "Compiler.h"
#include <cassert>

CBEMarshalExceptionFunction::CBEMarshalExceptionFunction()
: CBEOperationFunction(FUNCTION_MARSHAL_EXCEPTION)
{ }

/** \brief destructor of target class */
CBEMarshalExceptionFunction::~CBEMarshalExceptionFunction()
{ }

/** \brief creates the back-end marshal function for exceptions
 *  \param pFEOperation the corresponding front-end operation
 *  \param bComponentSide true if the function is created at component side
 */
void CBEMarshalExceptionFunction::CreateBackEnd(CFEOperation * pFEOperation, bool bComponentSide)
{
	// set target file name
	SetTargetFileName(pFEOperation);
	// set function name
	SetComponentSide(bComponentSide);
	SetFunctionName(pFEOperation, FUNCTION_MARSHAL_EXCEPTION);

	// add parameters
	CBEOperationFunction::CreateBackEnd(pFEOperation, bComponentSide);

	// replace return variable. Do this before parameter initialisation, so it
	// will _not_ be added to the message buffer.
	if (IsComponentSide())
		SetNoReturnVar();

	// add message buffer
	AddMessageBuffer(pFEOperation);

	// add marshaller and communication class
	CreateMarshaller();
	CreateCommunication();
	CreateTrace();
}

/** \brief adds the parameters of a front-end function to this class
 *  \param pFEOperation the front-end function
 */
void CBEMarshalExceptionFunction::AddParameters(CFEOperation *pFEOperation)
{
	AddBeforeParameters();

	// create parameter for exception
	if (CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
	{
		for_each(pFEOperation->m_RaisesDeclarators.begin(), pFEOperation->m_RaisesDeclarators.end(),
			DoCall<CBEMarshalExceptionFunction, CFEIdentifier>(this,
				&CBEMarshalExceptionFunction::AddParameter));
	}

	AddAfterParameters();
}

/** \brief adds a single parameter to this function
 *  \param pFEIdentifier the excepition to add
 *
 * This function adds the single exceptions as parameters
 */
void CBEMarshalExceptionFunction::AddParameter(CFEIdentifier * pFEIdentifier)
{
	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBENameFactory *pNF = CBENameFactory::Instance();

	CBETypedDeclarator *pParameter = pCF->GetNewTypedDeclarator();
	pParameter->SetParent(this);
	string sExceptionName = pFEIdentifier->GetName();
	string sTypeName = pNF->GetTypeName(pFEIdentifier, sExceptionName);
	string sName = pNF->GetUserExceptionVariable(sExceptionName);
	m_Parameters.Add(pParameter);
	pParameter->CreateBackEnd(sTypeName, sName, 0);
	// create out attribute
	CBEAttribute *pAttr = pCF->GetNewAttribute();
	pAttr->CreateBackEnd(ATTR_OUT);
	pParameter->m_Attributes.Add(pAttr);
}

/** \brief add parameters after all other parameters
 *  \return true if successful
 *
 * We get the exceptions of this function and add them as parameters.
 *
 * We have to write the server's message buffer type and the local variable
 * name. We cannot use the local type, because it's not defined anywhere.
 * To write it correctly, we have to obtain the server's message buffer's
 * declarator, which is the name of the type.
 */
void CBEMarshalExceptionFunction::AddAfterParameters()
{
	CCompiler::VerboseI("CBEMarshalExceptionFunction::%s called\n", __func__);

	CBEClassFactory *pCF = CBEClassFactory::Instance();
	CBENameFactory *pNF = CBENameFactory::Instance();
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

	CCompiler::VerboseD("CBEMarshalExceptionFunction::%s returns\n", __func__);
}

/** \brief test if this function should be written
 *  \param pFile the file to write to
 *  \return true if should be written
 *
 * A marshal function is written if client's side and IN or if component's side
 * and one of the parameters has an OUT or we have an exception to transmit.
 */
bool CBEMarshalExceptionFunction::DoWriteFunction(CBEFile* pFile)
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
CMsgStructType CBEMarshalExceptionFunction::GetSendDirection()
{
	return IsComponentSide() ? CMsgStructType::Exc : CMsgStructType::In;
}

/** \brief gets the direction of the unmarshal-parameters
 *  \return if at client's side DIRECTION_OUT, if at server's side DIRECTION_IN
 */
CMsgStructType CBEMarshalExceptionFunction::GetReceiveDirection()
{
	return IsComponentSide() ? CMsgStructType::In : CMsgStructType::Exc;
}

/** \brief get exception variable
 *  \return a reference to the exception variable
 */
CBETypedDeclarator* CBEMarshalExceptionFunction::GetExceptionVariable()
{
	CBETypedDeclarator *pRet = CBEOperationFunction::GetExceptionVariable();
	if (pRet)
		return pRet;

	// if no parameter, then try to find it in the message buffer
	CBEClass *pClass = GetSpecificParent<CBEClass>();
	assert(pClass);
	CBEMsgBuffer *pMsgBuf = IsComponentSide() ? pClass->GetMessageBuffer() : GetMessageBuffer();
	CCompiler::Verbose("CBEMarshalExceptionFunction::%s message buffer in class at %p\n",
		__func__, pMsgBuf);
	if (!pMsgBuf)
		return 0;
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetExceptionWordVariable();
	pRet = pMsgBuf->FindMember(sName, this, GetSendDirection());
	if (!pRet)
		pRet = pMsgBuf->FindMember(sName, this, GetReceiveDirection());
	CCompiler::Verbose("CBEMarshalExceptionFunction::%s exception var %s at %p\n", __func__,
		sName.c_str(), pRet);

	return pRet;
}

/** \brief tries to find a parameter by its type
 *  \param sTypeName the name of the type
 *  \return a reference to the found parameter
 */
CBETypedDeclarator* CBEMarshalExceptionFunction::FindParameterType(std::string sTypeName)
{
	CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
	if (pMsgBuffer && pMsgBuffer->HasType(sTypeName))
		return pMsgBuffer;
	return CBEOperationFunction::FindParameterType(sTypeName);
}

/** \brief writes the variable initializations of this function
 *  \param pFile the file to write to
 *
 * This implementation should initialize the pointers of the out variables. Do
 * not initialize the message buffer - this may overwrite the values we try to
 * unmarshal.
 */
void CBEMarshalExceptionFunction::WriteVariableInitialization(CBEFile& /*pFile*/)
{ }

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *
 * This implementation does nothing, because the unmarshalling does not
 * contain a message transfer.
 */
void CBEMarshalExceptionFunction::WriteInvocation(CBEFile& /*pFile*/)
{ }

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
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sName = pNF->GetMessageBufferVariable();
	if (!bCallFromSameClass && pParameter->m_Declarators.Find(sName))
		pParameter->GetType()->WriteCast(pFile, pParameter->HasReference());
	// call base class
	CBEOperationFunction::WriteCallParameter(pFile, pParameter, bCallFromSameClass);
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
 * external calls.
 */
void CBEMarshalExceptionFunction::WriteFunctionDefinition(CBEFile& pFile)
{
	CCompiler::VerboseI("CBEMarshalExceptionFunction::%s(%s) in %s called\n", __func__,
		pFile.GetFileName().c_str(), GetName().c_str());

	if (pFile.IsOfFileType(FILETYPE_IMPLEMENTATION) &&
		!CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
		pFile << "\tinline" << std::endl;

	CBEOperationFunction::WriteFunctionDefinition(pFile);
}

/** \brief write the access specifier for the marshal function
 *  \param pFile the file to write to
 */
void CBEMarshalExceptionFunction::WriteAccessSpecifier(CBEHeaderFile& pFile)
{
	if (!CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
		return;

	--pFile << "\tprotected:\n";
	++pFile;
}
