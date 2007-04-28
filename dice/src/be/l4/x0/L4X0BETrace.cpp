/**
 *    \file    dice/src/be/l4/x0/L4X0BETrace.cpp
 *    \brief   contains the implementation of the class CL4X0BETrace
 *
 *    \date    12/06/2005
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

#include "L4X0BETrace.h"
#include "be/l4/L4BEMsgBuffer.h"
#include "be/l4/L4BEIPC.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEMarshaller.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEFunction.h"
#include "be/BECommunication.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BESizes.h"
#include "TypeSpec-Type.h"
#include "Compiler.h"
#include <cassert>

CL4X0BETrace::CL4X0BETrace()
{
}

CL4X0BETrace::~CL4X0BETrace()
{
}

/** \brief prints the tracing message before a call
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void
CL4X0BETrace::BeforeCall(CBEFile *pFile,
    CBEFunction *pFunction)
{
    if (!CCompiler::IsOptionSet(PROGRAM_TRACE_CLIENT) &&
	!CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER) &&
	!CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF))
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

    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CL4BEMarshaller *pMarshaller = static_cast<CL4BEMarshaller*>(
	pCF->GetNewMarshaller());

    // get tracing function
    string sFunc;
    CCompiler::GetBackEndOption(string("trace-client-func"), sFunc);

    if (CCompiler::IsOptionSet(PROGRAM_TRACE_CLIENT))
    {
	*pFile << "\t" << sFunc << " (\"" << pFunction->GetName() << 
	    ": server %2X.%X\\n\", " << pObjName->GetName() << 
	    "->id.task, " << pObjName->GetName() << "->id.lthread);\n";
	*pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
	    ": with dw0=0x%x, dw1=0x%x, dw2=0x%x\\n\", ";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir,
		0, false, false))
	    *pFile << "0";
	*pFile << ", ";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir,
		1, false, false))
	    *pFile << "0";
	*pFile << ", ";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir,
		2, false, false))
	    *pFile << "0";
	*pFile << ");\n";
    }

    if (CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
	!bIsShortIPC)
    {
	*pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
	    ": before call\\n\");\n";
	pMsgBuffer->WriteDump(pFile);
    }
}

/** \brief write the tracing code after the call
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
    void
CL4X0BETrace::AfterCall(CBEFile *pFile,
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

    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);

    // get tracing function
    string sFunc;
    CCompiler::GetBackEndOption(string("trace-client-func"), sFunc);

    if (CCompiler::IsOptionSet(PROGRAM_TRACE_CLIENT) ||
	CCompiler::IsOptionSet(PROGRAM_TRACE_SERVER))
    {
	*pFile << "\t" << sFunc << " (\"" << pFunction->GetName() << 
	    ": return dope %x (ipc error %x)\\n\", " << sResult <<
	    ".msgdope, L4_IPC_ERROR(" << sResult << "));\n";
    }

    if (CCompiler::IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
	!bIsShortIPC)
    {
	*pFile << "\t" << sFunc << " (\"" << pFunction->GetName() <<
	    ": after call\\n\");\n";
	pMsgBuffer->WriteDump(pFile);
    }
}
