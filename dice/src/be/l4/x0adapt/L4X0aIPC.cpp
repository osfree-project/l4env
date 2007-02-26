/**
 *    \file    dice/src/be/l4/x0adapt/L4X0aIPC.cpp
 *    \brief   contains the implementation of the class CL4X0aIPC
 *
 *    \date    08/14/2002
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
#include "be/l4/x0adapt/L4X0aIPC.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEFile.h"
#include "be/BEFunction.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "be/BEMarshaller.h"
#include "be/BEWaitAnyFunction.h"

#include "TypeSpec-Type.h"
#include "Attribute-Type.h"

CL4X0aIPC::CL4X0aIPC()
 : CL4BEIPC()
{
}

/** destroys the IPC object */
CL4X0aIPC::~CL4X0aIPC()
{
}

/** \brief test if we can use assembler code
 *  \param pFunction the function using the IPC class
 *  \param pContext the context of the IPC
 *
 * Assembler can be used if no usage of C bindings is enforced,
 * if the optimization level is at least 2 and if the parameters
 * fit into the registers.
 */
bool CL4X0aIPC::UseAssembler(CBEFunction* pFunction,  CBEContext* pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
        return false;
    // test if the position size fits the parameters
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    if (!pMarshaller->TestPositionSize(pFunction, 4, pFunction->GetReceiveDirection(), false, false, 3 /* must fit 3 registers*/, pContext))
    {
        delete pMarshaller;
        return false;
    }
    delete pMarshaller;
    // no objections!
    return true;
}

/** \brief writes the wait IPC
 *  \param pFile the file to write the IPC to
 *  \param pFunction the function to write the IPC for
 *  \param pContext the context of the IPC writing
 */
void CL4X0aIPC::WriteWait(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    bool bAllowShortIPC = true;
    /* if any message can be received, we cannot use short IPC */
    if (dynamic_cast<CBEWaitAnyFunction*>(pFunction))
        bAllowShortIPC = false;
    if (UseAssembler(pFunction, pContext))
    {
        if (IsShortIPC(pFunction, pContext, pFunction->GetReceiveDirection()) &&
            bAllowShortIPC)
            WriteAsmShortWait(pFile, pFunction, pContext);
        else
            WriteAsmLongWait(pFile, pFunction, pContext);
    }
    else
        CL4BEIPC::WriteWait(pFile, pFunction, pContext);
}

/** \brief write the assembler version of a short IPC for wait
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4X0aIPC::WriteAsmShortWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);

    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    int nRecvDir = pFunction->GetReceiveDirection();

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx            \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n"); // save ebp no memory references ("m") after this point
    pFile->PrintIndent("\"pushl  %%%%esi            \\n\\t\"\n"); // save thread id address
    pFile->PrintIndent("\"movl   $1,%%%%ebp         \\n\\t\"\n"); // rcv short ipc, open wait
    pFile->PrintIndent("\"movl   $-1,%%%%eax        \\n\\t\"\n"); // no send
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl   %%%%edi,%%%%ecx    \\n\\t\"\n"); // save dw0
    pFile->PrintIndent("FromId32_Esi\n");
    pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // get addr of thread id
    pFile->PrintIndent("\"movl   %%%%edi,4(%%%%ebp) \\n\\t\"\n");
    pFile->PrintIndent("\"movl   %%%%esi,(%%%%ebp)  \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
    pFile->PrintIndent("\"movl   %%%%ebx,%%%%edi    \\n\\t\"\n"); // save dw2
    pFile->PrintIndent("\"popl   %%%%ebx            \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRecvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),\n"); // EDX -> dw0
    pFile->PrintIndent("\"=c\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 3, 4, nRecvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),\n"); // ECX (EDI) -> dw2
    pFile->PrintIndent("\"=S\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=D\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRecvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print(")\n"); // EDI (EBX) -> dw1
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"2\" (%s),\n", sTimeout.c_str());
    pFile->PrintIndent("\"3\" (%s)\n", pObjName->GetName().c_str());
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
    pFile->PrintIndent("\"movl   $1,%%%%ebp         \\n\\t\"\n"); // rcv short ipc, open wait
    pFile->PrintIndent("\"movl   $-1,%%%%eax        \\n\\t\"\n"); // no send
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl   %%%%edi,%%%%ecx    \\n\\t\"\n"); // save dw0
    pFile->PrintIndent("FromId32_Esi\n");
    pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=b\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRecvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),\n"); // EBX -> dw1
    pFile->PrintIndent("\"=c\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 3, 4, nRecvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),\n"); // ECX (EDI) -> dw2
    pFile->PrintIndent("\"=d\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRecvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),\n"); // EDX -> dw0
    pFile->PrintIndent("\"=S\" (%s->lh.low),\n", pObjName->GetName().c_str()); // ESI
    pFile->PrintIndent("\"=D\" (%s->lh.high)\n", pObjName->GetName().c_str()); // EDI
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"2\" (%s)\n", sTimeout.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the long IPC assembler code for wait
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4X0aIPC::WriteAsmLongWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx            \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n"); // save ebp no memory references ("m") after this point
    pFile->PrintIndent("\"pushl  %%%%esi            \\n\\t\"\n");
    pFile->PrintIndent("\"movl   %%%%eax,%%%%ebp    \\n\\t\"\n"); // rcv msg, open wait
    pFile->PrintIndent("\"movl   $-1,%%%%eax        \\n\\t\"\n"); // no send
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl   %%%%edi,%%%%ecx    \\n\\t\"\n"); // save dw0 in ecx
    pFile->PrintIndent("FromId32_Esi\n");
    pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // thread id addr
    pFile->PrintIndent("\"movl   %%%%edi,4(%%%%ebp) \\n\\t\"\n");
    pFile->PrintIndent("\"movl   %%%%esi,(%%%%ebp)  \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
    pFile->PrintIndent("\"movl   %%%%ebx,%%%%edi    \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebx            \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
    pFile->PrintIndent("\"=S\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4]))))\n"); // EDI (EBX) -> dw1
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(") | L4_IPC_OPEN_IPC),\n");
    pFile->PrintIndent("\"2\" (%s),\n", sTimeout.c_str());
    pFile->PrintIndent("\"3\" (%s)\n", pObjName->GetName().c_str());
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
    pFile->PrintIndent("\"movl   %%%%eax,%%%%ebp      \\n\\t\"\n"); // rcv msg descr
    pFile->PrintIndent("\"movl   $-1,%%%%eax        \\n\\t\"\n"); // no send
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl   %%%%edi,%%%%ecx    \\n\\t\"\n"); // save dw0
    pFile->PrintIndent("FromId32_Esi\n");
    pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=b\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n"); // EBX -> dw1
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
    pFile->PrintIndent("\"=S\" (%s->lh.low),\n", pObjName->GetName().c_str()); // ESI
    pFile->PrintIndent("\"=D\" (%s->lh.high)\n", pObjName->GetName().c_str()); // EDI
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (((int)");
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

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the replay-and-wait IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bSendFlexpage true if a flexpages is sent
 *  \param bSendShortIPC true if a short IPC is sent
 *  \param pContext the context of the IPC writing
 */
