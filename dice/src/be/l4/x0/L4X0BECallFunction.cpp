/**
 *    \file    dice/src/be/l4/x0/L4X0BECallFunction.cpp
 *    \brief   contains the implementation of the class CL4X0BECallFunction
 *
 *    \date    01/07/2004
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004
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

#include "be/l4/x0/L4X0BECallFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEIPC.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "be/BEMarshaller.h"

#include "TypeSpec-Type.h"
#include "Attribute-Type.h"


CL4X0BECallFunction::CL4X0BECallFunction()
 : CL4BECallFunction()
{
}

/** destroys the object */
CL4X0BECallFunction::~CL4X0BECallFunction()
{
}

/** \brief Writes the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * If we have a short IPC in both direction, we only need the result dope,
 * and two dummy dwords.
 */
void CL4X0BECallFunction::WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext)
{
    CL4BECallFunction::WriteVariableDeclaration(pFile, pContext); // msgdope, return var, msgbuffer
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (m_pComm->CheckProperty(this, COMM_PROP_USE_ASM, pContext) &&
        pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, DIRECTION_IN, pContext) &&
        pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, DIRECTION_OUT, pContext))
    {
        // write dummys
        CBENameFactory *pNF = pContext->GetNameFactory();
        string sDummy = pNF->GetDummyVariable(pContext);
        string sMWord = pNF->GetTypeName(TYPE_MWORD, false, pContext, 0);
        // write dummy variable
        *pFile << "\t" << sMWord << " " << sDummy <<
            " __attribute__((unused));\n";
    }
}

/* \brief write the marshalling code for the short IPC
 * \param pFile the file to write to
 * \param nStartOffset the offset where to start marshalling
 * \param bUseConstOffset true if nStartOffset can be used
 * \param pContext the context of the write operation
 *
 * If we have a short IPC into both direction, we skip the marshalling.
 */
void CL4X0BECallFunction::WriteMarshalling(CBEFile * pFile,  int nStartOffset,
                           bool & bUseConstOffset,  CBEContext * pContext)
{
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (!(m_pComm->CheckProperty(this, COMM_PROP_USE_ASM, pContext) &&
        pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, DIRECTION_IN, pContext) &&
        pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, DIRECTION_OUT, pContext)))
        CL4BECallFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
}

/* \brief write the unmarshalling code for the short IPC
 * \param pFile the file to write to
 * \param nStartOffset the offset where to start marshalling
 * \param bUseConstOffset true if nStartOffset can be used
 * \param pContext the context of the write operation
 *
 * If we have a short IPC in both direction, we can skip the unmarshalling.
 */
void CL4X0BECallFunction::WriteUnmarshalling(CBEFile * pFile,  int nStartOffset,
                            bool & bUseConstOffset,  CBEContext * pContext)
{
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (m_pComm->CheckProperty(this, COMM_PROP_USE_ASM, pContext) &&
        pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, DIRECTION_IN, pContext) &&
        pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, DIRECTION_OUT, pContext) &&
        !FindAttribute(ATTR_NOEXCEPTIONS))
        // unmarshal the exception from its word representation
        WriteEnvExceptionFromWord(pFile, pContext);
    else
        CL4BECallFunction::WriteUnmarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
}

/** \brief writes the invocation code for the short IPC
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * If we have a short IPC in both direction, we write assembler code
 * directly.
 */
void CL4X0BECallFunction::WriteInvocation(CBEFile * pFile,  CBEContext * pContext)
{
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (m_pComm->CheckProperty(this, COMM_PROP_USE_ASM, pContext) &&
        pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, DIRECTION_IN, pContext) &&
        pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, DIRECTION_OUT, pContext))
    {
        // skip send dope init
        WriteIPC(pFile, pContext);
        WriteIPCErrorCheck(pFile, pContext);
    }
    else
        CL4BECallFunction::WriteInvocation(pFile, pContext);
}

/** \brief prints the IPC call
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * We only write for PIC and !PIC && !PROFILE (anything else is currently not
 * supported.
 */
void CL4X0BECallFunction::WriteIPC(CBEFile * pFile,  CBEContext * pContext)
{
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (pContext->IsOptionSet(PROGRAM_TRACE_CLIENT))
    {
        // check if we use assembler
        bool bAssembler = m_pComm->CheckProperty(this, COMM_PROP_USE_ASM, pContext);
        // dump
        vector<CBEDeclarator*>::iterator iterCO = m_pCorbaObject->GetFirstDeclarator();
        CBEDeclarator *pObjName = *iterCO;
        string sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, false, pContext, 0);
        string sFunc = pContext->GetTraceClientFunc();
        if (sFunc.empty())
            sFunc = string("printf");
        pFile->PrintIndent("%s(\"%s: server %%x.%%x%s\", %s->id.task, %s->id.lthread);\n",
                            sFunc.c_str(), GetName().c_str(),
                            (sFunc=="LOG")?"":"\\n",
                            pObjName->GetName().c_str(),
                            pObjName->GetName().c_str());
        pFile->PrintIndent("%s(\"%s: with dw0=%%x, dw1=%%x, dw2=%%x%s\", ",
                            sFunc.c_str(), GetName().c_str(),
                            (sFunc=="LOG")?"":"\\n");
        if (bAssembler &&
            pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, GetSendDirection(), pContext) &&
            pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, GetReceiveDirection(), pContext))
        {
            pFile->Print("%s, ", m_sOpcodeConstName.c_str());                 /* EDX, 1 */
            CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
            if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetSendDirection(), false, pContext))
                pFile->Print("0");
            pFile->Print(", ");
            if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetSendDirection(), false, pContext))
                pFile->Print("0");
        }
        else
        {
            pFile->Print("(*((%s*)(&(", sMWord.c_str());
            pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
            pFile->Print("[0])))), ");
            pFile->Print("(*((%s*)(&(", sMWord.c_str());
            pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
            pFile->Print("[4])))), ");
            pFile->Print("(*((%s*)(&(", sMWord.c_str());
            pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
            pFile->Print("[8]))))");
        }
        pFile->Print(");\n");
    }
    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
        !pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, GetSendDirection(), pContext))
    {
        string sFunc = pContext->GetTraceClientFunc();
        if (sFunc.empty())
            sFunc = string("printf");
        pFile->PrintIndent("%s(\"%s before call%s\");\n", sFunc.c_str(),
                            GetName().c_str(), (sFunc=="LOG")?"":"\\n");
        pMsgBuffer->WriteDump(pFile, string(), pContext);
    }

    CL4BECallFunction::WriteIPC(pFile, pContext); // will call IPC class

    if (pContext->IsOptionSet(PROGRAM_TRACE_CLIENT))
    {
        string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
        string sFunc = pContext->GetTraceClientFunc();
        if (sFunc.empty())
            sFunc = string("printf");
        pFile->PrintIndent("%s(\"%s: return dope %%x (ipc error %%x)%s\", %s.msgdope, L4_IPC_ERROR(%s));\n",
                            sFunc.c_str(), GetName().c_str(),
                            (sFunc=="LOG")?"":"\\n",
                            sResult.c_str(), sResult.c_str());
    }
    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
        !pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, GetReceiveDirection(), pContext))
    {
        string sFunc = pContext->GetTraceClientFunc();
        if (sFunc.empty())
            sFunc = string("printf");
        pFile->PrintIndent("%s(\"%s after call%s\");\n", sFunc.c_str(),
                            GetName().c_str(), (sFunc=="LOG")?"":"\\n");
        pMsgBuffer->WriteDump(pFile, string(), pContext);
    }
}
