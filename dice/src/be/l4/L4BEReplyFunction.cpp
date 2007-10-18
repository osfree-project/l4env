/**
 *    \file    dice/src/be/l4/L4BEReplyFunction.cpp
 *    \brief   contains the implementation of the class CL4BEReplyFunction
 *
 *    \date    02/07/2002
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
#include "L4BEReplyFunction.h"
#include "L4BENameFactory.h"
#include "L4BEClassFactory.h"
#include "be/l4/v2/L4V2BEIPC.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BEMsgBuffer.h"
#include "be/Trace.h"
#include "be/BESizes.h"
#include "Compiler.h"
#include "TypeSpec-L4Types.h"
#include "Attribute-Type.h"
#include <cassert>

CL4BEReplyFunction::CL4BEReplyFunction()
: CBEReplyFunction()
{ }

/** destroy the object */
CL4BEReplyFunction::~CL4BEReplyFunction()
{ }

/** \brief initializes instance of this class
 *  \param pFEOperation the front-end operation to use as reference
 *  \return true if successful
 */
void CL4BEReplyFunction::CreateBackEnd(CFEOperation *pFEOperation, bool bComponentSide)
{
	CBEReplyFunction::CreateBackEnd(pFEOperation, bComponentSide);

	string exc = string(__func__);
	// add local variables
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
	string sDope = pNF->GetTypeName(TYPE_MSGDOPE_SEND, false);
	string sCurr = sResult;
	AddLocalVariable(sDope, sResult, 0, string("{ msgdope: 0 }"));
	// we might need the offset variables if we transmit [ref]
	// attributes, because strings are found in message buffer by
	// offset calculation if message buffer is at server side.
	if (!HasVariableSizedParameters(DIRECTION_INOUT) &&
		!HasArrayParameters(DIRECTION_INOUT) &&
		FindParameterAttribute(ATTR_REF))
	{
		sCurr = pNF->GetTempOffsetVariable();
		AddLocalVariable(TYPE_INTEGER, true, 4, sCurr, 0 /*stars*/);

		sCurr = pNF->GetOffsetVariable();
		AddLocalVariable(TYPE_INTEGER, true, 4, sCurr, 0);
	}
}

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *
 * In L4 this is a send. Do not set size dope, because the size dope is set by
 * the server (wait-any function).
 */
void CL4BEReplyFunction::WriteInvocation(CBEFile& pFile)
{
	// set size and send dopes
	CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
	assert(pMsgBuffer);
	pMsgBuffer->WriteInitialization(pFile, this, TYPE_MSGDOPE_SEND,
		CMsgStructType(GetSendDirection()));

	// invocate
	WriteIPC(pFile);
	WriteIPCErrorCheck(pFile);
}


/** \brief tests if this IPC was successful
 *  \param pFile the file to write to
 *
 * The IPC error check tests the result code of the IPC, whether the reply
 * operation had any errors.
 *
 * \todo: Do we want to block the server, waiting for one client, which might
 * not respond?
 */
void CL4BEReplyFunction::WriteIPCErrorCheck(CBEFile& pFile)
{
	if (!m_sErrorFunction.empty())
	{
		CBENameFactory *pNF = CBENameFactory::Instance();
		string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);

		pFile << "\t/* test for IPC errors */\n";
		pFile << "\tif (DICE_EXPECT_FALSE(L4_IPC_IS_ERROR(" << sResult <<
			")))\n";
		++pFile << "\t" << m_sErrorFunction << "(" << sResult << ", ";
		WriteCallParameter(pFile, GetEnvironment(), true);
		pFile << ");\n";
		--pFile;
	}
}

/** \brief writes the ipc code for this function
 *  \param pFile the file to write to
 */
void CL4BEReplyFunction::WriteIPC(CBEFile& pFile)
{
	if (m_pTrace)
		m_pTrace->BeforeReplyOnly(pFile, this);

	CBECommunication *pComm = GetCommunication();
	assert(pComm);
	pComm->WriteReply(pFile, this);

	if (m_pTrace)
		m_pTrace->AfterReplyOnly(pFile, this);
}

/** \brief init message buffer size dope
 *  \param pFile the file to write to
 */
void CL4BEReplyFunction::WriteVariableInitialization(CBEFile& pFile)
{
	CBEReplyFunction::WriteVariableInitialization(pFile);
	CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
	assert(pMsgBuffer);
	pMsgBuffer->WriteInitialization(pFile, this, TYPE_MSGDOPE_SIZE,
		CMsgStructType(GetSendDirection()));
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \return the size of the parameters
 *
 * If we recv flexpages, remove the exception size again, since either the
 * flexpage or the exception is sent.
 */
int CL4BEReplyFunction::GetSize(DIRECTION_TYPE nDirection)
{
	// get base class' size
	int nSize = CBEReplyFunction::GetSize(nDirection);
	if ((nDirection & DIRECTION_OUT) &&
		!m_Attributes.Find(ATTR_NOEXCEPTIONS) &&
		(GetParameterCount(TYPE_FLEXPAGE, DIRECTION_OUT) > 0))
		nSize -= CCompiler::GetSizes()->GetExceptionSize();
	return nSize;
}

/** \brief calculates the size of the function's fixed-sized parameters
 *  \param nDirection the direction to count
 *  \return the size of the parameters
 *
 * If we recv flexpages, remove the exception size again, since either the
 * flexpage or the exception is sent.
 */
int CL4BEReplyFunction::GetFixedSize(DIRECTION_TYPE nDirection)
{
	int nSize = CBEReplyFunction::GetFixedSize(nDirection);
	if ((nDirection & DIRECTION_OUT) &&
		!m_Attributes.Find(ATTR_NOEXCEPTIONS) &&
		(GetParameterCount(TYPE_FLEXPAGE, DIRECTION_OUT) > 0))
		nSize -= CCompiler::GetSizes()->GetExceptionSize();
	return nSize;
}