void CL4X0aIPC::WriteReplyAndWait(CBEFile* pFile,  CBEFunction* pFunction,  bool bSendFlexpage,  bool bSendShortIPC,  CBEContext* pContext)
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
        CL4BEIPC::WriteReplyAndWait(pFile, pFunction, bSendFlexpage, bSendShortIPC, pContext);
}

/** \brief writes the short IPC with flexpage for reply and wait
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the IPC write
 */
void CL4X0aIPC::WriteAsmShortFpageReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    string sDummy = pNF->GetDummyVariable(pContext);

    // l4_fpage_t + 2*l4_msgdope_t
    int nMsgBase = pContext->GetSizes()->GetSizeOfEnvType("l4_fpage_t") +
                pContext->GetSizes()->GetSizeOfEnvType("l4_msgdope_t")*2;
    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl %%%%ebx             \\n\\t\"\n");
    pFile->PrintIndent("\"pushl %%%%ebp             \\n\\t\"\n");
    pFile->PrintIndent("\"pushl %%%%esi             \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi   \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi   \\n\\t\"\n");
    pFile->PrintIndent("ToId32_EdiEsi\n");
    pFile->PrintIndent("\"movl  %%%%eax,%%%%ebp     \\n\\t\"\n");
    pFile->PrintIndent("\"orl   $1,%%%%ebp          \\n\\t\"\n"); // open wait
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi  \\n\\t\"\n", nMsgBase+8); // dw2 => edi
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+4); // dw1 => ebx
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase); // dw0 => edx
    pFile->PrintIndent("\"movl  $2,%%%%eax          \\n\\t\"\n"); // short fpage ipc
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl  %%%%edi,%%%%ecx     \\n\\t\"\n"); // save dw0 in ecx
    pFile->PrintIndent("FromId32_Esi\n");
    pFile->PrintIndent("\"popl  %%%%ebp             \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%esi,(%%%%ebp)   \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%edi,4(%%%%ebp)  \\n\\t\"\n");
    pFile->PrintIndent("\"popl  %%%%ebp             \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%ebx,%%%%esi     \\n\\t\"\n");
    pFile->PrintIndent("\"popl  %%%%ebx         \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
    pFile->PrintIndent("\"=S\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n"); // ESI (EBX) -> dw1
    pFile->PrintIndent("\"=D\" (%s)\n", sDummy.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"a\" (");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print("),\n");
    pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"S\" (%s)\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl %%%%ebp             \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%eax,%%%%ebp     \\n\\t\"\n"); // rcv descr
    pFile->PrintIndent("\"orl   $1,%%%%ebp          \\n\\t\"\n"); // open wait
    pFile->PrintIndent("ToId32_EdiEsi\n"); // EDI,ESI => ESI
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi  \\n\\t\"\n", nMsgBase+8); // dw2 => edi
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+4); // dw1 => ebx
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase); // dw0 => edx
    pFile->PrintIndent("\"movl  $2,%%%%eax          \\n\\t\"\n"); // fpage
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl  %%%%edi,%%%%ecx     \\n\\t\"\n"); // edi => ecx (dw0)
    pFile->PrintIndent("FromId32_Esi\n");
    pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=b\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n"); // EBX -> dw1
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
    pFile->PrintIndent("\"=S\" (%s->lh.low),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (%s->lh.high)\n", pObjName->GetName().c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"a\" (");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print("),\n"); /* EAX */
    pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"D\" (%s->lh.high)\n", pObjName->GetName().c_str()); /* EDI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the long IPC with flexpage for reply and wait
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the IPC write
 */
void CL4X0aIPC::WriteAsmLongFpageReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    // l4_fpage_t + 2*l4_msgdope_t
    int nMsgBase = pContext->GetSizes()->GetSizeOfEnvType("l4_fpage_t") +
                pContext->GetSizes()->GetSizeOfEnvType("l4_msgdope_t")*2;
    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    string sDummy = pNF->GetDummyVariable(pContext);
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl %%%%ebx             \\n\\t\"\n");
    pFile->PrintIndent("\"pushl %%%%ebp             \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%eax,%%%%ebp     \\n\\t\"\n");
    pFile->PrintIndent("\"orl   $1,%%%%ebp          \\n\\t\"\n");
    pFile->PrintIndent("\"pushl %%%%esi             \\n\\t\"\n"); // save thread id address
    pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi   \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi   \\n\\t\"\n");
    pFile->PrintIndent("ToId32_EdiEsi\n");
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase); // dw0 -> edx
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+4); // dw1 -> ebx
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi  \\n\\t\"\n", nMsgBase+8);   // dw2 -> edi
    pFile->PrintIndent("\"orl   $2,%%%%eax          \\n\\t\"\n"); // fpage ipc
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl  %%%%edi,%%%%ecx     \\n\\t\"\n"); // save dw0 in ecx
    pFile->PrintIndent("FromId32_Esi\n");
    pFile->PrintIndent("\"popl  %%%%ebp             \\n\\t\"\n"); // thread id address
    pFile->PrintIndent("\"movl  %%%%esi,(%%%%ebp)   \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%edi,4(%%%%ebp)  \\n\\t\"\n");
    pFile->PrintIndent("\"popl  %%%%ebp             \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%ebx,%%%%esi     \\n\\t\"\n"); // save ebx in esi
    pFile->PrintIndent("\"popl  %%%%ebx             \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
    pFile->PrintIndent("\"=S\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n"); // ESI (EBX) -> dw1
    pFile->PrintIndent("\"=D\" (%s)\n", sDummy.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"a\" (");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print("),\n"); /* EAX */
    pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str());
    pFile->PrintIndent("\"S\" (%s)\n", pObjName->GetName().c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl %%%%ebp             \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%eax,%%%%ebp     \\n\\t\"\n"); // rcv descr
    pFile->PrintIndent("\"orl   $1,%%%%ebp          \\n\\t\"\n"); // open wait
    pFile->PrintIndent("ToId32_EdiEsi\n");
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi  \\n\\t\"\n", nMsgBase+8); // dw2 -> edi
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+4); // dw1 -> ebx
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase); // dw0 -> edx
    pFile->PrintIndent("\"orl   $2,%%%%eax          \\n\\t\"\n"); // fpage ipc
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl  %%%%edi,%%%%ecx     \\n\\t\"\n"); // save edi in ecx
    pFile->PrintIndent("FromId32_Esi\n");
    pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
    pFile->PrintIndent("\"=b\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n"); // EBX -> dw1
    pFile->PrintIndent("\"=S\" (%s->lh.low),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (%s->lh.high)\n", pObjName->GetName().c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"a\" (");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print("),\n"); /* EAX */
    pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"D\" (%s->lh.high)\n", pObjName->GetName().c_str()); /* EDI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the short IPC without flexpage for reply and wait
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the IPC write
 */
