/**
 *    \file    dice/src/be/l4/x0/ia32/X0IA32IPC.cpp
 *    \brief   contains the declaration of the class CX0IA32IPC
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

#include "be/l4/x0/ia32/X0IA32IPC.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BEFile.h"
#include "be/BEFunction.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "be/BEType.h"
#include "be/BEMarshaller.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"


CX0IA32IPC::CX0IA32IPC()
 : CL4X0BEIPC()
{
}

/** destroys the IPC object */
CX0IA32IPC::~CX0IA32IPC()
{
}

/** \brief test if we can use assembler code
 *  \param pFunction the function to test
 *  \param pContext the context of the test
 *  \return true if assembler can be used
 */
bool CX0IA32IPC::UseAssembler(CBEFunction* pFunction,  CBEContext* pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
        return false;
    // test if the position size fits the parameters
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    if (!pMarshaller->TestPositionSize(pFunction, 4, pFunction->GetReceiveDirection(), false, false, 3 /* must fit 3 registers */, pContext))
    {
        delete pMarshaller;
        return false;
    }
    delete pMarshaller;
    // no objections!
    return true;
}

/** \brief writes the ipc code for the call
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteCall(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    if (UseAssembler(pFunction, pContext))
    {
        if (IsShortIPC(pFunction, pContext))
            WriteAsmShortCall(pFile, pFunction, pContext);
        else
            WriteAsmLongCall(pFile, pFunction, pContext);
    }
    else
        CL4X0BEIPC::WriteCall(pFile, pFunction, pContext);
}

/** \brief writes the assembler IPC code for the call IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write oepration
 */
