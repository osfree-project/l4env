/**
 *    \file    dice/src/be/l4/v4/L4V4BEReplyFunction.cpp
 *    \brief   contains the implementation of the class CL4V4BEReplyFunction
 *
 *    \date    02/20/2008
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2008
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
#include "L4V4BEReplyFunction.h"
#include "L4V4BENameFactory.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BEUserDefinedType.h"
#include "be/BEMsgBuffer.h"
#include "be/Trace.h"
#include "be/BESizes.h"
#include "Compiler.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"
#include <cassert>

CL4V4BEReplyFunction::CL4V4BEReplyFunction()
: CL4BEReplyFunction()
{ }

/** destroy the object */
CL4V4BEReplyFunction::~CL4V4BEReplyFunction()
{ }

/** \brief initializes instance of this class
 *  \param pFEOperation the front-end operation to use as reference
 *  \param bComponentSide true if this function is created at component side
 *  \return true if successful
 */
void CL4V4BEReplyFunction::CreateBackEnd(CFEOperation *pFEOperation, bool bComponentSide)
{
	// do not call direct base class (it adds the result var only)
	CBEReplyFunction::CreateBackEnd(pFEOperation, bComponentSide);

	// add local variables
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sMsgTag = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
	string sType = pNF->GetTypeName(TYPE_MSGTAG, false);
	AddLocalVariable(sType, sMsgTag, 0, string("L4_MsgTag()"));
}

/** \brief writes the marshaling of the message
 *  \param pFile the file to write to
 *
 * Simply marshal the message. V4 specific is to put the message after
 * marshalling into the message registers.
 */
void CL4V4BEReplyFunction::WriteMarshalling(CBEFile& pFile)
{
	CL4BEReplyFunction::WriteMarshalling(pFile);

	CBENameFactory *pNF = CBENameFactory::Instance();
	string sMsgBuffer = pNF->GetMessageBufferVariable();
	// first clear mr0 in message buffer
	pFile << "\tL4_MsgClear ( (L4_Msg_t*) &" << sMsgBuffer << " );\n";
	// load msgtag into message buffer
	pFile << "\tL4_Set_MsgLabel ( (L4_Msg_t*) &" << sMsgBuffer << ", " <<
		m_sOpcodeConstName << " );\n";
	// set dopes
	CBEMsgBuffer *pMsgBuffer = GetMessageBuffer();
	assert(pMsgBuffer);
	pMsgBuffer->WriteInitialization(pFile, this, TYPE_MSGDOPE_SEND, GetSendDirection());
	// load the message into the UTCB
	pFile << "\tL4_MsgLoad ( (L4_Msg_t*) &" << sMsgBuffer << " );\n";
}

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *
 * In L4 this is a send.
 */
void CL4V4BEReplyFunction::WriteInvocation(CBEFile& pFile)
{
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
void CL4V4BEReplyFunction::WriteIPCErrorCheck(CBEFile& pFile)
{
	CBENameFactory *pNF = CBENameFactory::Instance();
	string sResult = pNF->GetString(CL4BENameFactory::STR_MSGTAG_VARIABLE, 0);
	CBEDeclarator *pDecl = GetEnvironment()->m_Declarators.First();

	pFile << "\tif (L4_IpcFailed (" << sResult << "))\n" <<
		"\t{\n";
	// env.major = CORBA_SYSTEM_EXCEPTION;
	// env.repos_id = DICE_IPC_ERROR;
	string sSetFunc;
	if (((CBEUserDefinedType*)GetEnvironment()->GetType())->GetName() ==
		"CORBA_Server_Environment")
		sSetFunc = "CORBA_server_exception_set";
	else
		sSetFunc = "CORBA_exception_set";
	++pFile << "\t" << sSetFunc << " (";
	if (pDecl->GetStars() == 0)
		pFile << "&";
	pDecl->WriteName(pFile);
	pFile << ",\n";
	++pFile << "\tCORBA_SYSTEM_EXCEPTION,\n";
	pFile << "\tCORBA_DICE_EXCEPTION_IPC_ERROR,\n";
	pFile << "\t0);\n";
	// env.ipc_error = L4_IPC_ERROR(result);
	string sEnv;
	if (pDecl->GetStars() == 0)
		sEnv = "&";
	sEnv += pDecl->GetName();
	--pFile << "\tDICE_IPC_ERROR(" << sEnv << ") = L4_ErrorCode();\n";
	// return
	WriteReturn(pFile);
	// close
	--pFile << "\t}\n";
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \return the size of the parameters
 *
 * If we recv flexpages, remove the exception size again, since either the
 * flexpage or the exception is sent.
 */
int CL4V4BEReplyFunction::GetSize(DIRECTION_TYPE nDirection)
{
	// get base class' size
	int nSize = CL4BEReplyFunction::GetSize(nDirection);
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
int CL4V4BEReplyFunction::GetFixedSize(DIRECTION_TYPE nDirection)
{
	int nSize = CL4BEReplyFunction::GetFixedSize(nDirection);
	if ((nDirection & DIRECTION_OUT) &&
		!m_Attributes.Find(ATTR_NOEXCEPTIONS) &&
		(GetParameterCount(TYPE_FLEXPAGE, DIRECTION_OUT) > 0))
		nSize -= CCompiler::GetSizes()->GetExceptionSize();
	return nSize;
}