void CL4X0aIPC::WriteAsmShortReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    string sDummy = pNF->GetDummyVariable(pContext);

    // l4_fpage_t + 2*l4_msgdope_t
    int nMsgBase = pContext->GetSizes()->GetSizeOfEnvType("l4_fpage_t") +
                pContext->GetSizes()->GetSizeOfEnvType("l4_msgdope_t")*2;
    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;

    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl %%%%ebx             \\n\\t\"\n");
    pFile->PrintIndent("\"pushl %%%%ebp             \\n\\t\"\n");
    pFile->PrintIndent("\"pushl %%%%esi             \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi   \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi   \\n\\t\"\n");
    pFile->PrintIndent("ToId32_EdiEsi\n");
    pFile->PrintIndent("\"movl  %%%%eax,%%%%ebp     \\n\\t\"\n");
    pFile->PrintIndent("\"orl   $1,%%%%ebp          \\n\\t\"\n"); // open wait
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi  \\n\\t\"\n", nMsgBase+8); // dw2 => edi
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+4); // dw1 => ebx
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase); // dw0 => edx
    pFile->PrintIndent("\"subl  %%%%eax,%%%%eax     \\n\\t\"\n"); // short ipc
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl  %%%%edi,%%%%ecx     \\n\\t\"\n"); // save dw0 in ecx
    pFile->PrintIndent("FromId32_Esi\n");
    pFile->PrintIndent("\"popl  %%%%ebp             \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%esi,(%%%%ebp)   \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%edi,4(%%%%ebp)  \\n\\t\"\n");
    pFile->PrintIndent("\"popl  %%%%ebp             \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%ebx,%%%%esi     \\n\\t\"\n");
    pFile->PrintIndent("\"popl  %%%%ebx         \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
    pFile->PrintIndent("\"=S\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n"); // ESI (EBX) -> dw1
    pFile->PrintIndent("\"=D\" (%s)\n", sDummy.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"a\" (");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print("),\n");
    pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"S\" (%s)\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl %%%%ebp             \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%eax,%%%%ebp     \\n\\t\"\n"); // rcv descr
    pFile->PrintIndent("\"orl   $1,%%%%ebp          \\n\\t\"\n"); // open wait
    pFile->PrintIndent("ToId32_EdiEsi\n"); // EDI,ESI => ESI
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi  \\n\\t\"\n", nMsgBase+8); // dw2 => edi
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+4); // dw1 => ebx
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase); // dw0 => edx
    pFile->PrintIndent("\"subl  %%%%eax,%%%%eax     \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl  %%%%edi,%%%%ecx     \\n\\t\"\n"); // edi => ecx (dw0)
    pFile->PrintIndent("FromId32_Esi\n");
    pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=b\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n"); // EBX -> dw1
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
    pFile->PrintIndent("\"=S\" (%s->lh.low),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (%s->lh.high)\n", pObjName->GetName().c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"a\" (");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print("),\n"); /* EAX */
    pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"D\" (%s->lh.high)\n", pObjName->GetName().c_str()); /* EDI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the long IPC without flexpage for reply and wait
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the IPC write
 */
void CL4X0aIPC::WriteAsmLongReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    // l4_fpage_t + 2*l4_msgdope_t
    int nMsgBase = pContext->GetSizes()->GetSizeOfEnvType("l4_fpage_t") +
                pContext->GetSizes()->GetSizeOfEnvType("l4_msgdope_t")*2;
    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    string sDummy = pNF->GetDummyVariable(pContext);
    CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType *)pFunction->GetMessageBuffer();

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl %%%%ebx             \\n\\t\"\n");
    pFile->PrintIndent("\"pushl %%%%ebp             \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%eax,%%%%ebp     \\n\\t\"\n"); // save msgbuf in ebp
    pFile->PrintIndent("\"orl   $1,%%%%ebp          \\n\\t\"\n"); // open receive
    pFile->PrintIndent("\"pushl %%%%esi             \\n\\t\"\n"); // save thread id address
    pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi   \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi   \\n\\t\"\n");
    pFile->PrintIndent("ToId32_EdiEsi\n");
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase);   // dw0 -> edx
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+4); // dw1 -> ebx
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi  \\n\\t\"\n", nMsgBase+8); // dw2 -> edi
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl  %%%%edi,%%%%ecx     \\n\\t\"\n"); // save dw0 in ecx
    pFile->PrintIndent("FromId32_Esi\n");
    pFile->PrintIndent("\"popl  %%%%ebp             \\n\\t\"\n"); // thread id address
    pFile->PrintIndent("\"movl  %%%%esi,(%%%%ebp)   \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%edi,4(%%%%ebp)  \\n\\t\"\n");
    pFile->PrintIndent("\"popl  %%%%ebp             \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%ebx,%%%%esi     \\n\\t\"\n"); // save ebx in esi
    pFile->PrintIndent("\"popl  %%%%ebx             \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
    pFile->PrintIndent("\"=S\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n"); // ESI (EBX) -> dw1
    pFile->PrintIndent("\"=D\" (%s)\n", sDummy.c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"a\" (");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print("),\n"); /* EAX */
    pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str());
    pFile->PrintIndent("\"S\" (%s)\n", pObjName->GetName().c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl %%%%ebp             \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%eax,%%%%ebp     \\n\\t\"\n"); // rcv descr
    pFile->PrintIndent("\"orl   $1,%%%%ebp          \\n\\t\"\n"); // open wait
    pFile->PrintIndent("ToId32_EdiEsi\n");
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi  \\n\\t\"\n", nMsgBase+8); // dw2 -> edi
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+4); // dw1 -> ebx
    pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase); // dw0 -> edx
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl  %%%%edi,%%%%ecx     \\n\\t\"\n"); // save edi in ecx
    pFile->PrintIndent("FromId32_Esi\n");
    pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
    pFile->PrintIndent("\"=b\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n"); // EBX -> dw1
    pFile->PrintIndent("\"=S\" (%s->lh.low),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"=D\" (%s->lh.high)\n", pObjName->GetName().c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"a\" (");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print("),\n"); /* EAX */
    pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"D\" (%s->lh.high)\n", pObjName->GetName().c_str()); /* EDI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the IPC call code
 *  \param pFile the file to write the code to
 *  \param pFunction the function to write the code for
 *  \param pContext the context of the writing
 */
