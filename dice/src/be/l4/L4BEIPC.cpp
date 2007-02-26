/**
 *    \file    dice/src/be/l4/L4BEIPC.cpp
 *    \brief   contains the implementation of the class CL4BEIPC
 *
 *    \date    02/25/2003
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "be/l4/L4BEIPC.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEFunction.h"
#include "be/BEDeclarator.h"

#include "TypeSpec-Type.h"
#include "Attribute-Type.h"

CL4BEIPC::CL4BEIPC()
{
}

/**    \brief destructor of target class */
CL4BEIPC::~CL4BEIPC()
{
}

/** \brief write an IPC call
 *  \param pFile the file to write to
 *  \param pFunction the function to write it for
 *  \param pContext the context of the write operation
 */
void 
CL4BEIPC::WriteCall(CBEFile *pFile, 
    CBEFunction* pFunction, 
    CBEContext *pContext)
{
    CBENameFactory *pNF = pContext->GetNameFactory();
    string sServerID = pNF->GetComponentIDVariable(pContext);
    string sResult = pNF->GetString(STR_RESULT_VAR, pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sScheduling = pNF->GetScheduleClientVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    assert(pMsgBuffer);
    int nDirection = pFunction->GetSendDirection();
    bool bScheduling = pFunction->FindAttribute(ATTR_L4_SCHED_DECEIT); 
    /* OR further attributes */

    // XXX FIXME:
    // not implemented, because X0 adaption has no 3 word bindings
    // CL4BESizes *pSizes = (CL4BESizes*)pContext->GetSizes();
    // bool bWord3 = (pSizes->GetMaxShortIPCSize(DIRECTION_IN) / pSizes->GetSizeOfType(TYPE_MWORD)) == 3;
    // if (bWord3)
    //   pFile->PrintIndent("l4_ipc_call_w3(*%s,\n");
    // else
    //   pFile->PrintIndent("l4_ipc_call(*%s,\n");

    pFile->PrintIndent("l4_ipc_call(*%s,\n", sServerID.c_str());
    pFile->IncIndent();
    pFile->PrintIndent("");
    if (IsShortIPC(pFunction, pContext, nDirection))
    {
        pFile->Print("L4_IPC_SHORT_MSG");
        if (bScheduling)
            *pFile << " | " << sScheduling;
    }
    else
    {
        if ((pMsgBuffer->GetCount(TYPE_FLEXPAGE, nDirection) > 0) || bScheduling)
            pFile->Print("(%s*)((%s)", sMWord.c_str(), sMWord.c_str());

        if (pMsgBuffer->HasReference())
            pFile->Print("%s", sMsgBuffer.c_str());
        else
            pFile->Print("&%s", sMsgBuffer.c_str());

        if (pMsgBuffer->GetCount(TYPE_FLEXPAGE, nDirection) > 0)
            pFile->Print("|2");
        if (bScheduling)
            *pFile << "|" << sScheduling;
        if ((pMsgBuffer->GetCount(TYPE_FLEXPAGE, nDirection) > 0) || bScheduling)
            *pFile << ")";
    }
    pFile->Print(",\n");

    pFile->PrintIndent("*((%s*)(&(", sMWord.c_str());
    #warning if short send IPC print parameters here
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nDirection, pContext);
    pFile->Print("[0]))),\n");
    pFile->PrintIndent("*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nDirection, pContext);
    pFile->Print("[4]))),\n");

//  if (bWord3)
//     pFile->PrintIndent("*((%s*)(&(", sMWord.c_str());
//     pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
//     pFile->Print("[8]))),\n");

    nDirection = pFunction->GetReceiveDirection();
    if (IsShortIPC(pFunction, pContext, nDirection))
        pFile->PrintIndent("L4_IPC_SHORT_MSG,\n");
    else
    {
        if (pMsgBuffer->HasReference())
            pFile->PrintIndent("%s,\n", sMsgBuffer.c_str());
        else
            pFile->PrintIndent("&%s,\n", sMsgBuffer.c_str());
    }

    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    #warning if short recv IPC print parameter here
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nDirection, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nDirection, pContext);
    pFile->Print("[4])),\n");

//  if (bWord3)
//     pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
//     pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
//     pFile->Print("[8])),\n");

    pFile->PrintIndent("%s, &%s);\n", sTimeout.c_str(), sResult.c_str());

    pFile->DecIndent();
}

/** \brief write an IPC receive operation
 *  \param pFile the file to write to
 *  \param pFunction the function to write it for
 *  \param pContext the context of the write operation
 */
