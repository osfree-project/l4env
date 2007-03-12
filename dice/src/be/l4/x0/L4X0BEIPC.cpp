/**
 *  \file    dice/src/be/l4/x0/L4X0BEIPC.cpp
 *  \brief   contains the declaration of the class CL4X0BEIPC
 *
 *  \date    08/13/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2004
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
#include "be/l4/x0/L4X0BEIPC.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEMsgBuffer.h"
#include "be/l4/L4BEMarshaller.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEMarshalFunction.h"
#include "be/BEUnmarshalFunction.h"
#include "be/BEReplyFunction.h"
#include "be/BESndFunction.h"
#include "be/BEWaitFunction.h"
#include "be/BECallFunction.h"
#include "be/BEWaitAnyFunction.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BEMsgBuffer.h"
#include "TypeSpec-Type.h"
#include "Compiler.h"
#include <cassert>

CL4X0BEIPC::CL4X0BEIPC()
 : CL4BEIPC()
{
}

/** destroys the IPC object */
CL4X0BEIPC::~CL4X0BEIPC()
{
}

/** \brief writes the IPC call
 *  \param pFile the file to write to
 *  \param pFunction the function to write for

 */
void 
CL4X0BEIPC::WriteCall(CBEFile* pFile,  
	CBEFunction* pFunction)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sServerID = pNF->GetComponentIDVariable();
    string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
    string sTimeout = pNF->GetTimeoutClientVariable(pFunction);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true);
    string sMsgBuffer = pNF->GetMessageBufferVariable();
    string sDummy = pNF->GetDummyVariable();
    CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
    int nSendDir = pFunction->GetSendDirection();
    int nRecvDir = pFunction->GetReceiveDirection();
    bool bShortSend = 
	pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_SHORT_IPC, nSendDir);
    bool bShortRecv = 
	pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_SHORT_IPC, nRecvDir);
    CL4BEMarshaller *pMarshaller = 
	dynamic_cast<CL4BEMarshaller*>(pFunction->GetMarshaller());
    assert(pMarshaller);

    *pFile << "\tl4_ipc_call_w3(*" << sServerID << ",\n";
    pFile->IncIndent();
    *pFile << "\t";
    if (bShortSend)
	*pFile << "L4_IPC_SHORT_MSG";
    else
    {
        if (pFunction->GetParameterCount(TYPE_FLEXPAGE, nSendDir) > 0)
	    *pFile << "(" << sMWord << "*)((" << sMWord << ")";

        if (!pMsgBuffer->HasReference())
	    *pFile << "&";
	*pFile << sMsgBuffer;

        if (pFunction->GetParameterCount(TYPE_FLEXPAGE, nSendDir) > 0)
	    *pFile << "|2)";
    }
    *pFile << ",\n";

    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSendDir, 0, false,
	    false))
	*pFile << "0";
    *pFile << ",\n";
    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSendDir, 1, false,
	    false))
	*pFile << "0";
    *pFile << ",\n";
    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSendDir, 2, false,
	    false))
	*pFile << "0";
    *pFile << ",\n";

    if (bShortRecv)
	*pFile << "\tL4_IPC_SHORT_MSG,\n";
    else
    {
	*pFile << "\t";
        if (!pMsgBuffer->HasReference())
	    *pFile << "&";
	*pFile << sMsgBuffer << ",\n";
    }

    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nRecvDir, 0, true,
	    false))
	*pFile << "&" << sDummy;
    *pFile << ",\n";
    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nRecvDir, 1, true,
	    false))
	*pFile << "&" << sDummy;
    *pFile << ",\n";
    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nRecvDir, 2, true,
	    false))
	*pFile << "&" << sDummy;
    *pFile << ",\n";

    *pFile << "\t" << sTimeout << ", &" << sResult << ");\n";

    pFile->DecIndent();
}

/** \brief writes the IPC receive
 *  \param pFile the file to write to
 *  \param pFunction the function to write for

 */