void CX0IA32IPC::WriteAsmShortCall(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sDummy = pNF->GetDummyVariable(pContext);

    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    int nRecvDir = pFunction->GetReceiveDirection();
    int nSendDir = pFunction->GetSendDirection();

    // get exception
    CBETypedDeclarator *pException = pFunction->GetExceptionWord();
    CBEDeclarator *pExcDecl = 0;
    if (pException)
    {
        vector<CBEDeclarator*>::iterator iterExc = pException->GetFirstDeclarator();
        pExcDecl = *iterExc;
    }
    int nIndex = 1;

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx            \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n"); // save ebp no memory references ("m") after this point
    pFile->PrintIndent("\"movl   %%%%eax,%%%%ebx    \\n\\t\"\n");
    pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
    pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp    \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point
    pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx    \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebx            \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),                /* EAX, 0 */\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (");
    if (pExcDecl && !pFunction->FindAttribute(ATTR_NOEXCEPTIONS))
        pFile->Print("%s", pExcDecl->GetName().c_str());
    else
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nRecvDir, true, pContext))
            pFile->Print("%s", sDummy.c_str());
    pFile->Print("),                /* EDX, 1 */\n");
    pFile->PrintIndent("\"=c\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nRecvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),                /* ECX, 2 */\n");
    pFile->PrintIndent("\"=D\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nRecvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print(")                 /* EDI, 3 */\n");
    pFile->PrintIndent(":\n");
    // reset index
    nIndex = 1;
    if (pFunction->FindAttribute(ATTR_NOOPCODE))
    {
        pFile->PrintIndent("\"1\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nSendDir, false, pContext))
            pFile->Print("0");
        pFile->Print("),                 /* EDX, 1 */\n");
    }
    else
        pFile->PrintIndent("\"1\" (%s),                 /* EDX, 1 */\n", pFunction->GetOpcodeConstName().c_str());
    pFile->PrintIndent("\"2\" (%s),                 /* ECX, 2 */\n", sTimeout.c_str());
    pFile->PrintIndent("\"S\" (%s->raw),             /* ESI    */\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"0\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nSendDir, false, pContext))
        pFile->Print("0");
    pFile->Print("),                 /* EAX, 0 => EBX */\n");
    pFile->PrintIndent("\"3\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nSendDir, false, pContext))
        pFile->Print("0");
    pFile->Print(")                  /* EDI    */\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // reset index
    nIndex = 1;
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n"); // save ebp no memory references ("m") after this point
    pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
    pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp    \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),                /* EAX, 0 */\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (");
    if (pExcDecl && !pFunction->FindAttribute(ATTR_NOEXCEPTIONS))
        pFile->Print("%s", pExcDecl->GetName().c_str());
    else
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nRecvDir, true, pContext))
            pFile->Print("%s", sDummy.c_str());
    pFile->Print("),                /* EDX, 1 */\n");
    pFile->PrintIndent("\"=b\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nRecvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),                /* EBX, 2 */\n");
    pFile->PrintIndent("\"=c\" (%s),                /* ECX, 3 */\n", sDummy.c_str());
    pFile->PrintIndent("\"=D\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nRecvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print(")                 /* EDI, 4 */\n");
    pFile->PrintIndent(":\n");
    // reset index
    nIndex = 1;
    if (pFunction->FindAttribute(ATTR_NOOPCODE))
    {
        pFile->PrintIndent("\"1\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nSendDir, false, pContext))
            pFile->Print("0");
        pFile->Print("),                 /* EDX, 1 */\n");
    }
    else
        pFile->PrintIndent("\"1\" (%s),                 /* EDX, 1 */\n", pFunction->GetOpcodeConstName().c_str());
    pFile->PrintIndent("\"2\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nSendDir, false, pContext))
        pFile->Print("0");
    pFile->Print("),                 /* EBX, 2 */\n");
    pFile->PrintIndent("\"3\" (%s),                 /* ECX, 3 */\n", sTimeout.c_str());
    pFile->PrintIndent("\"S\" (%s->raw),             /* ESI    */\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"4\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nSendDir, false, pContext))
        pFile->Print("0");
    pFile->Print(")                  /* EDI, 4 */\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");
    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the assembler code for the long IPC call
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmLongCall(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    string sDummy = pNF->GetDummyVariable(pContext);
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    assert(pMsgBuffer);
    //int nSendDir = pFunction->GetSendDirection();
    //int nRecvDir = pFunction->GetReceiveDirection();

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx            \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n");
    pFile->PrintIndent("\"movl   %%%%edi,%%%%ebp      \\n\\t\"\n");
    pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi     \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx     \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx     \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx      \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebx            \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n");
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8])))),\n");
    pFile->PrintIndent("\"=S\" (%s)\n", sDummy.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (");
    /** do not test for short IPC here, because this function is not called
     * if it is a short IPC
     */
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print("),\n");
    pFile->PrintIndent("\"1\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("\"2\" (%s),\n", sTimeout.c_str());
    pFile->PrintIndent("\"3\" (");
    /** no test for short IPC (its a long call) */
    pFile->Print("((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n");
    pFile->PrintIndent("\"4\" (%s->raw)\n", pObjName->GetName().c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch

    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n"); // save ebp no memory references ("m") after this point
    pFile->PrintIndent("\"movl   %%%%ebx,%%%%ebp      \\n\\t\"\n");
    pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi     \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx     \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx     \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=b\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n");
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8])))),\n");
    pFile->PrintIndent("\"=c\" (%s)\n", sDummy.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"S\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"0\" (");
    /** no test for short IPC (its a long call) */
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print("),\n");
    pFile->PrintIndent("\"1\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("\"2\" (((int)");
    /** no test for short IPC (its a long call */
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n");
    pFile->PrintIndent("\"4\" (%s)\n", sTimeout.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the receive IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteReceive(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    if (UseAssembler(pFunction, pContext))
    {
        if (IsShortIPC(pFunction, pContext, pFunction->GetReceiveDirection()))
            WriteAsmShortReceive(pFile, pFunction, pContext);
        else
            WriteAsmLongReceive(pFile, pFunction, pContext);
    }
    else
        CL4X0BEIPC::WriteReceive(pFile, pFunction, pContext);
}

/** \brief writes the assembler version of the short receive IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmShortReceive(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    int nRcvDir = pFunction->GetReceiveDirection();
    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp   \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%eax    \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx  \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRcvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),                /* EDX, 1 */\n");
    pFile->PrintIndent("\"=c\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRcvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),                /* ECX, 2 */\n");
    pFile->PrintIndent("\"=S\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 3, 4, nRcvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print(")                 /* EDI, 3 */\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"2\" (%s)\n", sTimeout.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp   \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%eax    \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRcvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),                /* EDX, 1 */\n");
    pFile->PrintIndent("\"=b\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRcvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),                /* ECX, 2 */\n");
    pFile->PrintIndent("\"=S\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 3, 4, nRcvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print(")                 /* EDI, 3 */\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"c\" (%s)\n", sTimeout.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the assembler version of the long receive IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operaion
 */
