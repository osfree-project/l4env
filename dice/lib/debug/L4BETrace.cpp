/**
 *    \file    dice/src/be/l4/L4BETrace.cpp
 *    \brief   contains the implementation of the class CL4BETrace
 *
 *    \date    12/05/2005
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2005-2007
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

#include "L4BETrace.h"
#include "be/l4/L4BEMsgBuffer.h"
#include "be/l4/L4BEIPC.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEMarshaller.h"
#include "be/l4/TypeSpec-L4Types.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEFunction.h"
#include "be/BECommunication.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BESizes.h"
#include "be/BEClass.h"
#include "Compiler.h"
#include <cassert>

CL4BETrace::CL4BETrace()
{ }

CL4BETrace::~CL4BETrace()
{ }

/** \brief adds local variable needed for tracing to function
 *  \param pFunction the function to add the variables to
 */
void CL4BETrace::AddLocalVariable(CBEFunction *pFunction)
{
	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	bool bShortIPC = false;
	if (pMsgBuffer)
		bShortIPC = pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_SHORT_IPC, CMsgStructType::Generic);

	// write loop variable for msg buffer dump
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF) ||
		bShortIPC)
		return;

	string sCurr = string("_i");
	pFunction->AddLocalVariable(TYPE_INTEGER, false, 4, sCurr, 0);
	CBETypedDeclarator *pVariable = pFunction->m_LocalVariables.Find(sCurr);
	pVariable->AddLanguageProperty("attribute",
		"__attribute__ ((unused))");
}

/** \brief write the code required just before the loop starts
 *  \param pFile the file to write to
 *  \param pFunction the server loop function calling this hook
 *
 * This is different from InitServer, because InitServer is called before all
 * the variables are initialized. BeforeLoop is called after all the variables
 * are initialized and just before the loop actually starts doing something.
 */
void CL4BETrace::BeforeLoop(CBEFile& pFile, CBEFunction* pFunction)
{
	if (!pFunction->IsComponentSide())
		return;
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
		return;

	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	assert(pMsgBuffer);
	// get tracing function
	string sFunc;
	CCompiler::GetBackEndOption("trace-server-func", sFunc);

	pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
		": _size(%d,%d)\\n\", ";
	++pFile << "\t";
	pMsgBuffer->WriteMemberAccess(pFile, pFunction, CMsgStructType::Generic,
		TYPE_MSGDOPE_SIZE, 0);
	pFile << ".md.dwords, ";
	pMsgBuffer->WriteMemberAccess(pFile, pFunction, CMsgStructType::Generic,
		TYPE_MSGDOPE_SIZE, 0);
	pFile << ".md.strings);\n";
	--pFile;
}

