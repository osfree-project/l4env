/**
 *    \file    dice/src/be/l4/v2/L4V2BEReplyFunction.cpp
 *    \brief   contains the implementation of the class CL4V2BEReplyFunction
 *
 *    \date    06/01/2002
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

#include "L4V2BEReplyFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEClassFactory.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BEIPC.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "be/BEType.h"
#include "be/BEMarshaller.h"
#include "be/BECommunication.h"

#include "TypeSpec-Type.h"
#include "Attribute-Type.h"

CL4V2BEReplyFunction::CL4V2BEReplyFunction()
 : CL4BEReplyFunction()
{
}

/** destroys an instance of this class */
CL4V2BEReplyFunction::~CL4V2BEReplyFunction()
{
}


/** \brief writes the invocation code for the short IPC
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * If we have a short IPC in both direction, we write assembler code
 * directly.
 *
 * We have three branches: PIC, !PIC && PROFILE, and else. We always assume
 * gcc versions above 2.95 -> no 2.7 support. And we ignore the BIGASM support.
 *
 * If no message buffer is given this is a short IPC.
 */
void CL4V2BEReplyFunction::WriteInvocation(CBEFile * pFile,  CBEContext * pContext)
{
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);

    VERBOSE("CL4V2BEReplyFunction::WriteInvocation(%s) called\n",
        GetName().c_str());

    if (m_pComm->CheckProperty(this, COMM_PROP_USE_ASM, pContext) &&
        pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, GetSendDirection(), pContext) &&
        pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, GetReceiveDirection(), pContext))
    {
        int nSendDirection = GetSendDirection();
        if (pContext->IsOptionSet(PROGRAM_USE_SYMBOLS))
        {
            if (!pContext->HasSymbol("__PIC__") &&
                 pContext->HasSymbol("PROFILE"))
            {
                pMsgBuffer->WriteInitialization(pFile, TYPE_MSGDOPE_SEND, nSendDirection, pContext);
            }
        }
        else
        {
            pFile->Print("#if !defined(__PIC__) && defined(PROFILE) //2\n");
            pMsgBuffer->WriteInitialization(pFile, TYPE_MSGDOPE_SEND, nSendDirection, pContext);
            pFile->Print("#endif\n");
        }
        // skip send dope init
        if (!pContext->IsOptionSet(PROGRAM_NO_SEND_CANCELED_CHECK))
        {
            // sometimes it's possible to abort a call of a client.
            // but the client wants his call made, so we try until
            // the call completes
            pFile->PrintIndent("do\n");
            pFile->PrintIndent("{\n");
            pFile->IncIndent();
        }
        WriteIPC(pFile, pContext);
        if (!pContext->IsOptionSet(PROGRAM_NO_SEND_CANCELED_CHECK))
        {
            // now check if call has been canceled
            string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
            pFile->DecIndent();
            pFile->PrintIndent("} while ((L4_IPC_ERROR(%s) == L4_IPC_SEABORTED) || (L4_IPC_ERROR(%s) == L4_IPC_SECANCELED));\n",
                                sResult.c_str(), sResult.c_str());
        }
        WriteIPCErrorCheck(pFile, pContext);
    }
    else
        CL4BEReplyFunction::WriteInvocation(pFile, pContext);

    VERBOSE("CL4V2BEReplyFunction::WriteInvocation(%s) finished\n",
        GetName().c_str());
}

/* \brief write the marshalling code for the short IPC
 * \param pFile the file to write to
 * \param nStartOffset the offset where to start marshalling
 * \param bUseConstOffset true if nStartOffset can be used
 * \param pContext the context of the write operation
 *
 * If we have a short IPC into both direction, we skip the marshalling.
 *
 * The special about V2 is, that we have to marshal flexpage before the
 * opcode. They always have to come first.
 */
void CL4V2BEReplyFunction::WriteMarshalling(CBEFile * pFile,  int nStartOffset,
                           bool & bUseConstOffset,  CBEContext * pContext)
{
    VERBOSE("CL4V2BEReplyFunction::WriteMarshalling(%s) called\n",
        GetName().c_str());

    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    bool bUseAsmShort = m_pComm->CheckProperty(this, COMM_PROP_USE_ASM, pContext) &&
      pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, GetSendDirection(), pContext) &&
      pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, GetReceiveDirection(), pContext);
    bool bUseSymbols = pContext->IsOptionSet(PROGRAM_USE_SYMBOLS);
    // for asm short IPC we only need it if !PIC && PROFILE
    if (bUseAsmShort)
    {
        if (bUseSymbols)
        {
            if (!pContext->HasSymbol("__PIC__") && pContext->HasSymbol("PROFILE"))
                CL4BEReplyFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
        }
        else
        {
            *pFile << "#if !defined(__PIC__) && defined(PROFILE)\n";
            CL4BEReplyFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
            *pFile << "#endif\n";
        }
    }
    else
        CL4BEReplyFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);

    VERBOSE("CL4V2BEReplyFunction::WriteMarshalling(%s) finished\n",
        GetName().c_str());
}

