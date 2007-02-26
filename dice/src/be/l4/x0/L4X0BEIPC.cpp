/**
 *	\file	dice/src/be/l4/x0/L4X0BEIPC.h
 *	\brief	contains the declaration of the class CL4X0BEIPC
 *
 *	\date	08/13/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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

IMPLEMENT_DYNAMIC(CL4X0BEIPC);

CL4X0BEIPC::CL4X0BEIPC()
 : CL4BEIPC()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0BEIPC, CL4BEIPC);
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
    String sServerID = pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    String sTimeout = pContext->GetNameFactory()->GetTimeoutClientVariable(pContext);
    String sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
    String sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
	CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType*)pFunction->GetMessageBuffer();

    pFile->PrintIndent("l4_ipc_call_w3(*%s,\n", (const char *) sServerID);
    pFile->IncIndent();
    pFile->PrintIndent("");
    if (pMsgBuffer->HasSendFlexpages())
        pFile->Print("(%s*)((%s)", (const char*)sMWord, (const char*)sMWord);
    if (pMsgBuffer->IsShortIPC(pFunction->GetSendDirection(), pContext))
        pFile->Print("L4_IPC_SHORT_MSG");
    else
    {
        if (pMsgBuffer->HasReference())
            pFile->Print("%s", (const char *) sMsgBuffer);
        else
            pFile->Print("&%s", (const char *) sMsgBuffer);
    }
    if (pMsgBuffer->HasSendFlexpages())
        pFile->Print("|2)");
    pFile->Print(",\n");

    pFile->PrintIndent("*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0]))),\n");
    pFile->PrintIndent("*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4]))),\n");
    pFile->PrintIndent("*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[8]))),\n");

    if (pMsgBuffer->IsShortIPC(pFunction->GetReceiveDirection(), pContext))
        pFile->PrintIndent("L4_IPC_SHORT_MSG,\n");
    else
    {
        if (pMsgBuffer->HasReference())
            pFile->PrintIndent("%s,\n", (const char *) sMsgBuffer);
        else
            pFile->PrintIndent("&%s,\n", (const char *) sMsgBuffer);
    }

    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])),\n");
    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[8])),\n");

    pFile->PrintIndent("%s, &%s);\n", (const char *) sTimeout, (const char *) sResult);

    pFile->DecIndent();
}

/** \brief writes the IPC receive
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4X0BEIPC::WriteReceive(CBEFile* pFile,  CBEFunction* pFunction,  bool bAllowShortIPC,  CBEContext* pContext)
{
	String sServerID = pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    String sTimeout;
    if (pFunction->IsComponentSide())
        sTimeout = pContext->GetNameFactory()->GetTimeoutServerVariable(pContext);
    else
        sTimeout = pContext->GetNameFactory()->GetTimeoutClientVariable(pContext);
    String sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    String sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
	CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType*)pFunction->GetMessageBuffer();

    pFile->PrintIndent("l4_ipc_receive_w3(*%s,\n", (const char *) sServerID);
    pFile->IncIndent();

    if (pMsgBuffer->IsShortIPC(pFunction->GetReceiveDirection(), pContext) && bAllowShortIPC)
        pFile->PrintIndent("L4_IPC_SHORT_MSG,\n ");
    else
    {
        if (pMsgBuffer->HasReference())
            pFile->PrintIndent("%s,\n", (const char *) sMsgBuffer);
        else
            pFile->PrintIndent("&%s,\n", (const char *) sMsgBuffer);
    }

    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])),\n");
    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[8])),\n");

    pFile->PrintIndent("%s, &%s);\n", (const char *) sTimeout, (const char *) sResult);

    pFile->DecIndent();
}

/** \brief writes the IPC reply-and-wait
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4X0BEIPC::WriteReplyAndWait(CBEFile* pFile,  CBEFunction* pFunction,  bool bSendFlexpage,  bool bSendShortIPC,  CBEContext* pContext)
{
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    String sTimeout;
    if (pFunction->IsComponentSide())
        sTimeout = pContext->GetNameFactory()->GetTimeoutServerVariable(pContext);
    else
        sTimeout = pContext->GetNameFactory()->GetTimeoutClientVariable(pContext);
    String sServerID = pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    String sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    String sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
	CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType*)pFunction->GetMessageBuffer();
// 	bSendFlexpage = bSendFlexpage || pMsgBuffer->HasSendFlexpages();
// 	bSendShortIPC = bSendShortIPC || pMsgBuffer->IsShortIPC(pFunction->GetSendDirection(), pContext);

    pFile->PrintIndent("l4_ipc_reply_and_wait_w3(*%s,\n", (const char *) sServerID);
    pFile->IncIndent();
    pFile->PrintIndent("");
    if (bSendFlexpage)
        pFile->Print("(%s*)((%s)", (const char*)sMWord, (const char*)sMWord);
    if (bSendShortIPC)
        pFile->Print("L4_IPC_SHORT_MSG");
    else
        pFile->Print("%s", (const char *) sMsgBuffer);
    if (bSendFlexpage)
        pFile->Print("|2)");
    pFile->Print(",\n");

    pFile->PrintIndent("*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0]))),\n");
    pFile->PrintIndent("*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4]))),\n");
    pFile->PrintIndent("*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[8]))),\n");

    pFile->PrintIndent("%s,\n", (const char *) sServerID);

    pFile->PrintIndent("%s,\n", (const char *) sMsgBuffer);
    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])),\n");
    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[8])),\n");

    pFile->PrintIndent("%s, &%s);\n", (const char *) sTimeout, (const char *) sResult);

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
    String sServerID = pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    String sTimeout;
    if (pFunction->IsComponentSide())
        sTimeout = pContext->GetNameFactory()->GetTimeoutServerVariable(pContext);
    else
        sTimeout = pContext->GetNameFactory()->GetTimeoutClientVariable(pContext);
    String sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    String sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
	CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType*)pFunction->GetMessageBuffer();

    pFile->PrintIndent("l4_ipc_send_w3(*%s,\n", (const char *) sServerID);
    pFile->IncIndent();
    pFile->PrintIndent("");
    bool bVarBuffer = pMsgBuffer->IsVariableSized(nDirection) || (pMsgBuffer->GetAlias()->GetStars() > 0);
    if (pMsgBuffer->HasSendFlexpages())
        pFile->Print("(%s*)((%s)", (const char*)sMWord, (const char*)sMWord);
    if (pMsgBuffer->IsShortIPC(nDirection, pContext))
        pFile->Print("L4_IPC_SHORT_MSG");
    else
    {
        if (bVarBuffer)
            pFile->Print("%s", (const char *) sMsgBuffer);
        else
            pFile->Print("&%s", (const char *) sMsgBuffer);
    }
    if (pMsgBuffer->HasSendFlexpages())
        pFile->Print(")|2)");
    pFile->Print(",\n");

    pFile->PrintIndent("*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0]))),\n");
    pFile->PrintIndent("*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4]))),\n");
    pFile->PrintIndent("*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[8]))),\n");

    pFile->PrintIndent("%s, &%s);\n", (const char *) sTimeout, (const char *) sResult);

    pFile->DecIndent();
}

/** \brief writes the IPC wait
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4X0BEIPC::WriteWait(CBEFile* pFile,  CBEFunction* pFunction,  bool bAllowShortIPC,  CBEContext* pContext)
{
    String sServerID = pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    String sTimeout;
    if (pFunction->IsComponentSide())
        sTimeout = pContext->GetNameFactory()->GetTimeoutServerVariable(pContext);
    else
        sTimeout = pContext->GetNameFactory()->GetTimeoutClientVariable(pContext);
    String sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    String sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
	CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType*)pFunction->GetMessageBuffer();
    int nDirection = pFunction->GetReceiveDirection();
    bool bVarBuffer = pMsgBuffer->IsVariableSized(nDirection) || (pMsgBuffer->GetAlias()->GetStars() > 0);

    pFile->PrintIndent("l4_ipc_wait_w3(%s,\n", (const char *) sServerID);
    pFile->IncIndent();
    if (pMsgBuffer->IsShortIPC(nDirection, pContext) && bAllowShortIPC)
        pFile->PrintIndent("L4_IPC_SHORT_MSG,\n");
    else
    {
        if (bVarBuffer)
            pFile->PrintIndent("%s,\n", (const char *) sMsgBuffer);
        else
            pFile->PrintIndent("&%s,\n", (const char *) sMsgBuffer);
    }
    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])),\n");
    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[8])),\n");
    pFile->PrintIndent("%s, &%s);\n", (const char *) sTimeout, (const char *) sResult);
    pFile->DecIndent();
}