void CL4BEIPC::WriteReceive(CBEFile* pFile,  CBEFunction* pFunction, CBEContext* pContext)
{
    string sServerID = pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    string sTimeout;
    if (pFunction->IsComponentSide())
        sTimeout = pContext->GetNameFactory()->GetTimeoutServerVariable(pContext);
    else
        sTimeout = pContext->GetNameFactory()->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    string sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    assert(pMsgBuffer);

    pFile->PrintIndent("l4_ipc_receive(*%s,\n", sServerID.c_str());
    pFile->IncIndent();

    if (IsShortIPC(pFunction, pContext, pFunction->GetReceiveDirection()))
        pFile->PrintIndent("L4_IPC_SHORT_MSG,\n ");
    else
    {
        if (pMsgBuffer->HasReference())
            pFile->PrintIndent("%s,\n", sMsgBuffer.c_str());
        else
            pFile->PrintIndent("&%s,\n", sMsgBuffer.c_str());
    }

    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    #warning if short IPC print parameters here
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pFunction->GetReceiveDirection(), pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pFunction->GetReceiveDirection(), pContext);
    pFile->Print("[4])),\n");

    pFile->PrintIndent("%s, &%s);\n", sTimeout.c_str(), sResult.c_str());

    pFile->DecIndent();
}

/** \brief write an IPC wait operation
 *  \param pFile the file to write to
 *  \param pFunction the function to write it for
 *  \param pContext the context of the write operation
 */
void CL4BEIPC::WriteWait(CBEFile* pFile, CBEFunction *pFunction, CBEContext* pContext)
{
    string sServerID = pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    string sTimeout;
    if (pFunction->IsComponentSide())
        sTimeout = pContext->GetNameFactory()->GetTimeoutServerVariable(pContext);
    else
        sTimeout = pContext->GetNameFactory()->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    string sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    assert(pMsgBuffer);
    int nDirection = pFunction->GetReceiveDirection();
    bool bVarBuffer = pMsgBuffer->IsVariableSized(nDirection) || (pMsgBuffer->GetAlias()->GetStars() > 0);

    pFile->PrintIndent("l4_ipc_wait(%s,\n", sServerID.c_str());
    pFile->IncIndent();
    if (IsShortIPC(pFunction, pContext, nDirection))
        pFile->PrintIndent("L4_IPC_SHORT_MSG,\n");
    else
    {
        if (bVarBuffer)
            pFile->PrintIndent("%s,\n", sMsgBuffer.c_str());
        else
            pFile->PrintIndent("&%s,\n", sMsgBuffer.c_str());
    }
    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    #warning if send short IPC print parameter here
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nDirection, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nDirection, pContext);
    pFile->Print("[4])),\n");
    pFile->PrintIndent("%s, &%s);\n", sTimeout.c_str(), sResult.c_str());
    pFile->DecIndent();
}

/** \brief write an IPC reply and receive operation
 *  \param pFile the file to write to
 *  \param pFunction the function to write it for
 *  \param bSendFlexpage true if a flexpage should be send (false, if the message buffer should determine this)
 *  \param bSendShortIPC true if a short IPC should be send (false, if message buffer should determine this)
 *  \param pContext the context of the write operation
 */
void 
CL4BEIPC::WriteReplyAndWait(CBEFile* pFile, 
    CBEFunction* pFunction, 
    bool bSendFlexpage, 
    bool bSendShortIPC, 
    CBEContext* pContext)
{
    CBENameFactory *pNF = pContext->GetNameFactory();
    string sResult = pNF->GetString(STR_RESULT_VAR, pContext);
    string sTimeout;
    if (pFunction->IsComponentSide())
        sTimeout = pNF->GetTimeoutServerVariable(pContext);
    else
        sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sServerID = pNF->GetComponentIDVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    assert(pMsgBuffer);

    *pFile << "\tl4_ipc_reply_and_wait(*" << sServerID << ",\n";
    pFile->IncIndent();
    *pFile << "\t(";
    if (bSendShortIPC)
    {
        *pFile << sMWord << "*)(L4_IPC_SHORT_MSG";
        if (bSendFlexpage)
            *pFile << "|2";
    }
    else
    {
        if (bSendFlexpage)
            *pFile << sMWord << "*)((" << sMWord << ")";
        *pFile << sMsgBuffer;
        if (bSendFlexpage)
            pFile->Print("|2");
    }
    pFile->Print("),\n");

    int nRcvDir = pFunction->GetReceiveDirection();
    *pFile << "\t*((" << sMWord << "*)(&(";
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nRcvDir, pContext);
    *pFile << "[0]))),\n";
    *pFile << "\t*((" << sMWord << "*)(&(";
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nRcvDir, pContext);
    *pFile << "[4]))),\n";

    *pFile << "\t" << sServerID << ",\n";
    *pFile << "\t" << sMsgBuffer << ",\n";

    *pFile << "\t(" << sMWord << "*)(&(";
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    *pFile << "[0])),\n";
    *pFile << "\t(" << sMWord << "*)(&(";
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    *pFile << "[4])),\n";

    *pFile << "\t" << sTimeout << ", &" << sResult << ");\n";

    pFile->DecIndent();
}

