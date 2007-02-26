/**
 *    \file    dice/src/be/l4/x0/L4X0BEIPC.cpp
 *    \brief   contains the declaration of the class CL4X0BEIPC
 *
 *    \date    08/13/2002
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
#include "be/l4/x0/L4X0BEIPC.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEFunction.h"
#include "be/BEDeclarator.h"
#include "TypeSpec-Type.h"

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
 *  \param pContext the context of the write operation
 */
void CL4X0BEIPC::WriteCall(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    string sServerID = pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    string sTimeout = pContext->GetNameFactory()->GetTimeoutClientVariable(pContext);
    string sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
    string sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    assert(pMsgBuffer);
    int nSendDir = pFunction->GetSendDirection();
    int nRecvDir = pFunction->GetReceiveDirection();
    bool bShortSend = pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, nSendDir, pContext);
    bool bShortRecv = pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, nRecvDir, pContext);

    pFile->PrintIndent("l4_ipc_call_w3(*%s,\n", sServerID.c_str());
    pFile->IncIndent();
    pFile->PrintIndent("");
    if (bShortSend)
        pFile->Print("L4_IPC_SHORT_MSG");
    else
    {
        if (pMsgBuffer->GetCount(TYPE_FLEXPAGE, nSendDir) > 0)
            pFile->Print("(%s*)((%s)", sMWord.c_str(), sMWord.c_str());

        if (pMsgBuffer->HasReference())
            pFile->Print("%s", sMsgBuffer.c_str());
        else
            pFile->Print("&%s", sMsgBuffer.c_str());

        if (pMsgBuffer->GetCount(TYPE_FLEXPAGE, nSendDir) > 0)
            pFile->Print("|2)");
    }
    pFile->Print(",\n");

    pFile->PrintIndent("*((%s*)(&(", sMWord.c_str());
    #warning for short IPC print parameters
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nSendDir, pContext);
    pFile->Print("[0]))),\n");
    pFile->PrintIndent("*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nSendDir, pContext);
    pFile->Print("[4]))),\n");
    pFile->PrintIndent("*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nSendDir, pContext);
    pFile->Print("[8]))),\n");

    if (bShortRecv)
        pFile->PrintIndent("L4_IPC_SHORT_MSG,\n");
    else
    {
        if (pMsgBuffer->HasReference())
            pFile->PrintIndent("%s,\n", sMsgBuffer.c_str());
        else
            pFile->PrintIndent("&%s,\n", sMsgBuffer.c_str());
    }

    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nRecvDir, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nRecvDir, pContext);
    pFile->Print("[4])),\n");
    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nRecvDir, pContext);
    pFile->Print("[8])),\n");

    pFile->PrintIndent("%s, &%s);\n", sTimeout.c_str(), sResult.c_str());

    pFile->DecIndent();
}

/** \brief writes the IPC receive
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *    \param bAllowShortIPC true if short IPC is allowed
 *  \param pContext the context of the write operation
 */
void CL4X0BEIPC::WriteReceive(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
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

    pFile->PrintIndent("l4_ipc_receive_w3(*%s,\n", sServerID.c_str());
    pFile->IncIndent();

    if (pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, DIRECTION_OUT, pContext))
        pFile->PrintIndent("L4_IPC_SHORT_MSG,\n ");
    else
    {
        if (pMsgBuffer->HasReference())
            pFile->PrintIndent("%s,\n", sMsgBuffer.c_str());
        else
            pFile->PrintIndent("&%s,\n", sMsgBuffer.c_str());
    }

    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    #warning for short IPC print parameters
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])),\n");
    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8])),\n");

    pFile->PrintIndent("%s, &%s);\n", sTimeout.c_str(), sResult.c_str());

    pFile->DecIndent();
}

/** \brief writes the IPC reply-and-wait
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *    \param bSendFlexpage true if a flexpage is sent
 *    \param bSendShortIPC true if a short IPC is sent
 *  \param pContext the context of the write operation
 */