void 
CL4X0BEIPC::WriteReceive(CBEFile* pFile, 
	CBEFunction* pFunction)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sServerID = pNF->GetComponentIDVariable();
    string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
    string sTimeout;
    if (pFunction->IsComponentSide())
        sTimeout = pNF->GetTimeoutServerVariable(pFunction);
    else
        sTimeout = pNF->GetTimeoutClientVariable(pFunction);
    string sMsgBuffer = pNF->GetMessageBufferVariable();
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true);
    string sDummy = pNF->GetDummyVariable();
    CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
    CL4BEMarshaller *pMarshaller = 
	dynamic_cast<CL4BEMarshaller*>(pFunction->GetMarshaller());
    assert(pMarshaller);
    int nDirection = pFunction->GetReceiveDirection();

    *pFile << "\tl4_ipc_receive_w3(*" << sServerID << ",\n";
    pFile->IncIndent();

    *pFile << "\t";
    if (pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_SHORT_IPC, DIRECTION_OUT))
	*pFile << "L4_IPC_SHORT_MSG";
    else
    {
        if (!pMsgBuffer->HasReference())
	    *pFile << "&";
	*pFile << sMsgBuffer;
    }
    *pFile << ",\n";

    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 0, true,
	    false))
	*pFile << "&" << sDummy;
    *pFile << ",\n";
    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 1, true,
	    false))
	*pFile << "&" << sDummy;
    *pFile << ",\n";
    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 2, true,
	    false))
	*pFile << "&" << sDummy;
    *pFile << ",\n";

    *pFile << "\t" << sTimeout << ", &" << sResult << ");\n";

    pFile->DecIndent();
}

/** \brief writes the IPC reply-and-wait
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bSendFlexpage true if a flexpage is sent
 *  \param bSendShortIPC true if a short IPC is sent

 */
void 
CL4X0BEIPC::WriteReplyAndWait(CBEFile* pFile,  
	CBEFunction* pFunction,  
	bool bSendFlexpage,  
	bool bSendShortIPC)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
    string sTimeout;
    if (pFunction->IsComponentSide())
        sTimeout = pNF->GetTimeoutServerVariable(pFunction);
    else
        sTimeout = pNF->GetTimeoutClientVariable(pFunction);
    string sServerID = pNF->GetComponentIDVariable();
    string sMsgBuffer = pNF->GetMessageBufferVariable();
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true);
    string sDummy = pNF->GetDummyVariable();
    CL4BEMarshaller *pMarshaller = 
	dynamic_cast<CL4BEMarshaller*>(pFunction->GetMarshaller());
    assert(pMarshaller);
    int nSendDir = pFunction->GetSendDirection();
    int nRecvDir = pFunction->GetReceiveDirection();

    *pFile << "\tl4_ipc_reply_and_wait_w3(*" << sServerID << ",\n";
    pFile->IncIndent();
    *pFile << "\t";
    if (bSendFlexpage)
	*pFile << "(" << sMWord << "*)((" << sMWord << ")";
    if (bSendShortIPC)
	*pFile << "L4_IPC_SHORT_MSG";
    else
	*pFile << sMsgBuffer;
    if (bSendFlexpage)
	*pFile << "|2)";
    *pFile << ",\n";

    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSendDir, 0, false,
	    false))
	*pFile << "0";
    *pFile << ",\n";
    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSendDir, 1, false,
	    false))
	*pFile << "0";
    *pFile << ",\n";
    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSendDir, 2, false,
	    false))
	*pFile << "0";
    *pFile << ",\n";

    *pFile << "\t" << sServerID << ",\n";
    *pFile << sMsgBuffer << ",\n";

    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nRecvDir, 0, true,
	    false))
	*pFile << "&" << sDummy;
    *pFile << ",\n";
    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nRecvDir, 1, true,
	    false))
	*pFile << "&" << sDummy;
    *pFile << ",\n";
    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nRecvDir, 2, true,
	    false))
	*pFile << "&" << sDummy;
    *pFile << ",\n";

    *pFile << "\t" << sTimeout << ", &" << sResult << ");\n";

    pFile->DecIndent();
}

/** \brief writes the IPC send
 *  \param pFile the file to write to
 *  \param pFunction the function to write for

 */