/* \brief write the unmarshalling code for the short IPC
 * \param pFile the file to write to
 * \param nStartOffset the offset where to start marshalling
 * \param bUseConstOffset true if nStartOffset can be used
 * \param pContext the context of the write operation
 *
 * If we have a short IPC in both direction, we can skip the unmarshalling.
 */
void CL4V2BEReplyFunction::WriteUnmarshalling(CBEFile * pFile,  int nStartOffset,
                            bool & bUseConstOffset,  CBEContext * pContext)
{
    VERBOSE("CL4V2BEFunction::WriteUnmarshalling(%s) called\n",
        GetName().c_str());

    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (m_pComm->CheckProperty(this, COMM_PROP_USE_ASM, pContext) &&
      pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, GetSendDirection(), pContext))
    {
        // if short IPC consists of more than two parameters (bit-stuffing), then we
        // have to "unstuff" now
        // if !PIC && PROFILE we use "normal" IPC
        if (pContext->IsOptionSet(PROGRAM_USE_SYMBOLS))
        {
            if (!pContext->HasSymbol("__PIC__") &&
                 pContext->HasSymbol("PROFILE"))
            {
                CL4BEReplyFunction::WriteUnmarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
            }
        }
        else
        {
            pFile->Print("#if !defined(__PIC__) && defined(PROFILE)\n");
            CL4BEReplyFunction::WriteUnmarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
            pFile->Print("#endif\n");
        }
    }
    else
        CL4BEReplyFunction::WriteUnmarshalling(pFile, nStartOffset, bUseConstOffset, pContext);

    VERBOSE("CL4V2BEFunction::WriteUnmarshalling(%s) finished\n",
        GetName().c_str());
}

/** \brief Writes the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * If we have a short IPC in both direction, we only need the result dope,
 * and two dummy dwords.
 */
void CL4V2BEReplyFunction::WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext)
{
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    bool bUseAssembler = m_pComm->CheckProperty(this, COMM_PROP_USE_ASM, pContext);
    bool bShortIPC =
      pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, GetSendDirection(), pContext);
    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
        !bShortIPC)
        *pFile << "\tint _i;\n";
    if (bUseAssembler && bShortIPC)
    {
        // write dummys
        CBENameFactory *pNF = pContext->GetNameFactory();
        string sDummy = pNF->GetDummyVariable(pContext);
        string sMWord = pNF->GetTypeName(TYPE_MWORD, false, pContext, 0);
        string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);

        if (pContext->IsOptionSet(PROGRAM_USE_SYMBOLS))
        {
            if (pContext->HasSymbol("__PIC__"))
            {
                // write result variable
                *pFile << "\tl4_msgdope_t " << sResult << " = { msgdope: 0 };\n";
                *pFile << "\t" << sMWord << " " << sDummy << " __attribute__((unused));\n";
                if (!FindAttribute(ATTR_NOEXCEPTIONS))
                    // declare local exception variable
                    WriteExceptionWordDeclaration(pFile, true, pContext);
            }
            else
            {
                if (pContext->HasSymbol("PROFILE"))
                    CL4BEReplyFunction::WriteVariableDeclaration(pFile, pContext);
                else
                {
                    // write result variable
                    pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", sResult.c_str());
                    pFile->PrintIndent("%s %s = 0;\n", sMWord.c_str(), sDummy.c_str());
                    if (!FindAttribute(ATTR_NOEXCEPTIONS))
                        // declare local exception variable
                        WriteExceptionWordDeclaration(pFile, true, pContext);
                }
            }
        }
        else
        {
            // test if we need dummies
            pFile->Print("#if defined(__PIC__)\n");
            // write result variable
            pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", sResult.c_str());
            pFile->PrintIndent("%s %s __attribute__((unused));\n",
                                sMWord.c_str(), sDummy.c_str());
            if (!FindAttribute(ATTR_NOEXCEPTIONS))
                // declare local exception variable
                WriteExceptionWordDeclaration(pFile, true, pContext);

            pFile->Print("#else // !PIC\n");
            pFile->Print("#if defined(PROFILE)\n");
            CL4BEReplyFunction::WriteVariableDeclaration(pFile, pContext);

            pFile->Print("#else // !PROFILE\n");
            // write result variable
            pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", sResult.c_str());
            pFile->PrintIndent("%s %s = 0;\n", sMWord.c_str(), sDummy.c_str());
            if (!FindAttribute(ATTR_NOEXCEPTIONS))
                // declare local exception variable
                WriteExceptionWordDeclaration(pFile, true, pContext);
            pFile->Print("#endif // PROFILE\n");
            pFile->Print("#endif // !PIC\n");
            // if we have in either direction some bit-stuffing, we need more dummies
            // finished with declaration
        }
    }
    else
    {
        CL4BEReplyFunction::WriteVariableDeclaration(pFile, pContext);
        if (bUseAssembler)
        {
            // need dummies
            CBENameFactory *pNF = pContext->GetNameFactory();
            string sDummy = pNF->GetDummyVariable(pContext);
            string sMWord = pNF->GetTypeName(TYPE_MWORD, false, pContext, 0);
            if (pContext->IsOptionSet(PROGRAM_USE_SYMBOLS))
            {
                if (pContext->HasSymbol("__PIC__"))
                    *pFile << "\t" << sMWord << " " << sDummy << " __attribute__((unused));\n";
                else if (!pContext->HasSymbol("PROFILE"))
                    *pFile << "\t" << sMWord << " " << sDummy << " __attribute__((unused));\n";
            }
            else
            {
                *pFile << "#if defined(__PIC__)\n";
                *pFile << "\t" << sMWord << " " << sDummy << " __attribute__((unused));\n";
                *pFile << "#else // !PIC\n";
                *pFile << "#if !defined(PROFILE)\n";
                *pFile << "\t" << sMWord << " " << sDummy << " __attribute__((unused));\n";
                *pFile << "#endif // !PROFILE\n";
                *pFile << "#endif // !PIC\n";
            }
        }
    }
}