void CL4X0aIPC::WriteCall(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    if (UseAssembler(pFunction, pContext))
    {
        if (IsShortIPC(pFunction, pContext)) // both directions
            WriteAsmShortCall(pFile, pFunction, pContext);
        else
            WriteAsmLongCall(pFile, pFunction, pContext);
    }
    else
        CL4BEIPC::WriteCall(pFile, pFunction, pContext);
}

/** \brief writes the assembler version of a short IPC call
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the writing
 */
void CL4X0aIPC::WriteAsmShortCall(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();
    int nSendDir = pFunction->GetSendDirection();
    bool bSendFlexpage = pMsgBuffer->GetCount(TYPE_FLEXPAGE, nSendDir) > 0;

    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    int nRecvDir = pFunction->GetReceiveDirection();
    string sOpcodeConstName = pFunction->GetOpcodeConstName();

    // get exception
    CBETypedDeclarator *pException = pFunction->GetExceptionWord();
    CBEDeclarator *pExcDecl = 0;
    if (pException)
    {
        vector<CBEDeclarator*>::iterator iterExc = pException->GetFirstDeclarator();
        pExcDecl = *iterExc;
    }
    int nIndex = 1;

    // to increase the confusing code:
    // if we have PROGRAM_USE_SYMBOLS set, we test for __PIC__ and PROFILE
    // then we write the three parts bPIC, bPROF, bNPROF if they are set
    bool bPIC = true;
    bool bPROF = true;
    bool bNPROF = true;
    bool bSymbols = pContext->IsOptionSet(PROGRAM_USE_SYMBOLS);
    if (bSymbols)
    {
        bPIC = pContext->HasSymbol("__PIC__");
        bPROF = pContext->HasSymbol("PROFILE") && !bPIC;
        bNPROF = !bPROF && !bPIC;
    }

    if (!bSymbols)
        pFile->Print("#if defined(__PIC__)\n");
    if (bPIC)
    {
        // PIC branch
        pFile->PrintIndent("asm volatile(\n");
        pFile->IncIndent();
        pFile->PrintIndent("\"pushl  %%%%ebx            \\n\\t\"\n");
        pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n"); // save ebp no memory references ("m") after this point
        pFile->PrintIndent("\"movl   %%%%edi,%%%%ebx    \\n\\t\"\n"); // dw1
        pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi   \\n\\t\"\n");
        pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi   \\n\\t\"\n");
        pFile->PrintIndent("ToId32_EdiEsi\n"); // EDI,ESI => ESI
        pFile->PrintIndent("\"movl   %%%%eax,%%%%edi    \\n\\t\"\n"); // dw2
        if (bSendFlexpage)
            pFile->PrintIndent("\"movl  $2,%%%%eax        \\n\\t\"\n");
        else
            pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n"); // short ipc send
        pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp    \\n\\t\"\n"); // short ipc recv
        pFile->PrintIndent("IPC_SYSENTER\n");
        pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx    \\n\\t\"\n");
        pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point
        pFile->PrintIndent("\"popl   %%%%ebx            \\n\\t\"\n");
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
        pFile->PrintIndent("\"=d\" (");
        // XXX FIXME: what about short fpage IPC
        if (pExcDecl && !pFunction->FindAttribute(ATTR_NOEXCEPTIONS))
            pFile->Print("%s", pExcDecl->GetName().c_str());
        else
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nRecvDir, true, pContext))
                pFile->Print("%s", sDummy.c_str());
        pFile->Print("),\n"); // EDX -> dw0
        pFile->PrintIndent("\"=c\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nRecvDir, true, pContext))
            pFile->Print("%s", sDummy.c_str());
        pFile->Print("),\n"); // ECX (EBX) -> dw1
        pFile->PrintIndent("\"=D\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nRecvDir, true, pContext))
            pFile->Print("%s", sDummy.c_str());
        pFile->Print("),\n"); // EDI -> dw2
        pFile->PrintIndent("\"=S\" (%s)\n", sDummy.c_str());
        pFile->PrintIndent(":\n");
        // reset index
        nIndex = 1;
        if (pFunction->FindAttribute(ATTR_NOOPCODE))
        {
            pFile->PrintIndent("\"1\" (");
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nSendDir, false, pContext))
                pFile->Print("0");
            pFile->Print("),\n"); // EDX -> dw0
        }
        else
            pFile->PrintIndent("\"1\" (%s),\n", sOpcodeConstName.c_str()); // EDX -> dw0
        pFile->PrintIndent("\"2\" (%s),\n", sTimeout.c_str());
        pFile->PrintIndent("\"3\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nSendDir, false, pContext))
            pFile->Print("0");
        pFile->Print("),\n"); // EDI (EBX) -> dw1
        pFile->PrintIndent("\"0\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nSendDir, false, pContext))
            pFile->Print("0");
        pFile->Print("),\n"); // EAX (EDI) -> dw2
        pFile->PrintIndent("\"4\" (%s)\n", pObjName->GetName().c_str());
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"memory\"\n");
        pFile->DecIndent();
        pFile->PrintIndent(");\n");
    }
    if (!bSymbols)
    {
        pFile->Print("#else // !PIC\n");
        pFile->Print("#if !defined(PROFILE)\n");
    }
    if (bNPROF)
    {
        // reset index
        nIndex = 1;
        // !PIC branch
        pFile->PrintIndent("asm volatile(\n");
        pFile->IncIndent();
        pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n"); // save ebp no memory references ("m") after this point
        pFile->PrintIndent("\"movl  0(%%%%edi),%%%%esi  \\n\\t\"\n");
        pFile->PrintIndent("\"movl  4(%%%%edi),%%%%edi  \\n\\t\"\n");
        pFile->PrintIndent("ToId32_EdiEsi\n"); /* EDI,ESI = ESI */
        pFile->PrintIndent("\"movl   %%%%eax,%%%%edi            \\n\\t\"\n"); // dw2
        if (bSendFlexpage)
            pFile->PrintIndent("\"movl  $2,%%%%eax          \\n\\t\"\n");
        else
            pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
        pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp    \\n\\t\"\n");
        pFile->PrintIndent("IPC_SYSENTER\n");
        pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str()); // EAX, 0
        pFile->PrintIndent("\"=d\" (");
        if (pExcDecl && !pFunction->FindAttribute(ATTR_NOEXCEPTIONS))
            pFile->Print("%s", pExcDecl->GetName().c_str());
        else
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nRecvDir, true, pContext))
                pFile->Print("%s", sDummy.c_str());
        pFile->Print("),\n"); // EDX -> dw0
        pFile->PrintIndent("\"=b\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nRecvDir, true, pContext))
            pFile->Print("%s", sDummy.c_str());
        pFile->Print("),\n"); // EBX -> dw1
        pFile->PrintIndent("\"=D\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nRecvDir, true, pContext))
            pFile->Print("%s", sDummy.c_str());
        pFile->Print("),\n"); // EDI -> dw2
        pFile->PrintIndent("\"=c\" (%s)\n", sDummy.c_str()); // ECX, 4
        pFile->PrintIndent(":\n");
        // reset index
        nIndex = 1;
        if (pFunction->FindAttribute(ATTR_NOOPCODE))
        {
            pFile->PrintIndent("\"1\" (");
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nSendDir, false, pContext))
                pFile->Print("0"); // EDX -> dw0
            pFile->Print("),\n");
        }
        else
            pFile->PrintIndent("\"1\" (%s),\n", sOpcodeConstName.c_str()); // EDX -> dw0
        pFile->PrintIndent("\"2\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nSendDir, false, pContext))
            pFile->Print("0"); // EBX -> dw1
        pFile->Print("),\n");
        pFile->PrintIndent("\"0\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nSendDir, false, pContext))
            pFile->Print("0");
        pFile->Print("),\n"); // EAX (EDI) -> dw2
        pFile->PrintIndent("\"3\" (%s),\n", pObjName->GetName().c_str());
        pFile->PrintIndent("\"4\" (%s)\n", sTimeout.c_str()); // ECX
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"esi\", \"memory\"\n");
        pFile->DecIndent();
        pFile->PrintIndent(");\n");
    }
    if (!bSymbols)
    {
        pFile->Print("#endif // !PROFILE\n");
        pFile->Print("#endif // PIC\n");
    }
}