void CL4X0BEIPC::WriteReplyAndWait(CBEFile* pFile,  CBEFunction* pFunction,  bool bSendFlexpage,  bool bSendShortIPC,  CBEContext* pContext)
{
    string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    string sTimeout;
    if (pFunction->IsComponentSide())
        sTimeout = pContext->GetNameFactory()->GetTimeoutServerVariable(pContext);
    else
        sTimeout = pContext->GetNameFactory()->GetTimeoutClientVariable(pContext);
    string sServerID = pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    string sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    string sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    assert(pMsgBuffer);

    pFile->PrintIndent("l4_ipc_reply_and_wait_w3(*%s,\n", sServerID.c_str());
    pFile->IncIndent();
    pFile->PrintIndent("");
    if (bSendFlexpage)
        pFile->Print("(%s*)((%s)", sMWord.c_str(), sMWord.c_str());
    if (bSendShortIPC)
        pFile->Print("L4_IPC_SHORT_MSG");
    else
        pFile->Print("%s", sMsgBuffer.c_str());
    if (bSendFlexpage)
        pFile->Print("|2)");
    pFile->Print(",\n");

    pFile->PrintIndent("*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("[0]))),\n");
    pFile->PrintIndent("*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("[4]))),\n");
    pFile->PrintIndent("*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("[8]))),\n");

    pFile->PrintIndent("%s,\n", sServerID.c_str());

    pFile->PrintIndent("%s,\n", sMsgBuffer.c_str());
    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])),\n");
    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8])),\n");

    pFile->PrintIndent("%s, &%s);\n", sTimeout.c_str(), sResult.c_str());

    pFile->DecIndent();
}

/** \brief writes the IPC send
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4X0BEIPC::WriteSend(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    int nDirection = pFunction->GetSendDirection();
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

    pFile->PrintIndent("l4_ipc_send_w3(*%s,\n", sServerID.c_str());
    pFile->IncIndent();
    pFile->PrintIndent("");
    bool bVarBuffer = pMsgBuffer->IsVariableSized(nDirection) ||
                     (pMsgBuffer->GetAlias()->GetStars() > 0);
    bool bShortIPC = pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, nDirection, pContext);
    if (!bShortIPC && (pMsgBuffer->GetCount(TYPE_FLEXPAGE, nDirection) > 0))
        pFile->Print("(%s*)((%s)", sMWord.c_str(), sMWord.c_str());
    if (bShortIPC)
        pFile->Print("L4_IPC_SHORT_MSG");
    else
    {
        if (bVarBuffer)
            pFile->Print("%s", sMsgBuffer.c_str());
        else
            pFile->Print("&%s", sMsgBuffer.c_str());
    }
    if (!bShortIPC && (pMsgBuffer->GetCount(TYPE_FLEXPAGE, nDirection) > 0))
        pFile->Print(")|2)");
    pFile->Print(",\n");

    pFile->PrintIndent("*((%s*)(&(", sMWord.c_str());
    #warning for short IPC print parameter
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nDirection, pContext);
    pFile->Print("[0]))),\n");
    pFile->PrintIndent("*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nDirection, pContext);
    pFile->Print("[4]))),\n");
    pFile->PrintIndent("*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nDirection, pContext);
    pFile->Print("[8]))),\n");

    pFile->PrintIndent("%s, &%s);\n", sTimeout.c_str(), sResult.c_str());

    pFile->DecIndent();
}

/** \brief writes the IPC wait
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bAllowShortIPC true if a short IPC is received
 *  \param pContext the context of the write operation
 */
void CL4X0BEIPC::WriteWait(CBEFile* pFile,  CBEFunction* pFunction, CBEContext* pContext)
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
    bool bVarBuffer = pMsgBuffer->IsVariableSized(nDirection) ||
                     (pMsgBuffer->GetAlias()->GetStars() > 0);
    bool bShortIPC = pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, nDirection, pContext);

    pFile->PrintIndent("l4_ipc_wait_w3(%s,\n", sServerID.c_str());
    pFile->IncIndent();
    if (bShortIPC)
        pFile->PrintIndent("L4_IPC_SHORT_MSG,\n");
    else
    {
        if (bVarBuffer)
            pFile->PrintIndent("%s,\n", sMsgBuffer.c_str());
        else
            pFile->PrintIndent("&%s,\n", sMsgBuffer.c_str());
    }
    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    #warning for short IPC print parameter
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nDirection, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nDirection, pContext);
    pFile->Print("[4])),\n");
    pFile->PrintIndent("(%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, nDirection, pContext);
    pFile->Print("[8])),\n");
    pFile->PrintIndent("%s, &%s);\n", sTimeout.c_str(), sResult.c_str());
    pFile->DecIndent();
}
