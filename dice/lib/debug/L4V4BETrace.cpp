/**
 *    \file    dice/src/be/l4/v4/L4V4BETrace.cpp
 *    \brief   contains the implementation of the class CL4V4BETrace
 *
 *    \date    12/14/2005
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

#include "L4V4BETrace.h"
#include "be/l4/v4/L4V4BENameFactory.h"
#include "be/l4/L4BEMsgBuffer.h"
#include "be/l4/L4BEMarshaller.h"
#include "be/BEFunction.h"
#include "be/BEFile.h"
#include "be/BEDeclarator.h"
#include "Compiler.h"
#include "TypeSpec-Type.h"
#include <cassert>

CL4V4BETrace::CL4V4BETrace()
{
}

CL4V4BETrace::~CL4V4BETrace()
{
}

/** \brief adds local variable needed for tracing to function
 *  \param pFunction the function to add the variables to
 */
void
CL4V4BETrace::AddLocalVariable(CBEFunction *pFunction)
{
    CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
    bool bShortIPC = false;
    if (pMsgBuffer)
	pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_SHORT_IPC, 
	    CMsgStructType::Generic);

    // write loop variable for msg buffer dump
    if (!CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF) ||
	bShortIPC)
	return;

    string sCurr = string("_i");
    pFunction->AddLocalVariable(TYPE_INTEGER, false, 4, sCurr, 0);
    CBETypedDeclarator *pVariable = pFunction->m_LocalVariables.Find(sCurr);
    pVariable->AddLanguageProperty(string("attribute"), 
	string("__attribute__ ((unused))"));
}

/** \brief prints the tracing message before a call
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void
CL4V4BETrace::BeforeCall(CBEFile *pFile,
	CBEFunction *pFunction)
{
    if (!(CCompiler::IsOptionSet(PROGRAM_TRACE_CLIENT) ||
	    CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF)) &&
	!pFunction->IsComponentSide())
	return;
    if (!(CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER) ||
	    CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF)) &&
	pFunction->IsComponentSide())
	return;

    // get the CORBA Object name
    CBETypedDeclarator *pObj = pFunction->GetObject();
    assert(pObj);
    CBEDeclarator *pObjName = pObj->m_Declarators.First();

    // get tracing function
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sMsgTag = pNF->GetString(CL4V4BENameFactory::STR_MSGTAG_VARIABLE, 0);
    string sFunc;
    CCompiler::GetBackEndOption(string("trace-client-func"), sFunc);
    
    CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
    assert(pMsgBuffer);

    if (CCompiler::IsOptionSet(PROGRAM_TRACE_CLIENT) ||
	CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
    {
	string sMWord = pNF->GetTypeName(TYPE_MWORD, false, 0);
	*pFile << "\t" << sFunc << " (\"" << pFunction->GetName() << 
	    ": server %lX (mr0 %08lx)\\n\", " << pObjName->GetName() << 
	    "->raw, ";
	pMsgBuffer->WriteAccessToStruct(pFile, pFunction, 
	    pFunction->GetSendDirection());
	*pFile << "." << sMsgTag << ".raw);\n";
    }
}

/** \brief write the tracing code after the call
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void
CL4V4BETrace::AfterCall(CBEFile *pFile,
	CBEFunction *pFunction)
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

    // get tracing function
    string sFunc;
    CCompiler::GetBackEndOption(string("trace-client-func"), sFunc);

    if (CCompiler::IsOptionSet(PROGRAM_TRACE_CLIENT) ||
	CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
    {
	CBENameFactory *pNF = CCompiler::GetNameFactory();
	string sMsgTag = pNF->GetString(CL4V4BENameFactory::STR_MSGTAG_VARIABLE, 0);
	
	*pFile << "\tif (L4_IpcFailed (" << sMsgTag << "))\n";
	pFile->IncIndent();
	*pFile << "\t" << sFunc << " (\"" << pFunction->GetName() << 
	    ": IPC error: %lx in %s\\n\", L4_ErrorCode () >> 1, " <<
	    "(L4_ErrorCode() & 1) ? \"receive\" : \"send\");\n";
	pFile->DecIndent();
    }

    if (CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
	!bIsShortIPC)
    {
	*pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
	    ": after call\\n\");\n";
	pMsgBuffer->WriteDump(pFile);
    }
}

/** \brief write the tracing code shortly before switch statement
 *  \param pFile the file to write to
 *  \param pFunction the dispatch function to write for
 */