/** \brief write an IPC send operation
 *  \param pFile the file to write to
   \param pFunction the function to write it for
 *  \param pContext the context of the write operation
 */
void 
CL4BEIPC::WriteSend(CBEFile* pFile, 
    CBEFunction* pFunction, 
    CBEContext* pContext)
{
    int nDirection = pFunction->GetSendDirection();
    CBENameFactory *pNF = pContext->GetNameFactory();
    string sServerID = pNF->GetComponentIDVariable(pContext);
    string sResult = pNF->GetString(STR_RESULT_VAR, pContext);
    string sTimeout;
    if (pFunction->IsComponentSide())
        sTimeout = pNF->GetTimeoutServerVariable(pContext);
    else
        sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    string sScheduling = pNF->GetScheduleClientVariable(pContext);
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    assert(pMsgBuffer);

    *pFile << "\tl4_ipc_send(*" << sServerID << ",\n";
    pFile->IncIndent();
    *pFile << "\t";
    bool bVarBuffer = pMsgBuffer->IsVariableSized(nDirection) ||
                     (pMsgBuffer->GetAlias()->GetStars() > 0);
    bool bScheduling = pFunction->FindAttribute(ATTR_L4_SCHED_DECEIT); 
    /* OR further attributes */

    if (IsShortIPC(pFunction, pContext, nDirection))
    {
        *pFile << "L4_IPC_SHORT_MSG";
        if (bScheduling)
            *pFile << "|" << sScheduling;
        if (pMsgBuffer->GetCount(TYPE_FLEXPAGE, nDirection) > 0)
            *pFile << "|2";
    }
    else
    {
        if ((pMsgBuffer->GetCount(TYPE_FLEXPAGE, nDirection) > 0) || 
	    bScheduling)
            pFile->Print("(%s*)((%s)", sMWord.c_str(), sMWord.c_str());

        if (!bVarBuffer)
            *pFile << "&";
        *pFile << sMsgBuffer;

        if (pMsgBuffer->GetCount(TYPE_FLEXPAGE, nDirection) > 0)
            pFile->Print(")|2");
        if (bScheduling)
            *pFile << "|" << sScheduling;
        if ((pMsgBuffer->GetCount(TYPE_FLEXPAGE, nDirection) > 0) || 
	    bScheduling)
            *pFile << ")";
    }
    pFile->Print(",\n");

    *pFile << "\t*((" << sMWord << "*)(&(";
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nDirection, pContext);
    *pFile << "[0]))),\n";
    *pFile << "\t*((" << sMWord << "*)(&(";
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nDirection, pContext);
    *pFile << "[4]))),\n";

    *pFile << "\t" << sTimeout << ", &" << sResult << ");\n";

    pFile->DecIndent();
}

/** \brief write an IPC reply operation
 *  \param pFile the file to write to
 *  \param pFunction the function to write it for
 *  \param pContext the context of the write operation
 *
 * In the generic L4 case this is a send operation. We have to be careful though
 * with ASM code, which can push parameters directly into registers, since the
 * parameters for reply (exception) are not the same as for send (opcode).
 */
void CL4BEIPC::WriteReply(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    WriteSend(pFile, pFunction, pContext);
}

/** \brief determine if we should use assembler for the IPCs
 *  \param pFunction the function to write the call for
 *  \param pContext the context of the write operation
 *  \return true if assembler code should be written
 *
 * This implementation currently always returns false, because assembler code
 * is always ABI specific.
 */
bool CL4BEIPC::UseAssembler(CBEFunction *pFunction, CBEContext *pContext)
{
    return false;
}

/** \brief helper function to test for short IPC
 *  \param pFunction the function to test
 *  \param pContext the context of the test
 *  \param nDirection the direction to test
 *  \return true if the function uses short IPC in the specified direction
 *
 * This is a simple helper function, which just delegates the call to
 * the function's message buffer.
 */
bool CL4BEIPC::IsShortIPC(CBEFunction *pFunction, CBEContext *pContext, int nDirection)
{
    if (nDirection == 0)
        return IsShortIPC(pFunction, pContext, pFunction->GetSendDirection()) &&
               IsShortIPC(pFunction, pContext, pFunction->GetReceiveDirection());
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    assert(pMsgBuffer);
    return  pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, nDirection, pContext);
}

/**    \brief check if the property is fulfilled for this communication
 *    \param pFunction the function using the communication
 *    \param nProperty the property to check
 *    \param pContext the omnipresent context
 *    \return true if the property if fulfilled
 */
bool CL4BEIPC::CheckProperty(CBEFunction *pFunction, int nProperty, CBEContext *pContext)
{
    switch (nProperty)
    {
    case COMM_PROP_USE_ASM:
        return UseAssembler(pFunction, pContext);
        break;
    }
    return false;
}
