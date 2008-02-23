/**
 *    \file    dice/src/be/BECallFunction.cpp
 *    \brief   contains the implementation of the class CBECallFunction
 *
 *    \date    01/18/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "BECallFunction.h"
#include "BEContext.h"
#include "BEFile.h"
#include "BEType.h"
#include "BETypedDeclarator.h"
#include "BEAttribute.h"
#include "BEHeaderFile.h"
#include "BEImplementationFile.h"
#include "BEClient.h"
#include "BEMsgBuffer.h"
#include "BEDeclarator.h"
#include "BESizes.h"
#include "Trace.h"
#include "Compiler.h"
#include "Error.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"
#include "fe/FEOperation.h"
#include <cassert>

CBECallFunction::CBECallFunction()
: CBEOperationFunction(FUNCTION_CALL)
{ }

/** \brief destructor of target class */
CBECallFunction::~CBECallFunction()
{ }

/** \brief writes the variable initializations of this function
 *  \param pFile the file to write to
 */
void
CBECallFunction::WriteVariableInitialization(CBEFile& pFile)
{
	CCompiler::Verbose("CBECallFunction::WriteVariableInitialization called %s in %s\n",
		GetName().c_str(), pFile.GetFileName().c_str());
	// init message buffer
	CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
	pMsgBuffer->WriteInitialization(pFile, this, 0, CMsgStructType::Generic);
}

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *
 * This implementation calls the underlying message trasnfer mechanisms
 */
void CBECallFunction::WriteInvocation(CBEFile& /*pFile*/)
{ }

/** \brief writes the unmarshalling of the message
 *  \param pFile the file to write to
 *
 * This implementation should unpack the out parameters from the returned
 * message structure
 */
void
CBECallFunction::WriteUnmarshalling(CBEFile& pFile)
{
	CCompiler::Verbose("CBECallFunction::%s for %s called\n", __func__,
		GetName().c_str());

	bool bLocalTrace = false;
	if (!m_bTraceOn && m_pTrace)
	{
		m_pTrace->BeforeUnmarshalling(pFile, this);
		m_bTraceOn = bLocalTrace = true;
	}

	// unmarshal exception first
	WriteMarshalException(pFile, false, true);
	// unmarshal return variable
	WriteMarshalReturn(pFile, false);

	// now unmarshal rest
	CBEOperationFunction::WriteUnmarshalling(pFile);

	if (bLocalTrace)
	{
		m_pTrace->AfterUnmarshalling(pFile, this);
		m_bTraceOn = false;
	}

	CCompiler::Verbose("CBECallFunction::%s for %s finished\n", __func__,
		GetName().c_str());
}

/** \brief creates the call function
 *  \param pFEOperation the front-end operation used as reference
 *  \param bComponentSide true if function is created at compnent side
 *
 * This implementation only sets the name of the function.
 */
void CBECallFunction::CreateBackEnd(CFEOperation * pFEOperation, bool bComponentSide)
{
	CCompiler::Verbose("CBECallFunction::%s for operation %s called\n", __func__,
		pFEOperation->GetName().c_str());

	// set target file name
	SetTargetFileName(pFEOperation);
	// set name
	SetComponentSide(bComponentSide);
	SetFunctionName(pFEOperation, FUNCTION_CALL);

	CBEOperationFunction::CreateBackEnd(pFEOperation, bComponentSide);
	// add msg buffer
	// its the last, because it needs the existing BE parameters
	AddMessageBuffer(pFEOperation);
	// then add as local variable
	AddLocalVariable(GetMessageBuffer());
	// add exception variable
	AddExceptionVariable();
	CBETypedDeclarator *pException = GetExceptionVariable();
	if (pException)
	{
		// this is a stupid trick: in order to make the marshalling logic
		// unmarshal the exception into the environment, we need a parameter
		// or local variable with that name. Even though we never use it.
		pException->AddLanguageProperty(string("attribute"),
			string("__attribute__ ((unused))"));
	}
	// add marshaller and communication class
	CreateMarshaller();
	CreateCommunication();
	CreateTrace();

	// set initializer of return variable to zero
	CBETypedDeclarator *pVariable = GetReturnVariable();
	if (pVariable)
		pVariable->SetDefaultInitString(string("0"));

	CCompiler::Verbose("CBECallFunction::%s returns true\n", __func__);
}