void CX0IA32IPC::WriteAsmLongReceive(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();

    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"movl   %%%%eax,%%%%ebp  \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%eax    \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx  \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n");
    pFile->PrintIndent("\"=S\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8]))))\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n");
    pFile->PrintIndent("\"2\" (%s)\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"movl   %%%%eax,%%%%ebp  \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%eax    \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=b\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n");
    pFile->PrintIndent("\"=S\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8]))))\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n"); /* EAX => EBP */
    pFile->PrintIndent("\"2\" (%s)\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the reply and wait IPC code
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 *    \param bSendFlexpage true if a flexpage is sent
 *    \param bSendShortIPC true if a short ipc is sent
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteReplyAndWait(CBEFile* pFile,  CBEFunction* pFunction,  bool bSendFlexpage,  bool bSendShortIPC,  CBEContext* pContext)
{
    if (UseAssembler(pFunction, pContext))
    {
        if (bSendFlexpage)
        {
            if (bSendShortIPC)
                WriteAsmShortFpageReplyAndWait(pFile, pFunction, pContext);
            else
                WriteAsmLongFpageReplyAndWait(pFile, pFunction, pContext);
        }
        else
        {
            if (bSendShortIPC)
                WriteAsmShortReplyAndWait(pFile, pFunction, pContext);
            else
                WriteAsmLongReplyAndWait(pFile, pFunction, pContext);
        }
    }
    else
        CL4X0BEIPC::WriteReplyAndWait(pFile, pFunction, bSendFlexpage, bSendShortIPC, pContext);
}

/** \brief writes the short fpage IPC for the reply-and-wait IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmShortFpageReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);

    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl %%%%ebx         \\n\\t\"\n");
    pFile->PrintIndent("\"pushl %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%eax,%%%%ebp   \\n\\t\"\n");
    pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
    pFile->PrintIndent("\"movl  $2,%%%%eax      \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx   \\n\\t\"\n");
    pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent("\"popl  %%%%ebx         \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n");
    pFile->PrintIndent("\"=S\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8]))))\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(") | L4_IPC_OPEN_IPC),\n"); /* EAX => EBP (open ipc) */
    pFile->PrintIndent("\"1\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
    pFile->PrintIndent("\"2\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"3\" (%s->raw)\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%eax,%%%%ebp   \\n\\t\"\n"); // rcv descr
    pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx   \\n\\t\"\n"); // timeout
    pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
    pFile->PrintIndent("\"movl  $2,%%%%eax      \\n\\t\"\n"); /* fpage */
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=b\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n");
    pFile->PrintIndent("\"=S\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8]))))\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(") | L4_IPC_OPEN_IPC),\n"); /* EAX => EBP (open ipc) */
    pFile->PrintIndent("\"1\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
    pFile->PrintIndent("\"2\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"3\" (%s->raw)\n", pObjName->GetName().c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the long fpage IPC for the reply-and-wait IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmLongFpageReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);

    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl %%%%ebx         \\n\\t\"\n");
    pFile->PrintIndent("\"pushl %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%edi,%%%%ebp   \\n\\t\"\n");
    pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx   \\n\\t\"\n");
    pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent("\"popl  %%%%ebx         \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n");
    pFile->PrintIndent("\"=S\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8]))))\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(")|2),\n"); /* EAX => EBP (open ipc) */
    pFile->PrintIndent("\"1\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
    pFile->PrintIndent("\"2\" (%s),\n", sTimeout.c_str());
    pFile->PrintIndent("\"3\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"4\" (((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(") | L4_IPC_OPEN_IPC)\n"); /* EAX => EBP (open ipc) */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx   \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%edi,%%%%ebp   \\n\\t\"\n");
    pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=b\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n");
    pFile->PrintIndent("\"=S\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8]))))\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(")|2),\n"); /* EAX => EBP (open ipc) */
    pFile->PrintIndent("\"1\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
    pFile->PrintIndent("\"2\" (%s),\n", sTimeout.c_str());
    pFile->PrintIndent("\"3\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"4\" (((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(") | L4_IPC_OPEN_IPC)\n"); /* EAX => EBP (open ipc) */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the short IPC for the reply-and-wait IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmShortReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);

    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl %%%%ebx         \\n\\t\"\n");
    pFile->PrintIndent("\"pushl %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%eax,%%%%ebp   \\n\\t\"\n");
    pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
    pFile->PrintIndent("\"subl  %%%%eax,%%%%eax   \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx   \\n\\t\"\n");
    pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent("\"popl  %%%%ebx         \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n");
    pFile->PrintIndent("\"=S\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8]))))\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(") | L4_IPC_OPEN_IPC),\n");
    pFile->PrintIndent("\"1\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
    pFile->PrintIndent("\"2\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"3\" (%s->raw)\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx   \\n\\t\"\n"); // timeout
    pFile->PrintIndent("\"movl  %%%%eax,%%%%ebp   \\n\\t\"\n"); // rcv descr
    pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
    pFile->PrintIndent("\"subl  %%%%eax,%%%%eax   \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=b\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n");
    pFile->PrintIndent("\"=S\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8]))))\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(") | L4_IPC_OPEN_IPC),\n");
    pFile->PrintIndent("\"1\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
    pFile->PrintIndent("\"2\" (%s),\n", sTimeout.c_str()); /* EAX => ECX */
    pFile->PrintIndent("\"3\" (%s->raw)\n", pObjName->GetName().c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the long IPC for the reply-and-wait IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmLongReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);

    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl %%%%ebx         \\n\\t\"\n");
    pFile->PrintIndent("\"pushl %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%edi,%%%%ebp   \\n\\t\"\n");
    pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx   \\n\\t\"\n");
    pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent("\"popl  %%%%ebx         \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n");
    pFile->PrintIndent("\"=S\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8]))))\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print("),\n"); /* EAX => EBP (open ipc) */
    pFile->PrintIndent("\"1\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
    pFile->PrintIndent("\"2\" (%s),\n", sTimeout.c_str());
    pFile->PrintIndent("\"3\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"4\" (((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(") | L4_IPC_OPEN_IPC)\n"); /* EAX => EBP (open ipc) */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx   \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%edi,%%%%ebp   \\n\\t\"\n");
    pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=b\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n");
    pFile->PrintIndent("\"=S\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8]))))\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print("),\n"); /* EAX => EBP (open ipc) */
    pFile->PrintIndent("\"1\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
    pFile->PrintIndent("\"2\" (%s),\n", sTimeout.c_str()); /* EBX => ECX */
    pFile->PrintIndent("\"3\" (%s->raw),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"4\" (((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(") | L4_IPC_OPEN_IPC)\n"); /* EAX => EBP (open ipc) */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteSend(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    if (UseAssembler(pFunction, pContext))
    {
        if (IsShortIPC(pFunction, pContext, pFunction->GetSendDirection()))
            WriteAsmShortSend(pFile, pFunction, pContext);
        else
            WriteAsmLongSend(pFile, pFunction, pContext);
    }
    else
        CL4X0BEIPC::WriteSend(pFile, pFunction, pContext);
}