/** \brief prints the tracing message before a call
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CL4BETrace::BeforeCall(CBEFile& pFile, CBEFunction *pFunction)
{
	if (!(CCompiler::IsOptionSet(PROGRAM_TRACE_CLIENT) ||
			CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF)) &&
		!pFunction->IsComponentSide())
		return;
	if (!(CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER) ||
			CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF)) &&
		pFunction->IsComponentSide())
		return;

	CMsgStructType nRcvDir = pFunction->GetReceiveDirection();
	CMsgStructType nSndDir = pFunction->GetSendDirection();
	// check if we send a short IPC
	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	assert(pMsgBuffer);
	bool bIsShortIPC =
		pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_SHORT_IPC, nSndDir) &&
		pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_SHORT_IPC, nRcvDir);
	// check if we use assembler for call
	CBECommunication *pComm = pFunction->GetCommunication();
	assert(pComm);
	// get the CORBA Object name
	CBETypedDeclarator *pObj = pFunction->GetObject();
	assert(pObj);
	CBEDeclarator *pObjName = pObj->m_Declarators.First();

	CBENameFactory *pNF = CCompiler::GetNameFactory();
	CBEClassFactory *pCF = CCompiler::GetClassFactory();

	// get tracing function
	string sFunc;
	CCompiler::GetBackEndOption("trace-client-func", sFunc);

	if (CCompiler::IsOptionSet(PROGRAM_TRACE_CLIENT) ||
		CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
	{
		string sMWord = pNF->GetTypeName(TYPE_MWORD, false, 0);
		pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
			": server %2X.%X\\n\", " << pObjName->GetName() <<
			"->id.task, " << pObjName->GetName() << "->id.lthread);\n";

		pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
			": with dw0=0x%x, dw1=0x%x\\n\", ";
		++pFile << "\t(unsigned int)";
		CL4BEMarshaller *pMarshaller = static_cast<CL4BEMarshaller*>(
			pCF->GetNewMarshaller());
		if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir,
				0, false, false))
			pFile << "0";
		pFile << ",\n";
		pFile << "\t(unsigned int)";
		if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir,
				1, false, false))
			pFile << "0";
		pFile << ");\n";

		--pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
			": _size(%d,%d), _send(%d,%d)\\n\", ";
		++pFile << "\t";
		pMsgBuffer->WriteMemberAccess(pFile, pFunction, nSndDir, TYPE_MSGDOPE_SIZE, 0);
		pFile << ".md.dwords, ";
		pMsgBuffer->WriteMemberAccess(pFile, pFunction, nSndDir, TYPE_MSGDOPE_SIZE, 0);
		pFile << ".md.strings,\n";
		pFile << "\t";
		pMsgBuffer->WriteMemberAccess(pFile, pFunction, nSndDir, TYPE_MSGDOPE_SEND, 0);
		pFile << ".md.dwords, ";
		pMsgBuffer->WriteMemberAccess(pFile, pFunction, nSndDir, TYPE_MSGDOPE_SEND, 0);
		pFile << ".md.strings);\n";
		--pFile;
	}

	if (CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
		!bIsShortIPC)
	{
		pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
			": before call\\n\");\n";
		pMsgBuffer->WriteDump(pFile);
	}
}

/** \brief write the tracing code after the call
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CL4BETrace::AfterCall(CBEFile& pFile, CBEFunction *pFunction)
{
	if (!(CCompiler::IsOptionSet(PROGRAM_TRACE_CLIENT) ||
			CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF)) &&
		!pFunction->IsComponentSide())
		return;
	if (!(CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER) ||
			CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF)) &&
		pFunction->IsComponentSide())
		return;

	CMsgStructType nRcvDir = pFunction->GetReceiveDirection();
	CMsgStructType nSndDir = pFunction->GetSendDirection();
	// check if we send a short IPC
	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	assert(pMsgBuffer);
	bool bIsShortIPC =
		pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_SHORT_IPC, nSndDir) &&
		pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_SHORT_IPC, nRcvDir);

	CBENameFactory *pNF = CCompiler::GetNameFactory();
	string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);

	// get tracing function
	string sFunc;
	CCompiler::GetBackEndOption("trace-client-func", sFunc);

	if (CCompiler::IsOptionSet(PROGRAM_TRACE_CLIENT) ||
		CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
	{
		pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
			": return dope %lx (ipc error %lx)\\n\",\n";
		pFile << "\t\t" << sResult << ".msgdope, L4_IPC_ERROR("
			<< sResult << "));\n";
	}

	if (CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
		!bIsShortIPC)
	{
		pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
			": after call\\n\");\n";
		pMsgBuffer->WriteDump(pFile);
	}
}

/** \brief write the tracing code shortly before switch statement
 *  \param pFile the file to write to
 *  \param pFunction the dispatch function to write for
 */
void CL4BETrace::BeforeDispatch(CBEFile& pFile, CBEFunction *pFunction)
{
	if (!pFunction->IsComponentSide())
		return;
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
		return;

	CBEClassFactory *pCF = CCompiler::GetClassFactory();
	CBENameFactory *pNF = CCompiler::GetNameFactory();
	string sOpcodeVar = pNF->GetOpcodeVariable();
	string sObjectVar = pNF->GetCorbaObjectVariable();
	string sFunc;
	CCompiler::GetBackEndOption("trace-server-func", sFunc);
	CMsgStructType nDirection = pFunction->GetReceiveDirection();
	CL4BEMarshaller *pMarshaller = static_cast<CL4BEMarshaller*>(
		pCF->GetNewMarshaller());

	pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
		": opcode %lx received from %2X.%X\\n\", " << sOpcodeVar <<
		", " << sObjectVar << "->id.task, " << sObjectVar << "->id.lthread);\n";
	pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
		": received dw0=%lx, dw1=%lx\\n\", " << "(unsigned long)";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 0,
			false, false))
		pFile << "0";
	pFile << ", (unsigned long)";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 1,
			false, false))
		pFile << "0";
	pFile << ");\n";
}

/** \brief write the tracing code shortly after the switch statement
 *  \param pFile the file to write to
 *  \param pFunction the dispatch function to write for
 */
void CL4BETrace::AfterDispatch(CBEFile& pFile, CBEFunction *pFunction)
{
	if (!pFunction->IsComponentSide())
		return;
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
		return;

	CBEClassFactory *pCF = CCompiler::GetClassFactory();
	CBENameFactory *pNF = CCompiler::GetNameFactory();
	string sFunc;
	CCompiler::GetBackEndOption("trace-server-func", sFunc);
	string sReply = pNF->GetReplyCodeVariable();
	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	assert(pMsgBuffer);
	CMsgStructType nDirection = pFunction->GetSendDirection();
	CL4BEMarshaller *pMarshaller = static_cast<CL4BEMarshaller*>(
		pCF->GetNewMarshaller());

	pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
		": reply %s (dw0=%lx, dw1=%lx)\\n\", (" <<
		sReply << "==DICE_REPLY)?\"DICE_REPLY\":\"DICE_NO_REPLY\", " <<
		"(unsigned long)";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 0,
			false, false))
		pFile << "0";
	pFile << ", (unsigned long)";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 1,
			false, false))
		pFile << "0";
	pFile << " );\n";
	// print if we got an fpage
	pFile << "\t" << sFunc << " (\"  fpage: %s\\n\", (";
	pMsgBuffer->WriteMemberAccess(pFile, pFunction, nDirection,
		TYPE_MSGDOPE_SEND, 0);
	pFile << ".md.fpage_received==1)?\"yes\":\"no\");\n";
}