/** \brief manipulate the message buffer
 *  \param pMsgBuffer the message buffer to initialize
 *  \return true on success
 */
void
CBECallFunction::MsgBufferInitialization(CBEMsgBuffer *pMsgBuffer)
{
	CCompiler::Verbose("CBECallFunction::%s called\n", __func__);
	CBEOperationFunction::MsgBufferInitialization(pMsgBuffer);
	// check return type (do test here because sometimes we like to call
	// AddReturnVariable under different constraints--return parameter)
	CBEType *pType = GetReturnType();
	assert(pType);
	if (pType->IsVoid())
		return; // having a void return type is not an error
	// add return variable
	pMsgBuffer->AddReturnVariable(this);
	CCompiler::Verbose("CBECallFunction::%s returns true\n", __func__);
}

/** \brief checks if this parameter has to be marshalled or not
 *  \param pParameter the parameter to be checked
 *  \param bMarshal true if marshaling, false if unmarshaling
 *  \return true if this parameter is marshalled
 *
 * Only marshal those parameters with an IN attribute
 */
bool
CBECallFunction::DoMarshalParameter(CBETypedDeclarator *pParameter,
	bool bMarshal)
{
	CCompiler::Verbose("CBECallFunction::%s called for %s with marshal=%s and IN=%s, OUT=%s\n",
		__func__, pParameter->m_Declarators.First()->GetName().c_str(),
		(bMarshal) ? "yes" : "no",
		pParameter->m_Attributes.Find(ATTR_IN) ? "yes" : "no",
		pParameter->m_Attributes.Find(ATTR_OUT) ? "yes" : "no");
	if (!CBEOperationFunction::DoMarshalParameter(pParameter, bMarshal))
		return false;
	if (bMarshal && pParameter->m_Attributes.Find(ATTR_IN))
		return true;
	if (!bMarshal && pParameter->m_Attributes.Find(ATTR_OUT))
		return true;
	return false;
}

/** \brief checks if this function should be written
 *  \param pFile the target file to write to
 *  \return true if successful
 *
 * A call function is only written for a client file (it sould not have been
 * created if the attributes (IN,OUT) would not fit).
 *
 * A call function is not written if it has a uuid-range attribute. The the
 * call function is only used to initialize message buffers.
 */
bool CBECallFunction::DoWriteFunction(CBEFile* pFile)
{
	if (!IsTargetFile(pFile))
		return false;

	if (m_Attributes.Find(ATTR_UUID_RANGE))
		return false;

	return pFile->IsOfFileType(FILETYPE_CLIENT);
}

/** \brief calcualtes the size of this function
 *  \param nDirection the direction to calulate the size for
 *  \return the size of the function's parameters in bytes
 */
int CBECallFunction::GetSize(DIRECTION_TYPE nDirection)
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
int CBECallFunction::GetFixedSize(DIRECTION_TYPE nDirection)
{
	int nSize = CBEOperationFunction::GetFixedSize(nDirection);
	if ((nDirection & DIRECTION_IN) &&
		!m_Attributes.Find(ATTR_NOOPCODE))
		nSize += CCompiler::GetSizes()->GetOpcodeSize();
	if ((nDirection & DIRECTION_OUT) &&
		!m_Attributes.Find(ATTR_NOEXCEPTIONS))
		nSize += CCompiler::GetSizes()->GetExceptionSize();
	return nSize;
}

/** \brief writes the return type of a function
 *  \param pFile the file to write to
 *
 * For C++ we are completely virtual
 */
void
CBECallFunction::WriteReturnType(CBEFile& pFile)
{
	if (pFile.IsOfFileType(FILETYPE_HEADER) &&
		CCompiler::IsBackEndLanguageSet(PROGRAM_BE_CPP))
		pFile << "virtual ";
	CBEOperationFunction::WriteReturnType(pFile);
}