void 
CL4X0BEIPC::WriteSend(CBEFile* pFile,
	CBEFunction* pFunction)
{
    int nDirection = pFunction->GetSendDirection();
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sServerID = pNF->GetComponentIDVariable();
    string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
    string sTimeout;
    if (pFunction->IsComponentSide())
        sTimeout = pNF->GetTimeoutServerVariable(pFunction);
    else
        sTimeout = pNF->GetTimeoutClientVariable(pFunction);
    string sMsgBuffer = pNF->GetMessageBufferVariable();
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true);
    CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
    bool bVarBuffer = pMsgBuffer->IsVariableSized(0) ||
                     (pMsgBuffer->m_Declarators.First()->GetStars() > 0);
    bool bShortIPC = 
	pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_SHORT_IPC, nDirection);
    CL4BEMarshaller *pMarshaller = 
	dynamic_cast<CL4BEMarshaller*>(pFunction->GetMarshaller());
    assert(pMarshaller);

    *pFile << "\tl4_ipc_send_w3(*" << sServerID << ",\n";
    pFile->IncIndent();
    *pFile << "\t";
    if (!bShortIPC && 
	    (pFunction->GetParameterCount(TYPE_FLEXPAGE, nDirection) > 0))
	*pFile << "(" << sMWord << "*)((" << sMWord << ")";
    if (bShortIPC)
	*pFile << "L4_IPC_SHORT_MSG";
    else
    {
        if (!bVarBuffer)
	    *pFile << "&";
	*pFile << sMsgBuffer;
    }
    if (!bShortIPC && 
	    (pFunction->GetParameterCount(TYPE_FLEXPAGE, nDirection) > 0))
	*pFile << ")|2)";
    *pFile << ",\n";

    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 0, false,
	    false))
	*pFile << "0";
    *pFile << ",\n";
    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 1, false,
	    false))
	*pFile << "0";
    *pFile << ",\n";
    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 2, false,
	    false))
	*pFile << "0";
    *pFile << ",\n";

    *pFile << "\t" << sTimeout << ", &" << sResult << ");\n";

    pFile->DecIndent();
}

/** \brief writes the IPC wait
 *  \param pFile the file to write to
 *  \param pFunction the function to write for

 */
void 
CL4X0BEIPC::WriteWait(CBEFile* pFile,
	CBEFunction* pFunction)
{
    CBENameFactory *pNF = CCompiler::GetNameFactory();
    string sServerID = pNF->GetComponentIDVariable();
    string sResult = pNF->GetString(CL4BENameFactory::STR_RESULT_VAR);
    string sTimeout;
    if (pFunction->IsComponentSide())
        sTimeout = pNF->GetTimeoutServerVariable(pFunction);
    else
        sTimeout = pNF->GetTimeoutClientVariable(pFunction);
    string sMsgBuffer = pNF->GetMessageBufferVariable();
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true);
    string sDummy = pNF->GetDummyVariable();
    int nDirection = pFunction->GetReceiveDirection();
    CBEMsgBuffer *pMsgBuffer = pFunction->GetMessageBuffer();
    bool bVarBuffer = pMsgBuffer->IsVariableSized(0) ||
        (pMsgBuffer->m_Declarators.First()->GetStars() > 0);
    bool bShortIPC = 
	pMsgBuffer->HasProperty(CL4BEMsgBuffer::MSGBUF_PROP_SHORT_IPC, nDirection);
    CL4BEMarshaller *pMarshaller = 
	dynamic_cast<CL4BEMarshaller*>(pFunction->GetMarshaller());
    assert(pMarshaller);

    *pFile << "\tl4_ipc_wait_w3(" << sServerID << ",\n";
    pFile->IncIndent();
    *pFile << "\t";
    if (bShortIPC)
	*pFile << "L4_IPC_SHORT_MSG";
    else
    {
        if (!bVarBuffer)
	    *pFile << "&";
	*pFile << sMsgBuffer;
    }
    *pFile << ",\n";
   
    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 0, true,
	    false))
	*pFile << "&" << sDummy;
    *pFile << ",\n";
    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 1, true,
	    false))
	*pFile << "&" << sDummy;
    *pFile << ",\n";
    *pFile << "\t";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nDirection, 2, true,
	    false))
	*pFile << "&" << sDummy;
    *pFile << ",\n";

    *pFile << "\t" << sTimeout << ", &" << sResult << ");\n";
    
    pFile->DecIndent();
}