/** \brief write the tracing the code before reply IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CL4BETrace::BeforeReplyWait(CBEFile& pFile, CBEFunction *pFunction)
{
	if (!pFunction->IsComponentSide())
		return;
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
		return;

	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	assert(pMsgBuffer);
	CBEClassFactory *pCF = CCompiler::GetClassFactory();
	CBENameFactory *pNF = CCompiler::GetNameFactory();
	string sMWord = pNF->GetTypeName(TYPE_MWORD, false);
	string sFunc;
	CCompiler::GetBackEndOption("trace-server-func", sFunc);
	CL4BEMarshaller *pMarshaller = static_cast<CL4BEMarshaller*>(
		pCF->GetNewMarshaller());

	pFile << "\t" << sFunc << " (\"reply (dw0=%lx, dw1=%lx)\\n\", ";
	pFile << "(unsigned long)";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, CMsgStructType::Generic, 0, false,
			false))
		pFile << "0";
	pFile << ", (unsigned long)";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, CMsgStructType::Generic, 1, false,
			false))
		pFile << "0";
	pFile << ");\n";
	// dwords
	pFile << "\t" << sFunc << " (\"  words: %d\\n\", ";
	pMsgBuffer->WriteMemberAccess(pFile, pFunction, CMsgStructType::Out,
		TYPE_MSGDOPE_SEND, 0);
	pFile << ".md.dwords);\n";
	// strings
	pFile << "\t" << sFunc << " (\"  strings; %d\\n\", ";
	pMsgBuffer->WriteMemberAccess(pFile, pFunction, CMsgStructType::Out,
		TYPE_MSGDOPE_SEND, 0);
	pFile << ".md.strings);\n";
	// print if we got an fpage
	pFile << "\t" << sFunc << " (\"  fpage: %s\\n\", (";
	pMsgBuffer->WriteMemberAccess(pFile, pFunction, CMsgStructType::Out,
		TYPE_MSGDOPE_SEND, 0);
	pFile << ".md.fpage_received==1)?\"yes\":\"no\");\n";
}

/** \brief writes the tracing code after the server waited for a message
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CL4BETrace::AfterReplyWait(CBEFile& pFile, CBEFunction *pFunction)
{
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF))
		return;

	CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
	assert(pMsgBuffer);
	CBENameFactory *pNF = CCompiler::GetNameFactory();
	string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
	// get tracing function
	string sFunc;
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
		CCompiler::GetBackEndOption("trace-client-func", sFunc);
	else
		CCompiler::GetBackEndOption("trace-server-func", sFunc);

	if (CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF))
	{
		pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
			": after wait\\n\");\n";
		pMsgBuffer->WriteDump(pFile);
	}

	if (CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
	{
		pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
			": return dope %lx (ipc error %lx)\\n\",\n";
		pFile << "\t\t" << sResult << ".msgdope, L4_IPC_ERROR("
			<< sResult << "));\n";
	}
}

/** \brief write the IPC Error code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void CL4BETrace::WaitCommError(CBEFile& pFile, CBEFunction *pFunction)
{
	if (!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
		return;

	CBETypedDeclarator *pReturn = pFunction->GetReturnVariable();
	string sTraceFunc;
	CCompiler::GetBackEndOption("trace-server-func", sTraceFunc);
	CBENameFactory *pNF = CCompiler::GetNameFactory();
	string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);

	pFile << "\t" << sTraceFunc <<
		" (\"IPC Error in server: 0x%02lx\\n\", L4_IPC_ERROR(" <<
		sResult << "));\n";
	pFile << "\t" << sTraceFunc <<
		" (\"  reply-wait returns with opcode %lx\\n\", ";
	pReturn->m_Declarators.First()->WriteName(pFile);
	pFile << ");\n";

	// set exception if not set already
	CBETypedDeclarator *pEnv = pFunction->GetEnvironment();
	CBEDeclarator *pDecl = pEnv->m_Declarators.First();
	string sEnv;
	if (pDecl->GetStars() == 0)
		sEnv = "&";
	sEnv += pDecl->GetName();
	pFile << "\tif (DICE_IS_NO_EXCEPTION(" << sEnv << "))\n";
	++pFile << "\t" << sTraceFunc <<
		" (\"  no exception set, so set internal IPC error\\n\");\n";
	--pFile << "\telse\n";
	++pFile << "\t" << sTraceFunc << " (\"  exception already set\\n\");\n";
	--pFile;
}