/** \brief writes the assembler version of a long IPC call
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the writing
 */
void CL4X0aIPC::WriteAsmLongCall(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
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
    bool bSendFlexpage = pMsgBuffer->GetCount(TYPE_FLEXPAGE, pFunction->GetSendDirection()) > 0;

    bool bSendShortIPC = pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC,
                                    pFunction->GetSendDirection(), pContext);
    bool bRecvShortIPC = pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC,
                                    pFunction->GetReceiveDirection(), pContext);
    // l4_fpage_t + 2*l4_msgdope_t
    int nMsgBase = pContext->GetSizes()->GetSizeOfEnvType("l4_fpage_t") +
                pContext->GetSizes()->GetSizeOfEnvType("l4_msgdope_t")*2;

    // to increase the confusing code:
    // if we have PROGRAM_USE_SYMBOLS set, we test for __PIC__ and PROFILE
    // then we write the three parts bPIC, bPROF, bNPROF if they are set
    bool bPIC = true;
    bool bPROF = true;
    bool bNPROF = true;
    bool bSymbols = pContext->IsOptionSet(PROGRAM_USE_SYMBOLS);
    if (bSymbols)
    {
        bPIC = pContext->HasSymbol("__PIC__");
        bPROF = pContext->HasSymbol("PROFILE") && !bPIC;
        bNPROF = !bPROF && !bPIC;
    }

    if (!bSymbols)
        pFile->Print("#if defined(__PIC__)\n");
    if (bPIC)
    {
        // PIC branch
        pFile->PrintIndent("asm volatile(\n");
        pFile->IncIndent();
        pFile->PrintIndent("\"pushl  %%%%ebx            \\n\\t\"\n");
        pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n");
        if (bRecvShortIPC)
            pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp \\n\\t\"\n");
        else
        {
            pFile->PrintIndent("\"movl   %%%%edi,%%%%ebp      \\n\\t\"\n");
            pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi     \\n\\t\"\n");
            pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi     \\n\\t\"\n");
        }
        pFile->PrintIndent("ToId32_EdiEsi\n");
        if (bSendShortIPC)
        {
            pFile->PrintIndent("\"movl 8(%%%%eax),%%%%edi     \\n\\t\"\n");
            pFile->PrintIndent("\"movl 4(%%%%eax),%%%%ebx     \\n\\t\"\n");
            pFile->PrintIndent("\"movl (%%%%eax),%%%%edx     \\n\\t\"\n");
            if (bSendFlexpage)
                pFile->PrintIndent("\"movl   $2,%%%%eax         \\n\\t\"\n");
            else
                pFile->PrintIndent("\"subl  %%%%eax,%%%%eax      \\n\\t\"\n");
        }
        else
        {
            pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi     \\n\\t\"\n", nMsgBase+8);
            pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx     \\n\\t\"\n", nMsgBase+4);
            pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx     \\n\\t\"\n", nMsgBase);
            if (bSendFlexpage)
                pFile->PrintIndent("\"orl $2,%%%%eax            \\n\\t\"\n");
        }
        pFile->PrintIndent("IPC_SYSENTER\n");
        pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx      \\n\\t\"\n");
        pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
        pFile->PrintIndent("\"popl   %%%%ebx            \\n\\t\"\n");
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
        pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
        pFile->Print("[0])))),\n"); // EDX -> dw0
        pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
        pFile->Print("[4])))),\n"); // ECX (EBX) -> dw1
        pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
        pFile->Print("[8])))),\n"); // EDI -> dw2
        pFile->PrintIndent("\"=S\" (%s)\n", sDummy.c_str());
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"0\" (");
        if (bSendShortIPC)
        {
            pFile->Print("&(");
            pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
            pFile->Print("[0])");
        }
        else
        {
            if (pMsgBuffer->HasReference())
                pFile->Print("%s", sMsgBuffer.c_str());
            else
                pFile->Print("&%s", sMsgBuffer.c_str());
        }
        pFile->Print("),\n");
        pFile->PrintIndent("\"2\" (%s),\n", sTimeout.c_str());
        if (bRecvShortIPC)
        {
            pFile->PrintIndent("\"3\" (%s->lh.high),\n", pObjName->GetName().c_str());
            pFile->PrintIndent("\"4\" (%s->lh.low)\n", pObjName->GetName().c_str());
        }
        else
        {
            pFile->PrintIndent("\"3\" (((int)");
            if (pMsgBuffer->HasReference())
                pFile->Print("%s", sMsgBuffer.c_str());
            else
                pFile->Print("&%s", sMsgBuffer.c_str());
            pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n");
            pFile->PrintIndent("\"4\" (%s)\n", pObjName->GetName().c_str());
        }
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"memory\"\n");
        pFile->DecIndent();
        pFile->PrintIndent(");\n");
    }
    if (!bSymbols)
    {
        pFile->Print("#else // !PIC\n");
        pFile->Print("#if !defined(PROFILE)\n");
    }
    if (bNPROF)
    {
        // !PIC branch

        pFile->PrintIndent("asm volatile(\n");
        pFile->IncIndent();
        pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n"); // save ebp no memory references ("m") after this point
        pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx    \\n\\t\"\n");
        if (bRecvShortIPC)
            pFile->PrintIndent("\"subl  %%%%ebp,%%%%ebp \\n\\t\"\n"); // EDI, ESI are already in position
        else
        {
            pFile->PrintIndent("\"movl   %%%%edi,%%%%ebp    \\n\\t\"\n");
            pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi   \\n\\t\"\n");
            pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi   \\n\\t\"\n");
        }
        pFile->PrintIndent("ToId32_EdiEsi\n");
        if (bSendShortIPC)
        {
            pFile->PrintIndent("\"movl 8(%%%%eax),%%%%edi     \\n\\t\"\n");
            pFile->PrintIndent("\"movl 4(%%%%eax),%%%%ebx     \\n\\t\"\n");
            pFile->PrintIndent("\"movl (%%%%eax),%%%%edx     \\n\\t\"\n");
            if (bSendFlexpage)
                pFile->PrintIndent("\"movl  $2,%%%%eax          \\n\\t\"\n");
            else
                pFile->PrintIndent("\"subl  %%%%eax,%%%%eax     \\n\\t\"\n");
        }
        else
        {
            pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi     \\n\\t\"\n", nMsgBase+8);
            pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx     \\n\\t\"\n", nMsgBase+4);
            pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx     \\n\\t\"\n", nMsgBase);
            if (bSendFlexpage)
                pFile->PrintIndent("\"orl  $2,%%%%eax           \\n\\t\"\n");
        }
        pFile->PrintIndent("IPC_SYSENTER\n");
        pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
        pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
        pFile->Print("[0])))),\n"); // EDX -> dw0
        pFile->PrintIndent("\"=b\" (*((%s*)(&(", sMWord.c_str());
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
        pFile->Print("[4])))),\n"); // EBX -> dw1
        pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
        pFile->Print("[8])))),\n"); // EDI -> dw2
        pFile->PrintIndent("\"=S\" (%s)\n", sDummy.c_str());
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"0\" (");
        if (bSendShortIPC)
        {
            pFile->Print("&(");
            pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
            pFile->Print("[0])");
        }
        else
        {
            if (pMsgBuffer->HasReference())
                pFile->Print("%s", sMsgBuffer.c_str());
            else
                pFile->Print("&%s", sMsgBuffer.c_str());
        }
        pFile->Print("),\n");
        pFile->PrintIndent("\"2\" (%s),\n", sTimeout.c_str());
        if (bRecvShortIPC)
        {
            pFile->PrintIndent("\"3\" (%s->lh.high),\n", pObjName->GetName().c_str());
            pFile->PrintIndent("\"4\" (%s->lh.low)\n", pObjName->GetName().c_str());
        }
        else
        {
            pFile->PrintIndent("\"3\" (((int)");
            if (pMsgBuffer->HasReference())
                pFile->Print("%s", sMsgBuffer.c_str());
            else
                pFile->Print("&%s", sMsgBuffer.c_str());
            pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n");
            pFile->PrintIndent("\"4\" (%s)\n", pObjName->GetName().c_str());
        }
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"ecx\", \"memory\"\n");
        pFile->DecIndent();
        pFile->PrintIndent(");\n");
    }
    if (!bSymbols)
    {
        pFile->Print("#endif // !PROFILE\n");
        pFile->Print("#endif // PIC\n");
    }
}

