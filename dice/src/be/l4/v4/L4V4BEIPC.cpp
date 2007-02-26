/**
 *    \file    dice/src/be/l4/v4/L4V4BEIPC.cpp
 *    \brief    contains the implementation of the class CL4V4BEIPC
 *
 *    \date    01/08/2004
 *    \author    Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
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

#include "be/l4/v4/L4V4BEIPC.h"
#include "be/l4/v4/L4V4BENameFactory.h"
#include "be/BEContext.h"
#include "be/BEFunction.h"

CL4V4BEIPC::CL4V4BEIPC()
 : CL4BEIPC()
{
}

/** destroys the object */
CL4V4BEIPC::~CL4V4BEIPC()
{
}

/** \brief writes the IPC call invocation
 *  \param pFile the file to write to
 *  \param pFunction the function to call for
 *  \param pContext the context of the write operation
 */
void
CL4V4BEIPC::WriteCall(CBEFile* pFile,
    CBEFunction* pFunction,
    CBEContext* pContext)
{
    string sServerID =
        pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    string sReturn =
        pContext->GetNameFactory()->GetString(STR_MSGTAG_VARIABLE, pContext, 0);
    string sTimeout;
    if (pFunction->IsComponentSide())
        sTimeout = pContext->GetNameFactory()->GetTimeoutServerVariable(pContext);
    else
        sTimeout = pContext->GetNameFactory()->GetTimeoutClientVariable(pContext);

    // MsgTag Call(ThreadId to)
    *pFile << "\t" << sReturn << " = L4_Call_Timeouts ( *" << sServerID << ", "
        << sTimeout << ", L4_Never );\n";
}

/** \brief writes the IPC receive
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void
CL4V4BEIPC::WriteReceive(CBEFile* pFile,
    CBEFunction* pFunction,
    CBEContext* pContext)
{
    string sServerID =
        pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    string sReturn =
        pContext->GetNameFactory()->GetString(STR_MSGTAG_VARIABLE, pContext, 0);
    string sTimeout;
    if (pFunction->IsComponentSide())
        sTimeout = pContext->GetNameFactory()->GetTimeoutServerVariable(pContext);
    else
        sTimeout = pContext->GetNameFactory()->GetTimeoutClientVariable(pContext);

    // MsgTag Receive(ThreadId from) (timeout: never)
    *pFile << "\t" << sReturn << " = L4_Receive_Timeout (*" << sServerID <<
        ", " << sTimeout << ");\n";
}

/** \brief writes the IPC reply-and-wait
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bSendFlexpage true if we send a flexpage
 *  \param bSendShortIPC true if an short IPC is sent
 *  \param pContext the context of the write operation
 */
void
CL4V4BEIPC::WriteReplyAndWait(CBEFile* pFile,
    CBEFunction* pFunction,
    bool bSendFlexpage,
    bool bSendShortIPC,
    CBEContext* pContext)
{
    WriteReplyAndWait(pFile, pFunction, pContext);
}

/** \brief writes the IPC reply-and-wait
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void
CL4V4BEIPC::WriteReplyAndWait(CBEFile* pFile,
    CBEFunction* pFunction,
    CBEContext* pContext)
{
    string sServerID =
        pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    string sReturn =
        pContext->GetNameFactory()->GetString(STR_MSGTAG_VARIABLE, pContext, 0);

    // MsgTag ReplyWait(ThreadId to, ThreadId& from) (snd timeout: 0, rcv timeout: never)
    *pFile << "\t" << sReturn << " = L4_ReplyWait (*" << sServerID << ", " <<
        sServerID << ");\n";
}

/** \brief writes the IPC send
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4V4BEIPC::WriteSend(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    string sServerID = pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    string sReturn = pContext->GetNameFactory()->GetString(STR_MSGTAG_VARIABLE, pContext, 0);

    // MsgTag Send(ThreadId to) (timeout: never)
    *pFile << "\t" << sReturn << " = L4_Send (*" << sServerID << ");\n";
}

/** \brief writes the IPC wait
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void
CL4V4BEIPC::WriteWait(CBEFile* pFile,
    CBEFunction* pFunction,
    CBEContext* pContext)
{
    string sServerID =
        pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    string sReturn =
        pContext->GetNameFactory()->GetString(STR_MSGTAG_VARIABLE, pContext, 0);
    string sTimeout;
    if (pFunction->IsComponentSide())
        sTimeout = pContext->GetNameFactory()->GetTimeoutServerVariable(pContext);
    else
        sTimeout = pContext->GetNameFactory()->GetTimeoutClientVariable(pContext);

    // MsgTag Wait(ThreadId to) (timeout: never)
    *pFile << "\t" << sReturn << " = L4_Wait_Timeout (" << sTimeout << ", " <<
         sServerID << ");\n";
}
