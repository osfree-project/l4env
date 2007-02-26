/**
 *    \file    dice/src/be/l4/L4BEDispatchFunction.cpp
 *    \brief   contains the implementation of the class CL4BEDispatchFunction
 *
 *    \date    10/10/2003
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
#include "be/l4/L4BEDispatchFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "TypeSpec-Type.h"

CL4BEDispatchFunction::CL4BEDispatchFunction()
{
}

CL4BEDispatchFunction::CL4BEDispatchFunction(CL4BEDispatchFunction & src)
: CBEDispatchFunction(src)
{
}

/**    \brief destructor of target class */
CL4BEDispatchFunction::~CL4BEDispatchFunction()
{

}

/** \brief write the L4 specific switch code (add tracing)
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BEDispatchFunction::WriteSwitch(CBEFile * pFile,  CBEContext * pContext)
{
    string sOpcodeVar = pContext->GetNameFactory()->GetOpcodeVariable(pContext);
    string sObjectVar = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    string sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, false, pContext, 0);
    string sFunc = pContext->GetTraceServerFunc();
    string sReply = pContext->GetNameFactory()->GetReplyCodeVariable(pContext);
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);

    if (sFunc.empty())
        sFunc = string("printf");
    if (pContext->IsOptionSet(PROGRAM_TRACE_SERVER))
    {
        pFile->PrintIndent("%s(\"opcode %%x received from %%x.%%x%s\", %s, %s->id.task, %s->id.lthread);\n",
                            sFunc.c_str(),
                            (sFunc=="LOG")?"":"\\n",
                            sOpcodeVar.c_str(),
                            sObjectVar.c_str(),
                            sObjectVar.c_str());
        pFile->PrintIndent("%s(\"received dw0=%%x, dw1=%%x%s\", ", sFunc.c_str(),
                            (sFunc=="LOG")?"":"\\n");
        pFile->Print("(*((%s*)(&(", sMWord.c_str());
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
        pFile->Print("[0])))), ");
        pFile->Print("(*((%s*)(&(", sMWord.c_str());
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
        pFile->Print("[4]))))");
        pFile->Print(");\n");
    }
    CBEDispatchFunction::WriteSwitch(pFile, pContext);
    if (pContext->IsOptionSet(PROGRAM_TRACE_SERVER))
    {
        pFile->PrintIndent("%s(\"reply %%s (dw0=%%x, dw1=%%x)%s\", (%s==DICE_REPLY)?\"DICE_REPLY\":\"DICE_NO_REPLY\", ",
                            sFunc.c_str(),
                            (sFunc=="LOG")?"":"\\n",
                            sReply.c_str());
        pFile->Print("(*((%s*)(&(", sMWord.c_str());
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
        pFile->Print("[0])))), ");
        pFile->Print("(*((%s*)(&(", sMWord.c_str());
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
        pFile->Print("[4]))))");
        pFile->Print(");\n");
        // print if we got an fpage
        pFile->PrintIndent("%s(\"  fpage: %%s%s\", (",
                            sFunc.c_str(),
                            (sFunc=="LOG")?"":"\\n");
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SIZE, DIRECTION_IN, pContext);
        pFile->Print(".md.fpage_received==1)?\"yes\":\"no\");\n");
    }
}

/** \brief write the L4 specific code when setting the opcode exception in the message buffer
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BEDispatchFunction::WriteSetWrongOpcodeException(CBEFile* pFile,  CBEContext* pContext)
{
    // first call base class
    CBEDispatchFunction::WriteSetWrongOpcodeException(pFile, pContext);
    // set short IPC
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->WriteInitialization(pFile, TYPE_MSGDOPE_SEND, 0, pContext);
}

/** \brief writes the default case if there is no default function
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * If we have an error function, this function has been called on IPC errors. After
 * it has been called the IPC error exception is set in the environment. If this is
 * the case, we do not have to send a reply, because this was no real error.
 */
void CL4BEDispatchFunction::WriteDefaultCaseWithoutDefaultFunc(CBEFile* pFile,  CBEContext* pContext)
{
    vector<CBEDeclarator*>::iterator iterCE = m_pCorbaEnv->GetFirstDeclarator();
    CBEDeclarator *pDecl = *iterCE;
    pFile->PrintIndent("if ((");
    pDecl->WriteName(pFile, pContext);
    if (pDecl->GetStars() > 0)
        pFile->Print("->");
    else
        pFile->Print(".");
    pFile->Print("major == CORBA_SYSTEM_EXCEPTION) && \n");
    pFile->IncIndent();
    pFile->PrintIndent("(");
    pDecl->WriteName(pFile, pContext);
    if (pDecl->GetStars() > 0)
        pFile->Print("->");
    else
        pFile->Print(".");
    pFile->Print("repos_id == CORBA_DICE_INTERNAL_IPC_ERROR))\n");
    pFile->DecIndent();
    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    // clear exception
    pFile->PrintIndent("CORBA_server_exception_free(");
    if (pDecl->GetStars() == 0)
        pFile->Print("&");
    pDecl->WriteName(pFile, pContext);
    pFile->Print(");\n");
    // wait for next ipc
    string sReply = pContext->GetNameFactory()->GetReplyCodeVariable(pContext);
    pFile->PrintIndent("%s = DICE_NO_REPLY;\n", sReply.c_str());
    // finished
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
    // else: normal handling
    pFile->PrintIndent("else\n");
    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    CBEDispatchFunction::WriteDefaultCaseWithoutDefaultFunc(pFile, pContext);
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
}