/** \brief writes the send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4X0aIPC::WriteSend(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    if (UseAssembler(pFunction, pContext))
    {
        if (IsShortIPC(pFunction, pContext, pFunction->GetSendDirection()))
            WriteAsmShortSend(pFile, pFunction, pContext);
        else
            WriteAsmLongSend(pFile, pFunction, pContext);
    }
    else
        CL4BEIPC::WriteSend(pFile, pFunction, pContext);
}

/** \brief writes the short IPC send code
 *  \param pFile the file to write to
 *  \param pFunction the funciton to write for
 *  \param pContext the context of the writing
 */
void CL4X0aIPC::WriteAsmShortSend(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    bool bSendFlexpage = pFunction->GetMessageBuffer()->GetCount(TYPE_FLEXPAGE, pFunction->GetSendDirection()) > 0;
    short pos = 1;

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%ebp      \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
    pFile->PrintIndent("\"movl   %%%%edi,%%%%ebx  \\n\\t\"\n"); /* dw1 */
    pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi \\n\\t\"\n");
    pFile->PrintIndent("ToId32_EdiEsi\n");
    pFile->PrintIndent("\"movl   %%%%eax,%%%%edi  \\n\\t\"\n"); /* dw2 */
    if (bSendFlexpage)
        pFile->PrintIndent("\"movl  $2,%%%%eax      \\n\\t\"\n");
    else
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
    if (pFunction->FindAttribute(ATTR_NOOPCODE))
    {
        pFile->PrintIndent("\"2\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, pos++, 4, pFunction->GetSendDirection(), false, pContext))
            pFile->Print("0");
        pFile->Print("),\n");
    }
    else
        pFile->PrintIndent("\"2\" (%s),\n", pFunction->GetOpcodeConstName().c_str()); /* EDX, dw0 (opcode) */
    pFile->PrintIndent("\"1\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"4\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, pos++, 4, pFunction->GetSendDirection(), false, pContext))
        pFile->Print("0");
    pFile->Print("),\n"); /* EDI => EBX, dw1 */
    pFile->PrintIndent("\"3\" (%s),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"0\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, pos++, 4, pFunction->GetSendDirection(), false, pContext))
        pFile->Print("0");
    pFile->Print(")\n"); /* EAX => EDI, dw2 */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // reset position
    pos = 1;
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%ebp  \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
    pFile->PrintIndent("ToId32_EdiEsi\n"); /* EDI,ESI => ESI */
    pFile->PrintIndent("\"movl   %%%%eax,%%%%edi  \\n\\t\"\n");
    if (bSendFlexpage)
        pFile->PrintIndent("\"movl $2,%%%%eax       \\n\\t\"\n");
    else
        pFile->PrintIndent("\"subl   %%%%eax,%%%%eax  \\n\\t\"\n"); /* EAX = 0 (short IPC) */
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=c\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=S\" (%s)\n", sDummy.c_str());
    pFile->PrintIndent(":\n");
    if (pFunction->FindAttribute(ATTR_NOOPCODE))
    {
        pFile->PrintIndent("\"2\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, pos++, 4, pFunction->GetSendDirection(), false, pContext))
            pFile->Print("0");
        pFile->Print("),\n");
    }
    else
        pFile->PrintIndent("\"2\" (%s),\n", pFunction->GetOpcodeConstName().c_str()); /* EDX dw0*/
    pFile->PrintIndent("\"1\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"b\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, pos++, 4, pFunction->GetSendDirection(), false, pContext))
        pFile->Print("0");
    pFile->Print("),\n"); /* EBX, dw1 */
    pFile->PrintIndent("\"3\" (%s->lh.low),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"D\" (%s->lh.high),\n", pObjName->GetName().c_str()); /* EDI */
    pFile->PrintIndent("\"0\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, pos++, 4, pFunction->GetSendDirection(), false, pContext))
        pFile->Print("0");
    pFile->Print(")\n"); /* EAX => EDI, dw2 */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");

}