/** \brief write the assembler short send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmShortSend(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    int nSendDir = pFunction->GetSendDirection();

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%ebp  \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
    pFile->PrintIndent("\"movl   %%%%eax,%%%%ebx  \\n\\t\"\n");
    pFile->PrintIndent("\"subl   %%%%eax,%%%%eax  \\n\\t\"\n"); /* EAX = 0 (short IPC) */
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=c\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=S\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=D\" (%s)\n", sDummy.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSendDir, false, pContext))
        pFile->Print("0");
    pFile->Print("),\n"); /* EAX */
    pFile->PrintIndent("\"1\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"2\" (%s),\n", pFunction->GetOpcodeConstName().c_str()); /* EDX */
    pFile->PrintIndent("\"3\" (%s->raw),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"4\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nSendDir, false, pContext))
        pFile->Print("0");
    pFile->Print(")\n"); /* EDI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"movl   %%%%eax,%%%%ebx  \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%ebp  \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
    pFile->PrintIndent("\"subl   %%%%eax,%%%%eax  \\n\\t\"\n"); /* EAX = 0 (short IPC) */
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=c\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=S\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=D\" (%s)\n", sDummy.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSendDir, false, pContext))
        pFile->Print("0");
    pFile->Print("),\n"); /* EAX => EBX */ // kann man das direkt in EBX laden?
    pFile->PrintIndent("\"1\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"2\" (%s),\n", pFunction->GetOpcodeConstName().c_str()); /* EDX */
    pFile->PrintIndent("\"3\" (%s->raw),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"4\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nSendDir, false, pContext))
        pFile->Print("0");
    pFile->Print(")\n"); /* EDI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"ebx\", \"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief write the assembler long send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmLongSend(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    int nSendDir = pFunction->GetSendDirection();

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%ebp    \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=c\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=S\" (%s)\n", sDummy.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print("),\n");
    pFile->PrintIndent("\"1\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"2\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("\"3\" (%s->raw)\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"edi\", \"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%ebp  \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=c\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=S\" (%s)\n", sDummy.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print("),\n");
    pFile->PrintIndent("\"1\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"2\" (%s),\n", pFunction->GetOpcodeConstName().c_str()); /* EDX */
    pFile->PrintIndent("\"3\" (%s->raw),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"b\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSendDir, false, pContext))
        pFile->Print("0");
    pFile->Print("),\n"); /* EBX */
    pFile->PrintIndent("\"D\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSendDir, false, pContext))
        pFile->Print("0");
    pFile->Print(")\n"); /* EDI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"ebx\", \"edi\", \"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the reply IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteReply(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    if (UseAssembler(pFunction, pContext))
    {
        if (IsShortIPC(pFunction, pContext, pFunction->GetSendDirection()))
            WriteAsmShortReply(pFile, pFunction, pContext);
        else
            WriteAsmLongReply(pFile, pFunction, pContext);
    }
    else
        CL4X0BEIPC::WriteReply(pFile, pFunction, pContext);
}

/** \brief write the assembler short reply IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmShortReply(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    CBEDeclarator *pObjName = pFunction->GetObject()->GetDeclarator();
    int nSendDir = pFunction->GetSendDirection();
    CBETypedDeclarator *pException = pFunction->GetExceptionWord();
    string sException;
    if (pException && pException->GetDeclarator())
        sException = pException->GetDeclarator()->GetName();

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%ebp  \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
    pFile->PrintIndent("\"movl   %%%%eax,%%%%ebx  \\n\\t\"\n");
    pFile->PrintIndent("\"subl   %%%%eax,%%%%eax  \\n\\t\"\n"); /* EAX = 0 (short IPC) */
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=c\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=S\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=D\" (%s)\n", sDummy.c_str());
    pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"a\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSendDir, false, pContext))
        pFile->Print("0");
    pFile->Print("),\n"); /* EAX */
    pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"d\" (%s),\n", sException.c_str()); /* EDX */
    pFile->PrintIndent("\"S\" (%s->raw),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"D\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nSendDir, false, pContext))
        pFile->Print("0");
    pFile->Print(")\n"); /* EDI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"movl   %%%%eax,%%%%ebx  \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%ebp  \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
    pFile->PrintIndent("\"subl   %%%%eax,%%%%eax  \\n\\t\"\n"); /* EAX = 0 (short IPC) */
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=c\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=S\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=D\" (%s)\n", sDummy.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSendDir, false, pContext))
        pFile->Print("0");
    pFile->Print("),\n"); /* EAX => EBX */ // kann man das direkt in EBX laden?
    pFile->PrintIndent("\"1\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"2\" (%s),\n", sException.c_str()); /* EDX */
    pFile->PrintIndent("\"3\" (%s->raw),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"4\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nSendDir, false, pContext))
        pFile->Print("0");
    pFile->Print(")\n"); /* EDI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"ebx\", \"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief write the assembler long reply IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmLongReply(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    CBEDeclarator *pObjName = pFunction->GetObject()->GetDeclarator();
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    int nSendDir = pFunction->GetSendDirection();
    CBETypedDeclarator *pException = pFunction->GetExceptionWord();
    string sException;
    if (pException && pException->GetDeclarator())
        sException = pException->GetDeclarator()->GetName();

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%ebp    \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=c\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=S\" (%s)\n", sDummy.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"a\" (");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print("),\n");
    pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"d\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("\"S\" (%s->raw)\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"edi\", \"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%ebp  \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=c\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=S\" (%s)\n", sDummy.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"a\" (");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print("),\n");
    pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"d\" (%s),\n", sException.c_str()); /* EDX */
    pFile->PrintIndent("\"S\" (%s->raw),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"b\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSendDir, false, pContext))
        pFile->Print("0");
    pFile->Print("),\n"); /* EBX */
    pFile->PrintIndent("\"D\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSendDir, false, pContext))
        pFile->Print("0");
    pFile->Print(")\n"); /* EDI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"ebx\", \"edi\", \"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the wait IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bAllowShortIPC true if short IPC is allowed
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteWait(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    if (UseAssembler(pFunction, pContext))
    {
        if (IsShortIPC(pFunction, pContext, pFunction->GetReceiveDirection()))
            WriteAsmShortWait(pFile, pFunction, pContext);
        else
            WriteAsmLongWait(pFile, pFunction, pContext);
    }
    else
        CL4X0BEIPC::WriteWait(pFile, pFunction, pContext);
}

/** \brief writes the assembler version of the short IPC wait code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmShortWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);

    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    int nRcvDir = pFunction->GetReceiveDirection();

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx            \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n"); // save ebp no memory references ("m") after this point
    pFile->PrintIndent("\"movl   $1,%%%%ebp           \\n\\t\"\n"); /* rcv short ipc, open wait */
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
    pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx      \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebx            \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRcvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),                /* EDX, 1 */\n");
    pFile->PrintIndent("\"=c\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRcvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),                /* ECX, 2 */\n");
    pFile->PrintIndent("\"=S\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 3, 4, nRcvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print(")                 /* EDI, 3 */\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (L4_IPC_NIL_DESCRIPTOR),\n");
    pFile->PrintIndent("\"1\" (%s)\n", sTimeout.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n"); // save ebp no memory references ("m") after this point
    pFile->PrintIndent("\"movl   %%%%edx,%%%%ecx      \\n\\t\"\n"); /* timeout, ecx */
    pFile->PrintIndent("\"movl   $1,%%%%ebp           \\n\\t\"\n"); /* rcv short ipc, open wait */
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRcvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),                /* EDX, 1 */\n");
    pFile->PrintIndent("\"=b\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRcvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),                /* ECX, 2 */\n");
    pFile->PrintIndent("\"=S\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 3, 4, nRcvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print(")                 /* EDI, 3 */\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (L4_IPC_NIL_DESCRIPTOR),\n");
    pFile->PrintIndent("\"1\" (%s)\n", sTimeout.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"ecx\", \"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the assembler version of the long IPC wait code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmLongWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);

    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx            \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n"); // save ebp no memory references ("m") after this point
    pFile->PrintIndent("\"movl   %%%%edx,%%%%ebp      \\n\\t\"\n"); /* rcv msg, open wait */
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
    pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx      \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebx            \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n");
    pFile->PrintIndent("\"=S\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8]))))\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (L4_IPC_NIL_DESCRIPTOR),\n");
    pFile->PrintIndent("\"1\" (((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(") | L4_IPC_OPEN_IPC),\n");
    pFile->PrintIndent("\"2\" (%s)\n", sTimeout.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n"); // save ebp no memory references ("m") after this point
    pFile->PrintIndent("\"movl   %%%%edx,%%%%ecx      \\n\\t\"\n"); /* timeout, ecx */
    pFile->PrintIndent("\"movl   %%%%ebx,%%%%ebp      \\n\\t\"\n"); /* rcv short ipc, open wait */
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=b\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n");
    pFile->PrintIndent("\"=S\" (%s->raw),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8]))))\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (L4_IPC_NIL_DESCRIPTOR),\n");
    pFile->PrintIndent("\"1\" (%s),\n", sTimeout.c_str());
    pFile->PrintIndent("\"2\" (((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(") | L4_IPC_OPEN_IPC)\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"ecx\", \"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}