void
CL4V4BETrace::BeforeDispatch(CBEFile *pFile,
	CBEFunction *pFunction)
{
    if (!pFunction->IsComponentSide())
	return;
    if (!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
	return;

    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sOpcodeVar = pNF->GetOpcodeVariable();
    string sObjectVar = pNF->GetCorbaObjectVariable();
    string sMsgTag = pNF->GetString(CL4V4BENameFactory::STR_MSGTAG_VARIABLE, 0);
    string sFunc;
    CCompiler::GetBackEndOption(string("trace-server-func"), sFunc);
    CMsgStructType nDirection = pFunction->GetReceiveDirection();

    *pFile << "\t" << sFunc << " (\"" << pFunction->GetName() << 
	": opcode %lx received from %lX\\n\",\n";
    pFile->IncIndent();
    *pFile << "\t" << sOpcodeVar << ",\n";
    *pFile << "\t" << sObjectVar << "->raw);\n";
    pFile->DecIndent();
    *pFile << "\t" << sFunc << " (\"" << pFunction->GetName() << 
	": received %d untyped and %d typed elements\\n\",\n";
    pFile->IncIndent();
    *pFile << "\t";
    // this writes access to the word members
    CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->WriteAccessToStruct(pFile, pFunction, nDirection);
    *pFile << "." << sMsgTag << ".X.u,\n";
    *pFile << "\t";
    pMsgBuffer->WriteAccessToStruct(pFile, pFunction, nDirection);
    *pFile << "." << sMsgTag << ".X.t);\n";
    pFile->DecIndent();
}

/** \brief write the tracing code shortly after the switch statement
 *  \param pFile the file to write to
 *  \param pFunction the dispatch function to write for
 */
void
CL4V4BETrace::AfterDispatch(CBEFile *pFile,
	CBEFunction *pFunction)
{
    if (!pFunction->IsComponentSide())
	return;
    if (!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
	return;

    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sFunc;
    CCompiler::GetBackEndOption(string("trace-server-func"), sFunc);
    string sReply = pNF->GetReplyCodeVariable();
    CMsgStructType nDirection = pFunction->GetSendDirection();
    CL4BEMarshaller *pMarshaller = static_cast<CL4BEMarshaller*>(
	pCF->GetNewMarshaller());

    *pFile << "\t" << sFunc << " (\"" << pFunction->GetName() << 
	": reply %s (dw0=%lx, dw1=%lx)\\n\",\n";
    pFile->IncIndent();
    *pFile << "\t(" << sReply << 
	"==DICE_REPLY) ?\n";
    *pFile << "\t\"DICE_REPLY\" : \"DICE_NO_REPLY\",\n";
    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 0, false,
	    false))
	*pFile << "0";
    *pFile << ",\n";
    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 1, false,
	    false))
	*pFile << "0";
    *pFile << ");\n";
    pFile->DecIndent();
}

/** \brief write the IPC Error code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void
CL4V4BETrace::WaitCommError(CBEFile *pFile,
	CBEFunction *pFunction)
{
    if (!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
	return;

    string sFunc;
    CCompiler::GetBackEndOption(string("trace-server-func"), sFunc);
    *pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
	": IPC error: %lx\\n\", L4_ErrorCode ());\n";
}

/** \brief write the tracing the code before reply IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void
CL4V4BETrace::BeforeReplyWait(CBEFile *pFile,
    CBEFunction *pFunction)
{
    if (!pFunction->IsComponentSide())
	return;
    if (!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
	return;

    CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
    assert(pMsgBuffer);
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sObjectVar = pNF->GetCorbaObjectVariable();
    string sMsgTag = pNF->GetString(CL4V4BENameFactory::STR_MSGTAG_VARIABLE, 0);
    string sFunc;
    CCompiler::GetBackEndOption(string("trace-server-func"), sFunc);
    
    *pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
	": send reply to %lX (label %08lx)\\n\", " << sObjectVar <<
	"->raw, ";
    pMsgBuffer->WriteAccessToStruct(pFile, pFunction, DIRECTION_INOUT);
    *pFile << "." << sMsgTag << ".raw);\n";
}


/** \brief writes the tracing code after the server waited for a message
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void
CL4V4BETrace::AfterReplyWait(CBEFile *pFile,
	CBEFunction *pFunction)
{
    if (!CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
	!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
	return;

    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sObjectVar = pNF->GetCorbaObjectVariable();
    string sMsgTag = pNF->GetString(CL4V4BENameFactory::STR_MSGTAG_VARIABLE, 0);
    string sFunc;
    CCompiler::GetBackEndOption(string("trace-server-func"), sFunc);

    *pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
	": received request from %lX (mr0 %08lx)\\n\", " << sObjectVar <<
	"->raw, " << sMsgTag << ".raw);\n";

    *pFile << "\tif (L4_IpcFailed (" << sMsgTag << "))\n";
    pFile->IncIndent();
    *pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
	": IPC error: %lx\\n\", L4_ErrorCode ());\n";
    pFile->DecIndent();

    *pFile << "\tif (" << sMsgTag << ".X.flags & 1)\n";
    *pFile << "\t{\n";
    pFile->IncIndent();
    *pFile << "\tL4_ThreadId_t _id = L4_ActualSender();\n";
    *pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
	": propagated, actual sender %lX\\n\", _id.raw);\n";
    pFile->DecIndent();
    *pFile << "\t}\n";
}