/** \brief writes the long IPC assembler code for the Send IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4X0aIPC::WriteAsmLongSend(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    CBEMsgBufferType* pMsgBuffer = pFunction->GetMessageBuffer();
    bool bSendFlexpage = pMsgBuffer->GetCount(TYPE_FLEXPAGE, pFunction->GetSendDirection()) > 0;

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("ToId32_EdiEsi\n");
    pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
    pFile->PrintIndent("\"orl    $-1,%%%%ebp    \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
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
    if (bSendFlexpage)
        pFile->Print("(int)(");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    if (bSendFlexpage)
        pFile->Print(")|2");
    pFile->Print("),\n");
    pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"d\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"D\" (%s->lh.high)\n", pObjName->GetName().c_str()); /* EDI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("ToId32_EdiEsi\n");
    pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
    pFile->PrintIndent("\"orl    $-1,%%%%ebp    \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
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
    if (bSendFlexpage)
        pFile->Print("(int)(");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    if (bSendFlexpage)
        pFile->Print(")|2");
    pFile->Print("),\n");
    pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"d\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"D\" (%s->lh.high)\n", pObjName->GetName().c_str()); /* EDI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
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
void CL4X0aIPC::WriteReply(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    if (UseAssembler(pFunction, pContext))
    {
        if (IsShortIPC(pFunction, pContext, pFunction->GetSendDirection()))
            WriteAsmShortReply(pFile, pFunction, pContext);
        else
            WriteAsmLongReply(pFile, pFunction, pContext);
    }
    else
        CL4BEIPC::WriteReply(pFile, pFunction, pContext);
}

/** \brief writes the short IPC reply code
 *  \param pFile the file to write to
 *  \param pFunction the funciton to write for
 *  \param pContext the context of the writing
 */
void CL4X0aIPC::WriteAsmShortReply(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    bool bSendFlexpage = pFunction->GetMessageBuffer()->GetCount(TYPE_FLEXPAGE, pFunction->GetSendDirection()) > 0;
    short pos = 1;
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
    pFile->PrintIndent("\"movl   $-1,%%%%ebp      \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
    pFile->PrintIndent("\"movl   %%%%edi,%%%%ebx  \\n\\t\"\n"); /* dw1 */
    pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi \\n\\t\"\n");
    pFile->PrintIndent("ToId32_EdiEsi\n");
    pFile->PrintIndent("\"movl   %%%%eax,%%%%edi  \\n\\t\"\n"); /* dw2 */
    if (bSendFlexpage)
        pFile->PrintIndent("\"movl  $2,%%%%eax      \\n\\t\"\n");
    else
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
    if (pFunction->FindAttribute(ATTR_NOEXCEPTIONS))
    {
        pFile->PrintIndent("\"2\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, pos++, 4, pFunction->GetSendDirection(), false, pContext))
            pFile->Print("0");
        pFile->Print("),\n");
    }
    else
        pFile->PrintIndent("\"2\" (%s),\n", sException.c_str()); /* EDX, dw0 (opcode) */
    pFile->PrintIndent("\"1\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"4\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, pos++, 4, pFunction->GetSendDirection(), false, pContext))
        pFile->Print("0");
    pFile->Print("),\n"); /* EDI => EBX, dw1 */
    pFile->PrintIndent("\"3\" (%s),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"0\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, pos++, 4, pFunction->GetSendDirection(), false, pContext))
        pFile->Print("0");
    pFile->Print(")\n"); /* EAX => EDI, dw2 */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // reset position
    pos = 1;
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%ebp  \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
    pFile->PrintIndent("ToId32_EdiEsi\n"); /* EDI,ESI => ESI */
    pFile->PrintIndent("\"movl   %%%%eax,%%%%edi  \\n\\t\"\n");
    if (bSendFlexpage)
        pFile->PrintIndent("\"movl $2,%%%%eax       \\n\\t\"\n");
    else
        pFile->PrintIndent("\"subl   %%%%eax,%%%%eax  \\n\\t\"\n"); /* EAX = 0 (short IPC) */
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=c\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=S\" (%s)\n", sDummy.c_str());
    pFile->PrintIndent(":\n");
    if (pFunction->FindAttribute(ATTR_NOEXCEPTIONS))
    {
        pFile->PrintIndent("\"2\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, pos++, 4, pFunction->GetSendDirection(), false, pContext))
            pFile->Print("0");
        pFile->Print("),\n");
    }
    else
        pFile->PrintIndent("\"2\" (%s),\n", sException.c_str()); /* EDX dw0*/
    pFile->PrintIndent("\"1\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"b\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, pos++, 4, pFunction->GetSendDirection(), false, pContext))
        pFile->Print("0");
    pFile->Print("),\n"); /* EBX, dw1 */
    pFile->PrintIndent("\"3\" (%s->lh.low),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"D\" (%s->lh.high),\n", pObjName->GetName().c_str()); /* EDI */
    pFile->PrintIndent("\"0\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, pos++, 4, pFunction->GetSendDirection(), false, pContext))
        pFile->Print("0");
    pFile->Print(")\n"); /* EAX => EDI, dw2 */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");

}