/** \brief add local variables required in functions
 *  \param pFunction the function to add the local variables to
 *  \return true if successful
 */
bool 
CL4X0BEIPC::AddLocalVariable(CBEFunction *pFunction)
{
    int nSndDir = pFunction->GetSendDirection();

    CBENameFactory *pNF = CCompiler::GetNameFactory();
    assert(pFunction);

    // temp offset and offset variable
    if (dynamic_cast<CBEMarshalFunction*>(pFunction) ||
        dynamic_cast<CBEUnmarshalFunction*>(pFunction) ||
        dynamic_cast<CBEReplyFunction*>(pFunction) ||
        dynamic_cast<CBESndFunction*>(pFunction) ||
        dynamic_cast<CBEWaitFunction*>(pFunction))
    {
        // check for temp
        if (pFunction->HasVariableSizedParameters(nSndDir) ||
            pFunction->HasArrayParameters(nSndDir))
        {
	    try
	    {
		string sTmpVar = pNF->GetTempOffsetVariable();
		pFunction->AddLocalVariable(TYPE_INTEGER, true, 4, sTmpVar, 
		    0);
		CBETypedDeclarator *pVariable = 
		    pFunction->m_LocalVariables.Find(sTmpVar);
		pVariable->AddLanguageProperty(string("attribute"), 
		    string("__attribute__ ((unused))"));

		string sOffsetVar = pNF->GetOffsetVariable();
		pFunction->AddLocalVariable(TYPE_INTEGER, true, 4, 
		    sOffsetVar, 0);
		pVariable = pFunction->m_LocalVariables.Find(sOffsetVar);
		pVariable->AddLanguageProperty(string("attribute"), 
		    string("__attribute__ ((unused))"));
	    }
	    catch (CBECreateException *e)
	    {
		e->Print();
		delete e;
		return false;
	    }
        }
    }

    // Dummy Variable
    if (dynamic_cast<CBECallFunction*>(pFunction) ||
        dynamic_cast<CBEWaitAnyFunction*>(pFunction) ||
        dynamic_cast<CBEWaitFunction*>(pFunction) ||
        dynamic_cast<CBEReplyFunction*>(pFunction) ||
        dynamic_cast<CBESndFunction*>(pFunction))
    {
	// depends on the availability of enough members for registers or
	// parameters of IPC call
	CL4BEMsgBuffer *pMsgBuffer = dynamic_cast<CL4BEMsgBuffer*>
	    (pFunction->GetMessageBuffer());
	assert(pMsgBuffer);
	int nRcvDir = pFunction->GetReceiveDirection();
	// interface functions use generic struct, instead of using dummys
	bool bUseDummy = dynamic_cast<CBEOperationFunction*>(pFunction) &&
	    !pMsgBuffer->HasWordMembers(pFunction, nRcvDir);
        // should not depend on DEFINES
        if (bUseDummy)
        {
            string sDummy = pNF->GetDummyVariable();
	    try
	    {
		pFunction->AddLocalVariable(TYPE_MWORD, false, 0, sDummy, 
		    0);
	    }
	    catch (CBECreateException *e)
	    {
		e->Print();
		delete e;
                return false;
	    }
            CBETypedDeclarator *pVariable = 
		pFunction->m_LocalVariables.Find(sDummy);
            pVariable->AddLanguageProperty(string("attribute"), 
		    string("__attribute__ ((unused))"));
        }
    }

    return true;
}

/** \brief writes a reply
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 */
void
CL4X0BEIPC::WriteReply(CBEFile* pFile,
    CBEFunction* pFunction)
{
    WriteSend(pFile, pFunction);
}

/** \brief writes the initialization
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 */
void
CL4X0BEIPC::WriteInitialization(CBEFile* /*pFile*/, 
    CBEFunction* /*pFunction*/)
{}

/** \brief writes the assigning of a local name to a communication port
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 */
void
CL4X0BEIPC::WriteBind(CBEFile* /*pFile*/, 
    CBEFunction* /*pFunction*/)
{}

/** \brief writes the initialization
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 */
void
CL4X0BEIPC::WriteCleanup(CBEFile* /*pFile*/, 
    CBEFunction* /*pFunction*/)
{}