/** \brief writest the IPC call
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4V2BEReplyFunction::WriteIPC(CBEFile * pFile,  CBEContext * pContext)
{
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    bool bUseAssembler = m_pComm->CheckProperty(this, COMM_PROP_USE_ASM, pContext);
    bool bIsShortIPC =
      pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, GetSendDirection(), pContext);
    string sFunc = pContext->GetTraceClientFunc();
    if (sFunc.empty())
        sFunc = string("printf");

    if (pContext->IsOptionSet(PROGRAM_TRACE_CLIENT))
    {
        vector<CBEDeclarator*>::iterator iterCO = m_pCorbaObject->GetFirstDeclarator();
        CBEDeclarator *pObjName = *iterCO;
        string sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, false, pContext, 0);
        pFile->PrintIndent("%s(\"%s: server %%2X.%%X%s\", %s->id.task, %s->id.lthread);\n",
                            sFunc.c_str(),
                            (sFunc=="LOG")?"":GetName().c_str(),
                            (sFunc=="LOG")?"":"\\n",
                            pObjName->GetName().c_str(),
                            pObjName->GetName().c_str());
        pFile->PrintIndent("%s(\"%s: with dw0=%%x, dw1=%%x%s\", ",
                            sFunc.c_str(),
                            (sFunc=="LOG")?"":GetName().c_str(),
                            (sFunc=="LOG")?"":"\\n");
        if (bUseAssembler && bIsShortIPC)
        {
            pFile->Print("%s, ", m_sOpcodeConstName.c_str());                 /* EDX, 1 */
            CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
            if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetSendDirection(), false, pContext))
                pFile->Print("0");
        }
        else
        {
            pFile->Print("(*((%s*)(&(", sMWord.c_str());
            pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
            pFile->Print("[0])))), ");
            pFile->Print("(*((%s*)(&(", sMWord.c_str());
            pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
            pFile->Print("[4]))))");
        }
        pFile->Print(");\n");
    }
    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
        !bIsShortIPC)
    {
        pFile->PrintIndent("%s(\"%s before call%s\");\n", sFunc.c_str(),
                            GetName().c_str() ,(sFunc=="LOG")?"":"\\n");
        pMsgBuffer->WriteDump(pFile, string(), pContext);
    }

    // IPC Code start
    CL4BEReplyFunction::WriteIPC(pFile, pContext);
    // IPC Code stop

    string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    if (pContext->IsOptionSet(PROGRAM_TRACE_CLIENT))
    {
        pFile->PrintIndent("%s(\"%s: return dope %%x (ipc error %%x)%s\", %s.msgdope, L4_IPC_ERROR(%s));\n",
                            sFunc.c_str(), (sFunc=="LOG")?"":GetName().c_str(),
                            (sFunc=="LOG")?"":"\\n", sResult.c_str(),
                            sResult.c_str());
    }
    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
        !bIsShortIPC)
    {
        pFile->PrintIndent("%s(\"%s after call%s\");\n", sFunc.c_str(),
                            GetName().c_str(), (sFunc=="LOG")?"":"\\n");
        pMsgBuffer->WriteDump(pFile, sResult, pContext);
    }
}

/** \brief writes the variable initialization
 *  \param pFile the file to write to
 *  \param pContext the context of teh write operation
 *
 * If we use the asm short IPC code we do not init any variables (Its done during declaration).
 */
void CL4V2BEReplyFunction::WriteVariableInitialization(CBEFile * pFile,  CBEContext * pContext)
{
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (m_pComm->CheckProperty(this, COMM_PROP_USE_ASM, pContext) &&
      pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, GetSendDirection(), pContext) &&
      pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, GetReceiveDirection(), pContext))
    {
        if (pContext->IsOptionSet(PROGRAM_USE_SYMBOLS))
        {
            if (!pContext->HasSymbol("__PIC__") &&
                 pContext->HasSymbol("PROFILE"))
            {
                CL4BEReplyFunction::WriteVariableInitialization(pFile, pContext);
            }
        }
        else
        {
            pFile->Print("#if !defined(__PIC__) && defined(PROFILE) //1\n");
            CL4BEReplyFunction::WriteVariableInitialization(pFile, pContext);
            pFile->Print("#endif // !PIC && PROFILE\n");
        }
    }
    else
        CL4BEReplyFunction::WriteVariableInitialization(pFile, pContext);
}