/** \brief writes the long IPC assembler code for the reply IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4X0aIPC::WriteAsmLongReply(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    CBEDeclarator *pObjName = pFunction->GetObject()->GetDeclarator();
    CBEMsgBufferType* pMsgBuffer = pFunction->GetMessageBuffer();
    bool bSendFlexpage = pMsgBuffer->GetCount(TYPE_FLEXPAGE, pFunction->GetSendDirection()) > 0;

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("ToId32_EdiEsi\n");
    pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
    pFile->PrintIndent("\"orl    $-1,%%%%ebp    \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
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
    if (bSendFlexpage)
        pFile->Print("(int)(");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    if (bSendFlexpage)
        pFile->Print(")|2");
    pFile->Print("),\n");
    pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"d\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"D\" (%s->lh.high)\n", pObjName->GetName().c_str()); /* EDI */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#else // !PIC\n");
    pFile->Print("#if !defined(PROFILE)\n");
    // !PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("ToId32_EdiEsi\n");
    pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
    pFile->PrintIndent("\"orl    $-1,%%%%ebp    \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
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
    if (bSendFlexpage)
        pFile->Print("(int)(");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    if (bSendFlexpage)
        pFile->Print(")|2");
    pFile->Print("),\n");
    pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"d\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str()); /* ESI */
    pFile->PrintIndent("\"D\" (%s->lh.high)\n", pObjName->GetName().c_str()); /* EDI */
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
void CL4X0aIPC::WriteReceive(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    bool bAllowShortIPC = true;
    /* if any message can be received, we cannot use short IPC */
    if (dynamic_cast<CBEWaitAnyFunction*>(pFunction))
        bAllowShortIPC = false;
    if (UseAssembler(pFunction, pContext))
    {
        if (IsShortIPC(pFunction, pContext, pFunction->GetReceiveDirection()) &&
            bAllowShortIPC)
            WriteAsmShortReceive(pFile, pFunction, pContext);
        else
            WriteAsmLongReceive(pFile, pFunction, pContext);
    }
    else
        CL4BEIPC::WriteReceive(pFile, pFunction, pContext); // its a 2 word C binding
}

/** \brief writes short IPC assembler code for the receive case
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write oepration
 */
void CL4X0aIPC::WriteAsmShortReceive(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sDummy = pNF->GetDummyVariable(pContext);

    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    int nRecvDir = pFunction->GetReceiveDirection();

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("ToId32_EdiEsi\n");
    pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp   \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%eax    \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx  \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("\"popl   %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRecvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),\n"); // EDX -> dw0
    pFile->PrintIndent("\"=c\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRecvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),\n"); // ECX (EBX) -> dw1
    pFile->PrintIndent("\"=S\" (%s),\n", sDummy.c_str()); // Id stays the same
    pFile->PrintIndent("\"=D\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRecvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print(")\n"); // EDI -> dw2
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"2\" (%s),\n", sTimeout.c_str());
    pFile->PrintIndent("\"3\" (%s->lh.low),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"4\" (%s->lh.high)\n", pObjName->GetName().c_str());
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
    pFile->PrintIndent("ToId32_EdiEsi\n");
    pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp  \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%eax    \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRecvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),\n"); // EDX -> dw0
    pFile->PrintIndent("\"=b\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRecvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print("),\n"); // EBX -> dw1
    pFile->PrintIndent("\"=c\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=D\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 3, 4, nRecvDir, true, pContext))
        pFile->Print("%s", sDummy.c_str());
    pFile->Print(")\n"); // EDI -> dw2
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"3\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"4\" (%s->lh.high)\n", pObjName->GetName().c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}

/** \brief writes the long IPC assembler code for the receive case
 *  \param pFile the file to write to
 *  \param pFunction the function to write to
 *  \param pContext the context of the write operation
 */
void CL4X0aIPC::WriteAsmLongReceive(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();

    pFile->Print("#if defined(__PIC__)\n");
    // PIC branch
    pFile->PrintIndent("asm volatile(\n");
    pFile->IncIndent();
    pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
    pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent("ToId32_EdiEsi\n");
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
    pFile->Print("[0])))),\n"); // EDX -> dw0
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n"); // ECX (EBX) -> dw1
    pFile->PrintIndent("\"=S\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8]))))\n"); // EDI -> dw2
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n");
    pFile->PrintIndent("\"2\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"3\" (%s->lh.low),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"4\" (%s->lh.high)\n", pObjName->GetName().c_str());
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
    pFile->PrintIndent("ToId32_EdiEsi\n");
    pFile->PrintIndent("\"movl   %%%%eax,%%%%ebp  \\n\\t\"\n");
    pFile->PrintIndent("\"movl   $-1,%%%%eax    \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
    pFile->PrintIndent("\"=b\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[4])))),\n"); // EBX -> dw1
    pFile->PrintIndent("\"=c\" (%s),\n", sDummy.c_str());
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", sMWord.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[8]))))\n"); // ED) -> dw2
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"0\" (((int)");
    if (pMsgBuffer->HasReference())
        pFile->Print("%s", sMsgBuffer.c_str());
    else
        pFile->Print("&%s", sMsgBuffer.c_str());
    pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n"); /* EAX => EBP */
    pFile->PrintIndent("\"3\" (%s),\n", sTimeout.c_str()); /* ECX */
    pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str());
    pFile->PrintIndent("\"4\" (%s->lh.high)\n", pObjName->GetName().c_str());
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
    pFile->DecIndent();
    pFile->PrintIndent(");\n");

    pFile->Print("#endif // !PROFILE\n");
    pFile->Print("#endif // PIC\n");
}
