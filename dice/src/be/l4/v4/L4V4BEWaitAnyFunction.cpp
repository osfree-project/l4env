/**
 *  \file     dice/src/be/l4/v4/L4V4BEWaitAnyFunction.cpp
 *  \brief    contains the implementation of the class CL4V4BEWaitAnyFunction
 *
 *  \date     Mon Jul 5 2004
 *  \author   Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004 by
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
#include "L4V4BEWaitAnyFunction.h"
#include "L4V4BENameFactory.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEDeclarator.h"
#include "be/BEMarshaller.h"
#include "be/BEUserDefinedType.h"
#include "be/BECommunication.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"

CL4V4BEWaitAnyFunction::CL4V4BEWaitAnyFunction(bool bOpenWait, bool bReply)
 : CL4BEWaitAnyFunction(bOpenWait, bReply)
{
}

/** destroys the instance of the class */
CL4V4BEWaitAnyFunction::~CL4V4BEWaitAnyFunction()
{
}

/** \brief writes the variable declarations of this function
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * The variable declarations of the wait-any function only contains so-called
 * helper variables. This is the result variable.
 */
void
CL4V4BEWaitAnyFunction::WriteVariableDeclaration(CBEFile * pFile,
    CBEContext * pContext)
{
    // first call base class (skip base class)
    CBEWaitAnyFunction::WriteVariableDeclaration(pFile, pContext);
    // need message tag
    string sMsgTag = pContext->GetNameFactory()->GetString(STR_MSGTAG_VARIABLE,
                        pContext, 0);
    *pFile << "\tL4_MsgTag_t " << sMsgTag << ";\n";

    // write loop variable for msg buffer dump
    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF))
        pFile->PrintIndent("int _i;\n");
}

/** \brief writes the invocation call to thetarget file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * The wait any function simply waits for any message and unmarshals the
 * opcode. Since the message buffer is a referenced parameter, we know for sure,
 * that the "buffer" is a pointer.
 */
void
CL4V4BEWaitAnyFunction::WriteInvocation(CBEFile * pFile,
    CBEContext * pContext)
{
    // load message
    bool bVarSized = false;
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (pMsgBuffer->GetAlias() &&
        ((pMsgBuffer->GetAlias()->GetStars() > 0) ||
         pMsgBuffer->IsVariableSized(GetReceiveDirection())))
         bVarSized = true;

    if (m_bReply)
    {
        // load the message into the UTCB
        *pFile << "\tL4_MsgLoad ( " << ((bVarSized) ? "" : "&") <<
            pMsgBuffer->GetAlias()->GetName() << " );\n";
    }

    // invocate
    WriteIPC(pFile, pContext);

    // store message
    string sMsgTag = pContext->GetNameFactory()->GetString(STR_MSGTAG_VARIABLE,
                        pContext, 0);
    *pFile << "\tL4_MsgStore (" << sMsgTag << ", " <<
        ((bVarSized) ? "" : "&") << pMsgBuffer->GetAlias()->GetName() <<
        ");\n";

    WriteExceptionCheck(pFile, pContext); // reset exception
    WriteIPCErrorCheck(pFile, pContext); // set IPC exception
    if (m_bReply)
        WriteReleaseMemory(pFile, pContext);

    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF))
    {
        string sResult =
            pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
        pMsgBuffer->WriteDump(pFile, sResult, pContext);
    }
}

/** \brief writes the ipc code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void
CL4V4BEWaitAnyFunction::WriteIPCReplyWait(CBEFile *pFile,
    CBEContext *pContext)
{
    assert(m_pComm);
    m_pComm->WriteReplyAndWait(pFile, this, pContext);
}

/** \brief writes the unmarshalling code for this function
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start unmarshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it \
 *           to false if not possible
 *  \param pContext the context of the write operation
 *
 * The wait-any function does only unmarshal the opcode. We can print this code
 * by hand. We should use a marshaller anyways.
 */
void CL4V4BEWaitAnyFunction::WriteUnmarshalling(CBEFile * pFile,
    int nStartOffset,
    bool& bUseConstOffset,
    CBEContext * pContext)
{
    /* If the option noopcode is set, we do not unmarshal anything at all. */
    if (FindAttribute(ATTR_NOOPCODE))
        return;
    /* the opcode is the label in the msgtag */
    string sMsgTag = pContext->GetNameFactory()->GetString(STR_MSGTAG_VARIABLE,
                        pContext, 0);
    /* get name of opcode (return variable) */
    vector<CBEDeclarator*>::iterator iterRet = m_pReturnVar->GetFirstDeclarator();
    CBEDeclarator *pD = *iterRet;

    *pFile << "\t// 'unmarshal' the opcode\n";
    *pFile << "\t" << pD->GetName() << " = L4_Label (" << sMsgTag << ");\n";
}

/** \brief write the error checking code for the IPC
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void
CL4V4BEWaitAnyFunction::WriteIPCErrorCheck(CBEFile * pFile,
    CBEContext * pContext)
{
    string sMsgTag = pContext->GetNameFactory()->GetString(STR_MSGTAG_VARIABLE,
                        pContext, 0);
    *pFile << "\t/* test for IPC errors */\n";
    *pFile << "\tif ( L4_IpcFailed ( " << sMsgTag << " ))\n";
    *pFile << "\t{\n";
    pFile->IncIndent();
    // set opcode to zero value
    if (m_pReturnVar)
        m_pReturnVar->WriteSetZero(pFile, pContext);
    if (!m_sErrorFunction.empty())
    {
        *pFile << "\t" << m_sErrorFunction << "(" << sMsgTag << ", ";
        WriteCallParameter(pFile, m_pCorbaEnv, pContext);
        *pFile << ");\n";
    }
    // set label to zero and store in msgbuf
    bool bVarSized = false;
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (pMsgBuffer->GetAlias() &&
        ((pMsgBuffer->GetAlias()->GetStars() > 0) ||
         pMsgBuffer->IsVariableSized(GetReceiveDirection())))
         bVarSized = true;
    *pFile << "\tL4_Set_MsgLabel ( " << ((bVarSized) ? "" : "&") <<
        pMsgBuffer->GetAlias()->GetName() << ", 0);\n";
    // set exception
    vector<CBEDeclarator*>::iterator iterCE = m_pCorbaEnv->GetFirstDeclarator();
    CBEDeclarator *pDecl = *iterCE;
    string sSetFunc;
    if (((CBEUserDefinedType*)m_pCorbaEnv->GetType())->GetName() ==
        "CORBA_Server_Environment")
        sSetFunc = "CORBA_server_exception_set";
    else
        sSetFunc = "CORBA_exception_set";
    *pFile << "\t" << sSetFunc << "(";
    if (pDecl->GetStars() == 0)
        pFile->Print("&");
    pDecl->WriteName(pFile, pContext);
    pFile->Print(",\n");
    pFile->IncIndent();
    pFile->PrintIndent("CORBA_SYSTEM_EXCEPTION,\n");
    pFile->PrintIndent("CORBA_DICE_INTERNAL_IPC_ERROR,\n");
    pFile->PrintIndent("0);\n");
    pFile->DecIndent();
    // returns 0 -> falls into default branch of server loop
    WriteReturn(pFile, pContext);
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
}
