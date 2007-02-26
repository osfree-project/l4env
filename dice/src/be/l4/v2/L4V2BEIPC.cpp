/**
 *    \file    dice/src/be/l4/v2/L4V2BEIPC.cpp
 *    \brief   contains the declaration of the class CL4V2BEIPC
 *
 *    \date    08/13/2003
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

#include "L4V2BEIPC.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEContext.h"
#include "be/BEFunction.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEMarshaller.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"


CL4V2BEIPC::CL4V2BEIPC()
 : CL4BEIPC()
{

}

/** destructor for IPC class */
CL4V2BEIPC::~CL4V2BEIPC()
{
}

/** \brief write L4 V2 specific call code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4V2BEIPC::WriteCall(CBEFile * pFile,  CBEFunction * pFunction,  CBEContext * pContext)
{
    if (UseAssembler(pFunction, pContext))
    {
        if (IsShortIPC(pFunction, pContext))
            WriteAsmShortCall(pFile, pFunction, pContext);
        else
            WriteAsmLongCall(pFile, pFunction, pContext);
    }
    else
        CL4BEIPC::WriteCall(pFile, pFunction, pContext);
}

/** \brief write the assembler version of the short IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write the call for
 *  \param pContext the context of the write operation
 *
 * This is only called if UseAsmShortIPCShortIPC == true, which means that we have a short IPC
 * in both direction. This allows us some optimizations in the assembler code.
 */
void CL4V2BEIPC::WriteAsmShortCall(CBEFile *pFile, CBEFunction *pFunction, CBEContext *pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    bool bScheduling = pFunction->FindAttribute(ATTR_L4_SCHED_DECEIT); /* OR further attributes */
    string sScheduling = pContext->GetNameFactory()->GetScheduleClientVariable(pContext);

    CBEDeclarator *pObjName = pFunction->GetObject()->GetDeclarator();
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    int nRcvDir = pFunction->GetReceiveDirection();
    int nSndDir = pFunction->GetSendDirection();
    CBETypedDeclarator *pException = pFunction->GetExceptionWord();
    string sException;
    if (pException && pException->GetDeclarator())
        sException = pException->GetDeclarator()->GetName();

    // to increase the confusing code:
    // if we have PROGRAM_USE_SYMBOLS set, we test for __PIC__ and PROFILE
    // then we write the three parts bPIC, bPROF, bNPROF if they are set
    bool bPIC = true;
    bool bPROF = true;
    bool bNPROF = true;
    bool bSymbols = pContext->IsOptionSet(PROGRAM_USE_SYMBOLS);
    bool bSendFlexpage = pFunction->GetMessageBuffer()->GetCount(TYPE_FLEXPAGE, nSndDir) > 0;
    if (bSymbols)
    {
        bPIC = pContext->HasSymbol("__PIC__");
        bPROF = pContext->HasSymbol("PROFILE") && !bPIC;
        bNPROF = !bPROF && !bPIC;
    }

    if (!bSymbols)
        pFile->Print("#ifdef __PIC__\n");
    if (bPIC)
    {
        // scheduling (|2)
        // eax schedule bit
        // ebx pushed         <- edi                       DW1
        // ebp pushed         <- 0
        // ecx timeout
        // edx opcode | dw0                                DW0
        // edi dw0 | dw1      -> ebx <- 4(esi)
        // esi dest           <- 0(esi)

        // no scheduling
        // eax dw0 | dw1      -> ebx <- 0 (|2)
        // ebx pushed         <- eax
        // ebp pushed         <- 0
        // ecx timeout
        // edx opcode | dw0
        // edi dest.lh.high
        // esi dest.lh.low

        // PIC branch
        pFile->PrintIndent("asm volatile(\n");
        pFile->IncIndent();
        pFile->PrintIndent("\"pushl  %%%%ebx          \\n\\t\"\n");
        pFile->PrintIndent("\"pushl  %%%%ebp          \\n\\t\"\n"); // save ebp no memory references ("m") after this point
        if (bScheduling)
        {
            if (bSendFlexpage)
                *pFile << "\t\"orl $2,%%%%eax \\n\\t\"\n";
            *pFile << "\t\"movl %%%%edi,%%%%ebx \\n\\t\"\n";
            *pFile << "\t\"movl 4(%%%%esi),%%%%edi \\n\\t\"\n";
            *pFile << "\t\"movl (%%%%esi),%%%%esi \\n\\t\"\n";
        }
        else
        {
            pFile->PrintIndent("\"movl   %%%%eax,%%%%ebx    \\n\\t\"\n");
            if (bSendFlexpage)
                pFile->PrintIndent("\"movl $2,%%%%eax       \\n\\t\"\n");
            else
                pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
        }
        pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp    \\n\\t\"\n");
        pFile->PrintIndent("IPC_SYSENTER\n");
        pFile->PrintIndent("\"popl   %%%%ebp          \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point
        pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx    \\n\\t\"\n");
        pFile->PrintIndent("\"popl   %%%%ebx          \\n\\t\"\n");
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());                /* EAX, 0 */
        pFile->PrintIndent("\"=d\" (");
        int nIndex = 1;
        if (pFunction->FindAttribute(ATTR_NOEXCEPTIONS))
        {
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nRcvDir, true, pContext))
                pFile->Print("%s", sDummy.c_str());
        }
        else
            *pFile << sException;
        pFile->Print("),\n");                /* EDX, 1 */
        pFile->PrintIndent("\"=c\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nRcvDir, true, pContext))
            pFile->Print("%s", sDummy.c_str());
        pFile->Print(")\n");                /* ECX, 2 */
        pFile->PrintIndent(":\n");
        nIndex = 1;
        *pFile << "\t\"d\" (";
        if (pFunction->FindAttribute(ATTR_NOOPCODE))
        {
            // get send parameter
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nSndDir, false, pContext))
                *pFile << "0";
        }
        else
            *pFile << pFunction->GetOpcodeConstName();
        *pFile << "),\n";
        if (bScheduling)
        {
            *pFile << "\t\"a\" (" << sScheduling << "),\n";
            *pFile << "\t\"S\" (" << pObjName->GetName() << "),\n";
            *pFile << "\t\"D\" (";
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nSndDir, false, pContext))
                *pFile << "0";
            *pFile << "),\n";
        }
        else
        {
            pFile->PrintIndent("\"a\" (");
            // get send parameter
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nSndDir, false, pContext))
                pFile->Print("0");
            pFile->Print("),\n");                 /* EAX, 0 => EBX */
            pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str());          /* ESI    */
            pFile->PrintIndent("\"D\" (%s->lh.high),\n", pObjName->GetName().c_str());          /* EDI    */
        }
        pFile->PrintIndent("\"c\" (%s)\n", sTimeout.c_str());                 /* ECX, 2 */
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"memory\"\n");
        pFile->DecIndent();
        pFile->PrintIndent(");\n");
    } // PIC
    if (!bSymbols)
    {
        pFile->Print("#else // !__PIC__\n");
        pFile->Print("#ifdef PROFILE\n");
    }
    if (bPROF) // is !__PIC__ && PROFILE
    {
        // !PIC && PROFILE branch
        // uses ipc_i386_call_static (l4/sys/lib/src/ipc-profile.c)
        CL4BEIPC::WriteCall(pFile, pFunction, pContext);
    }
    if (!bSymbols)
        pFile->Print("#else // !PROFILE\n");
    if (bNPROF) // is !__PIC__ && !PROFILE
    {
        // else
        pFile->PrintIndent("asm volatile(\n");
        pFile->IncIndent();
        pFile->PrintIndent("\"pushl  %%%%ebp          \\n\\t\"\n"); // save ebp no memory references ("m") after this point
        if (bScheduling)
        {
            if (bSendFlexpage)
                pFile->PrintIndent("\"orl $2,%%%%eax       \\n\\t\"\n");
        }
        else
        {
            if (bSendFlexpage)
                pFile->PrintIndent("\"movl $2,%%%%eax       \\n\\t\"\n");
            else
                pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
        }
        pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp    \\n\\t\"\n");
        pFile->PrintIndent("IPC_SYSENTER\n");
        pFile->PrintIndent("\"popl   %%%%ebp          \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());                /* EAX, 0 */
        pFile->PrintIndent("\"=d\" (");
        int nIndex = 1;
        if (pFunction->FindAttribute(ATTR_NOEXCEPTIONS))
        {
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nRcvDir, true, pContext))
                pFile->Print("%s", sDummy.c_str());
        }
        else
            *pFile << sException;
        pFile->Print("),\n");                /* EDX, 1 */
        pFile->PrintIndent("\"=b\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nRcvDir, true, pContext))
            pFile->Print("%s", sDummy.c_str());
        pFile->Print("),\n");                /* EBX, 2 */
        pFile->PrintIndent("\"=c\" (%s)\n", sDummy.c_str());                /* ECX, 3 */
        pFile->PrintIndent(":\n");
        if (bScheduling)
            *pFile << "\t\"a\" (" << sScheduling << "),\n";
        nIndex = 1;
        *pFile << "\t\"d\" (";
        if (pFunction->FindAttribute(ATTR_NOOPCODE))
        {
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nSndDir, false, pContext))
                pFile->Print("0");
        }
        else
            *pFile <<  pFunction->GetOpcodeConstName();
        *pFile << "),\n";
        pFile->PrintIndent("\"b\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nSndDir, false, pContext))
            pFile->Print("0");
        pFile->Print("),\n");                 /* EBX, 2 */
        pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str());                 /* ECX, 3 */
        pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str());          /* ESI    */
        pFile->PrintIndent("\"D\" (%s->lh.high)\n", pObjName->GetName().c_str());          /* EDI    */
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"memory\"\n");
        pFile->DecIndent();
        pFile->PrintIndent(");\n");
    }
    if (!bSymbols)
    {
        pFile->Print("#endif // PROFILE\n");
        pFile->Print("#endif // __PIC__\n");
    }
}

/** \brief write the long IPC in assembler
 *  \param pFile the file to write to
 *  \param pFunction the function to write the IPC for
 *  \param pContext the context of the write operation
 */
void CL4V2BEIPC::WriteAsmLongCall(CBEFile *pFile, CBEFunction *pFunction, CBEContext *pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    bool bScheduling = pFunction->FindAttribute(ATTR_L4_SCHED_DECEIT); /* OR further attributes */
    string sScheduling = pContext->GetNameFactory()->GetScheduleClientVariable(pContext);

    // l4_fpage_t + 2*l4_msgdope_t
    int nMsgBase = pContext->GetSizes()->GetSizeOfEnvType("l4_fpage_t") +
                pContext->GetSizes()->GetSizeOfEnvType("l4_msgdope_t")*2;

    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    int nSndDir = pFunction->GetSendDirection();
    int nRcvDir = pFunction->GetReceiveDirection();

    bool bSendShortIPC = IsShortIPC(pFunction, pContext, nSndDir);
    bool bRecvShortIPC = IsShortIPC(pFunction, pContext, nRcvDir);
    bool bSendFlexpage = pFunction->GetMessageBuffer()->GetCount(TYPE_FLEXPAGE, nSndDir) > 0;

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
        pFile->Print("#ifdef __PIC__\n");
    if (bPIC)
    {
        // PIC branch
        pFile->PrintIndent("asm volatile(\n");
        pFile->IncIndent();
        pFile->PrintIndent("\"pushl  %%%%ebx         \\n\\t\"\n");
        pFile->PrintIndent("\"pushl  %%%%ebp         \\n\\t\"\n");
        if (bSendShortIPC)
        {
            pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx  \\n\\t\"\n");
            pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx  \\n\\t\"\n");
            if (bSendFlexpage)
            {
                if (bScheduling)
                    *pFile << "\t\"orl    $2,%%%%eax     \\n\\t\"\n";
                else
                    *pFile << "\t\"movl   $2,%%%%eax     \\n\\t\"\n";
            }
            else if (!bScheduling)
                pFile->PrintIndent("\"subl   %%%%eax,%%%%eax   \\n\\t\"\n"); // snd msg descr = 0
            // if (!bSendFlexpage && bScheduling) -> eax keeps the value
        }
        else
        {
            // if long ipc we can extract the dwords directly from the msg buffer structure in EAX
            if (bScheduling)
            {
                *pFile << "\t\"movl %%%%eax,%%%%edx  \\n\\t\"\n";
                *pFile << "\t\"andl $0xfffffffc,%%%%edx  \\n\\t\"\n";
                *pFile << "\t\"movl " << nMsgBase+4 << "(%%%%edx),%%%%ebx  \\n\\t\"\n";
                *pFile << "\t\"movl " << nMsgBase << "(%%%%edx),%%%%edx  \\n\\t\"\n";
            }
            else
            {
                *pFile << "\t\"movl " << nMsgBase+4 << "(%%%%eax),%%%%ebx  \\n\\t\"\n";
                *pFile << "\t\"movl " << nMsgBase << "(%%%%eax),%%%%edx  \\n\\t\"\n";
            }
            if (bSendFlexpage)
                pFile->PrintIndent("\"orl $2,%%%%eax    \\n\\t\"\n");
        }
        if (bRecvShortIPC)
            pFile->PrintIndent("\"subl   %%%%ebp, %%%%ebp  \\n\\t\"\n"); // receive short IPC
        else
            pFile->PrintIndent("\"movl   %%%%edi, %%%%ebp  \\n\\t\"\n");
        pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi  \\n\\t\"\n");
        pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi  \\n\\t\"\n");
        pFile->PrintIndent("IPC_SYSENTER\n");
        pFile->PrintIndent("\"popl  %%%%ebp          \\n\\t\"\n");
        pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx    \\n\\t\"\n");
        pFile->PrintIndent("\"popl  %%%%ebx          \\n\\t\"\n");
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
        pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
        pFunction->GetMessageBuffer()->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
        pFile->Print("[0])))),\n");
        pFile->PrintIndent("\"=c\" (*((%s*)(&(", sMWord.c_str());
        pFunction->GetMessageBuffer()->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
        pFile->Print("[4])))),\n");
        pFile->PrintIndent("\"=S\" (%s),\n", sDummy.c_str());
        pFile->PrintIndent("\"=D\" (%s)\n", sDummy.c_str());
        pFile->PrintIndent(":\n");
        // skip this if short IPC
        if (bSendShortIPC)
        {
            // eax gets scheduling bits
            if (bScheduling)
            {
                *pFile << "\"a\" (" << sScheduling << "),\n";
            }
            // -> if short IPC we have to set send dwords
            pFile->PrintIndent("\"1\" (&(");
            pFunction->GetMessageBuffer()->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
            pFile->Print("[0])),\n");
        }
        else
        {
            pFile->PrintIndent("\"a\" (");
            if (bScheduling)
                *pFile << "(long)(";
            if (pFunction->GetMessageBuffer()->HasReference())
                pFile->Print("%s", sMsgBuffer.c_str());
            else
                pFile->Print("&%s", sMsgBuffer.c_str());
            if (bScheduling)
                *pFile << ")|" << sScheduling;
            pFile->Print("),\n");
        }
        pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str());
        // only if not short IPC we have to hand this to the assembler code
        if (!bRecvShortIPC)
        {
            pFile->PrintIndent("\"D\" (((int)");
            if (pFunction->GetMessageBuffer()->HasReference())
                pFile->Print("%s", sMsgBuffer.c_str());
            else
                pFile->Print("&%s", sMsgBuffer.c_str());
            pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n");
        }
        pFile->PrintIndent("\"S\" (%s)\n", pObjName->GetName().c_str());
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"memory\"\n");
        pFile->DecIndent();
        pFile->PrintIndent(");\n");
    } // PIC
    if (!bSymbols)
    {
        pFile->Print("#else // !__PIC__\n");
        pFile->Print("#ifdef PROFILE\n");
    }
    if (bPROF)
    {
        // !PIC && PROFILE branch
        // uses ipc_i386_call_static (l4/sys/lib/src/ipc-profile.c)
        CL4BEIPC::WriteCall(pFile, pFunction, pContext);
    }
    if (!bSymbols)
        pFile->Print("#else // !PROFILE\n");
    if (bNPROF)
    {
        // else
        pFile->PrintIndent("asm volatile(\n");
        pFile->IncIndent();
        pFile->PrintIndent("\"pushl %%%%ebp          \\n\\t\"\n");   /* save ebp, no memory references ("m") after this point */
        if (bRecvShortIPC)
            pFile->PrintIndent("\"subl  %%%%ebp,%%%%ebp    \\n\\t\"\n"); /* recv msg descriptor = 0 */
        else
            pFile->PrintIndent("\"movl  %%%%ebx, %%%%ebp   \\n\\t\"\n");
        if (bSendShortIPC)
        {
            pFile->PrintIndent("\"movl 4(%%%%edx), %%%%ebx \\n\\t\"\n");   /* dest.lh.high -> edi */
            pFile->PrintIndent("\"movl  (%%%%edx), %%%%edx \\n\\t\"\n");   /* dest.lh.low  -> esi */
            if (bSendFlexpage)
            {
                if (bScheduling)
                    *pFile << "\t\"orl    $2,%%%%eax     \\n\\t\"\n";
                else
                    *pFile << "\t\"movl   $2,%%%%eax     \\n\\t\"\n";
            }
            else if (!bScheduling)
                pFile->PrintIndent("\"subl   %%%%eax,%%%%eax   \\n\\t\"\n"); // snd msg descr = 0
        }
        else
        {
            // extract dwords directly from msg buffer
            if (bScheduling)
            {
                *pFile << "\t\"movl %%%%eax,%%%%edx  \\n\\t\"\n";
                *pFile << "\t\"andl $0xfffffffc,%%%%edx  \\n\\t\"\n";
                *pFile << "\t\"movl " << nMsgBase+4 << "(%%%%edx),%%%%ebx  \\n\\t\"\n";
                *pFile << "\t\"movl " << nMsgBase << "(%%%%edx),%%%%edx  \\n\\t\"\n";
            }
            else
            {
                pFile->PrintIndent("\"movl %d(%%%%eax), %%%%ebx \\n\\t\"\n", nMsgBase+4);   /* dest.lh.high -> edi */
                pFile->PrintIndent("\"movl %d(%%%%eax), %%%%edx \\n\\t\"\n", nMsgBase);   /* dest.lh.low  -> esi */
            }
            if (bSendFlexpage)
                pFile->PrintIndent("\"orl $2, %%%%eax \\n\\t\"\n");
        }
        pFile->PrintIndent("IPC_SYSENTER\n");
        pFile->PrintIndent("\"popl  %%%%ebp          \\n\\t\"\n");   /* restore ebp, no memory references ("m") before this point */
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());               /* EAX, 0 */
        pFile->PrintIndent("\"=d\" (*((%s*)(&(", sMWord.c_str());
        pFunction->GetMessageBuffer()->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
        pFile->Print("[0])))),\n");           /* EDX, 1 */
        pFile->PrintIndent("\"=b\" (*((%s*)(&(", sMWord.c_str());
        pFunction->GetMessageBuffer()->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
        pFile->Print("[4])))),\n");           /* EBX, 2 */
        pFile->PrintIndent("\"=c\" (%s)\n", sDummy.c_str());                 /* EDI, 3 */
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str());                   /* dest, 4  */
        pFile->PrintIndent("\"D\" (%s->lh.high),\n", pObjName->GetName().c_str());                   /* dest, 4  */
        if (bSendShortIPC)
        {
            // eax gets scheduling bits
            if (bScheduling)
            {
                *pFile << "\"a\" (" << sScheduling << "),\n";
            }
            // dwords in message buffer
            pFile->PrintIndent("\"d\" (&(", sMWord.c_str());
            pFunction->GetMessageBuffer()->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_IN, pContext);
            pFile->Print("[0])),\n");             /* EDX, 1 */
        }
        else
        {
            pFile->PrintIndent("\"a\" (");
            if (bScheduling)
                *pFile << "(" << sMWord << ")(";
            if (pFunction->GetMessageBuffer()->HasReference())
                pFile->Print("%s", sMsgBuffer.c_str());
            else
                pFile->Print("&%s", sMsgBuffer.c_str());
            if (bScheduling)
                *pFile << ")|" << sScheduling;
            pFile->Print("),\n");           /* EAX, 0 */
        }
        if (!bRecvShortIPC)
        {
            pFile->PrintIndent("\"b\" (((int)");
            if (pFunction->GetMessageBuffer()->HasReference())
                pFile->Print("%s", sMsgBuffer.c_str());
            else
                pFile->Print("&%s", sMsgBuffer.c_str());
            pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n"); /* EDI, 3 rcv msg -> ebp */
        }
        pFile->PrintIndent("\"c\" (%s)\n", sTimeout.c_str());                /* timeout, 5 */
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"memory\"\n");
        pFile->DecIndent();
        pFile->PrintIndent(");\n");
    }
    if (!bSymbols)
    {
        pFile->Print("#endif // PROFILE\n");
        pFile->Print("#endif // __PIC__\n");
    }
}

/** \brief test if we could write assembler code for the IPC
 *  \param pFunction the function to write the IPC for
 *  \param pContext the context of the write operation
 *  \return true if assembler should be written
 *
 * We may write assembler if the optimization level is larger than 1 and
 * if we are not forced to write C bindings.
 */
bool CL4V2BEIPC::UseAssembler(CBEFunction* pFunction,  CBEContext* pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
        return false;
    // test position size
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    bool bReturn = pMarshaller->TestPositionSize(pFunction, 4, pFunction->GetReceiveDirection(), false, false, 2/* must fit 2 registers */, pContext);
    delete pMarshaller;
    // return
    return bReturn;
}

/** \brief writes the send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4V2BEIPC::WriteSend(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
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

/** \brief writes the assembler short send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4V2BEIPC::WriteAsmShortSend(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    bool bScheduling = pFunction->FindAttribute(ATTR_L4_SCHED_DECEIT); /* OR further attributes */
    string sScheduling = pContext->GetNameFactory()->GetScheduleClientVariable(pContext);

    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    int nSndDir = pFunction->GetSendDirection();

    // to increase the confusing code:
    // if we have PROGRAM_USE_SYMBOLS set, we test for __PIC__ and PROFILE
    // then we write the three parts bPIC, bPROF, bNPROF if they are set
    bool bPIC = true;
    bool bPROF = true;
    bool bNPROF = true;
    bool bSymbols = pContext->IsOptionSet(PROGRAM_USE_SYMBOLS);
    bool bSendFlexpage = pFunction->GetMessageBuffer()->GetCount(TYPE_FLEXPAGE, nSndDir) > 0;
    if (bSymbols)
    {
        bPIC = pContext->HasSymbol("__PIC__");
        bPROF = pContext->HasSymbol("PROFILE") && !bPIC;
        bNPROF = !bPROF && !bPIC;
    }

    if (!bSymbols)
        pFile->Print("#ifdef __PIC__\n");
    if (bPIC)
    {

        // scheduling
        // eax: scheduling bit
        // ebx: pushed
        // ebp: pushed           -> -1
        // ecx: timeout
        // edx: opcode | dw0
        // ESI: dest             -> EDI/ESI
        // EDI: dw0 | dw1        -> ebx

        // no scheduling
        // eax: dw0 | dw1        -> ebx (set 0)
        // ebx: pushed
        // ebp: pushed           -> -1
        // ecx: timeout
        // edx: opcode | dw0
        // ESI: dest.lh.low
        // EDI: dest.lh.high

        // PIC branch
        pFile->PrintIndent("asm volatile(\n");
        pFile->IncIndent();
        pFile->PrintIndent("\"pushl  %%%%ebx          \\n\\t\"\n");
        pFile->PrintIndent("\"pushl  %%%%ebp          \\n\\t\"\n"); // save ebp no memory references ("m") after this point
        if (bScheduling)
        {
            if (bSendFlexpage)
                *pFile << "\t\"orl $2,%%%%eax \\n\\t\"\n";
            *pFile << "\t\"movl %%%%edi,%%%%ebx \\n\\t\"\n";
            *pFile << "\t\"movl 4(%%%%esi),%%%%edi \\n\\t\"\n";
            *pFile << "\t\"movl (%%%%esi),%%%%esi \\n\\t\"\n";
        }
        else
        {
            pFile->PrintIndent("\"movl   %%%%eax,%%%%ebx    \\n\\t\"\n");
            if (bSendFlexpage)
                pFile->PrintIndent("\"movl $2,%%%%eax       \\n\\t\"\n");
            else
                pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
        }
        pFile->PrintIndent("\"movl   $-1,%%%%ebp    \\n\\t\"\n"); // no receive
        pFile->PrintIndent("IPC_SYSENTER\n");
        pFile->PrintIndent("\"popl   %%%%ebp          \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point
        pFile->PrintIndent("\"popl   %%%%ebx          \\n\\t\"\n");
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());                /* EAX, 0 */
        pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());
        pFile->PrintIndent("\"=c\" (%s)\n", sDummy.c_str());
        pFile->PrintIndent(":\n");
        int nIndex = 1;
        *pFile << "\t\"d\" (";
        if (pFunction->FindAttribute(ATTR_NOOPCODE))
        {
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nSndDir, false, pContext))
                *pFile << "0";
        }
        else
            *pFile << pFunction->GetOpcodeConstName();
        *pFile  << "),\n";
        if (bScheduling)
        {
            *pFile << "\t\"a\" (" << sScheduling << "),\n";
            *pFile << "\t\"S\" (" << pObjName->GetName() << "),\n";
            pFile->PrintIndent("\"D\" (");
            // get send parameter
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nSndDir, false, pContext))
                pFile->Print("0");
            pFile->Print("),\n");                 /* EDI, 0 => EBX */
        }
        else
        {
            pFile->PrintIndent("\"a\" (");
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nSndDir, false, pContext))
                pFile->Print("0");
            pFile->Print("),\n");                 /* EAX, 0 => EBX */
            pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str());          /* ESI    */
            pFile->PrintIndent("\"D\" (%s->lh.high),\n", pObjName->GetName().c_str());          /* EDI    */
        }
        pFile->PrintIndent("\"c\" (%s)\n", sTimeout.c_str());                 /* ECX, 2 */
        pFile->DecIndent();
        pFile->PrintIndent(");\n");
    } // PIC
    if (!bSymbols)
    {
        pFile->Print("#else // !__PIC__\n");
        pFile->Print("#ifdef PROFILE\n");
    }
    if (bPROF)
        CL4BEIPC::WriteSend(pFile, pFunction, pContext);
    if (!bSymbols)
        *pFile << "#else // !PROFILE\n";
    if (bNPROF) // is !__PIC__ && !PROFILE
    {

        // scheduling
        // eax: scheduling bit
        // ebx: dw0 | dw1
        // ebp: pushed           -> -1
        // ecx: timeout
        // edx: opcode | dw0
        // ESI: dest.lh.low
        // EDI: dest.lh.high

        // no scheduling
        // eax:
        // ebx: dw0 | dw1
        // ebp: pushed           -> -1
        // ecx: timeout
        // edx: opcode | dw0
        // ESI: dest.lh.low
        // EDI: dest.lh.high

        pFile->PrintIndent("asm volatile(\n");
        pFile->IncIndent();
        pFile->PrintIndent("\"pushl  %%%%ebp          \\n\\t\"\n"); // save ebp no memory references ("m") after this point
        if (bScheduling)
        {
            if (bSendFlexpage)
                *pFile << "\t\"orl $2,%%%%eax  \\n\\t\"\n";
        }
        else
        {
            if (bSendFlexpage)
                pFile->PrintIndent("\"movl $2,%%%%eax       \\n\\t\"\n");
            else
                pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
        }
        pFile->PrintIndent("\"movl   $-1,%%%%ebp    \\n\\t\"\n"); // no receive
        pFile->PrintIndent("IPC_SYSENTER\n");
        pFile->PrintIndent("\"popl   %%%%ebp          \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());                /* EAX, 0 */
        pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());
        pFile->PrintIndent("\"=b\" (%s),\n", sDummy.c_str());
        pFile->PrintIndent("\"=c\" (%s)\n", sDummy.c_str());                /* ECX, 3 */
        pFile->PrintIndent(":\n");
        if (bScheduling)
            *pFile << "\t\"a\" (" << sScheduling << "),\n";
        int nIndex = 1;
        *pFile << "\t\"d\" (";
        if (pFunction->FindAttribute(ATTR_NOOPCODE))
        {
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nSndDir, false, pContext))
                pFile->Print("0");
        }
        else
            *pFile << pFunction->GetOpcodeConstName();
        *pFile << "),\n";
        pFile->PrintIndent("\"b\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nSndDir, false, pContext))
            pFile->Print("0");
        pFile->Print("),\n");                 /* EBX, 2 */
        pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str());                 /* ECX, 3 */
        pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str());          /* ESI    */
        pFile->PrintIndent("\"D\" (%s->lh.high)\n", pObjName->GetName().c_str());          /* EDI    */
        pFile->DecIndent();
        pFile->PrintIndent(");\n");
    }
    if (!bSymbols)
    {
        pFile->Print("#endif // PROFILE\n");
        pFile->Print("#endif // __PIC__\n");
    }
}

/** \brief writes the assembler long send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4V2BEIPC::WriteAsmLongSend(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    bool bScheduling = pFunction->FindAttribute(ATTR_L4_SCHED_DECEIT); /* OR further attributes */
    string sScheduling = pContext->GetNameFactory()->GetScheduleClientVariable(pContext);

    // l4_fpage_t + 2*l4_msgdope_t
    int nMsgBase = pContext->GetSizes()->GetSizeOfEnvType("l4_fpage_t") +
                pContext->GetSizes()->GetSizeOfEnvType("l4_msgdope_t")*2;

    CBEDeclarator *pObjName = pFunction->GetObject()->GetDeclarator();
    int nSndDir = pFunction->GetSendDirection();

    bool bSendFlexpage = pFunction->GetMessageBuffer()->GetCount(TYPE_FLEXPAGE, nSndDir) > 0;
    bool bVarBuf = pFunction->GetMessageBuffer()->HasReference();
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);

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
        pFile->Print("#ifdef __PIC__\n");
    if (bPIC)
    {

        // scheduling
        // eax: scheduling bit | msgbuf
        // ebx: pushed
        // ebp: pushed           -> -1
        // ecx: timeout
        // edx: msgbuf
        // ESI: dest.lh.low
        // EDI: dest.lh.high

        // no scheduling
        // eax: msgbuf
        // ebx: pushed
        // ebp: pushed           -> -1
        // ecx: timeout
        // edx:
        // ESI: dest.lh.low
        // EDI: dest.lh.high

        // PIC branch
        pFile->PrintIndent("asm volatile(\n");
        pFile->IncIndent();
        pFile->PrintIndent("\"pushl  %%%%ebx         \\n\\t\"\n");
        pFile->PrintIndent("\"pushl  %%%%ebp         \\n\\t\"\n");
        // if long ipc we can extract the dwords directly from the msg buffer structure in EAX
        if (bScheduling)
        {
            *pFile << "\t\"movl " << nMsgBase+4 << "(%%%%edx),%%%%ebx \\n\\t\"\n";
            *pFile << "\t\"movl " << nMsgBase << "(%%%%edx),%%%%edx \\n\\t\"\n";
        }
        else
        {
            pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+4);
            pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase);
        }
        if (bSendFlexpage)
            *pFile << "\t\"orl $2,%%%%eax    \\n\\t\"\n";
        pFile->PrintIndent("\"movl   $-1, %%%%ebp  \\n\\t\"\n"); // receive short IPC
        pFile->PrintIndent("IPC_SYSENTER\n");
        pFile->PrintIndent("\"popl  %%%%ebp          \\n\\t\"\n");
        pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx    \\n\\t\"\n");
        pFile->PrintIndent("\"popl  %%%%ebx          \\n\\t\"\n");
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
        pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());
        pFile->PrintIndent("\"=c\" (%s),\n", sDummy.c_str());
        pFile->PrintIndent("\"=S\" (%s),\n", sDummy.c_str());
        pFile->PrintIndent("\"=D\" (%s)\n", sDummy.c_str());
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"a\" (");
        if (bScheduling)
            *pFile << "(unsigned long)(";
        if (!bVarBuf)
            *pFile << "&";
        *pFile << sMsgBuffer;
        if (bScheduling)
            *pFile << ")|" << sScheduling;
        *pFile << "),\n";
        if (bScheduling)
            *pFile << "\t\"d\" (" << ((bVarBuf) ? "" : "&") << sMsgBuffer << "),\n";
        pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str());
        pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str());
        pFile->PrintIndent("\"D\" (%s->lh.high)\n", pObjName->GetName().c_str());
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"memory\"\n");
        pFile->DecIndent();
        pFile->PrintIndent(");\n");
    } // PIC
    if (!bSymbols)
    {
        pFile->Print("#else // !__PIC__\n");
        pFile->Print("#ifdef PROFILE\n");
    }
    if (bPROF)
        CL4BEIPC::WriteSend(pFile, pFunction, pContext);
    if (!bSymbols)
        *pFile << "#else // !PROFILE\n";
    if (bNPROF)
    {

        // scheduling
        // eax: scheduling bit | msgbuf
        // ebx: dw0 | dw1
        // ebp: pushed           -> -1
        // ecx: timeout
        // edx: opcode | dw0
        // ESI: dest.lh.low
        // EDI: dest.lh.high

        // no scheduling
        // eax: msgbuf
        // ebx: dw0 | dw1
        // ebp: pushed           -> -1
        // ecx: timeout
        // edx: opcode | dw0
        // ESI: dest.lh.low
        // EDI: dest.lh.high

        pFile->PrintIndent("asm volatile(\n");
        pFile->IncIndent();
        pFile->PrintIndent("\"pushl %%%%ebp          \\n\\t\"\n");   /* save ebp, no memory references ("m") after this point */
        pFile->PrintIndent("\"movl  $-1,%%%%ebp    \\n\\t\"\n"); /* recv msg descriptor = 0 */
        if (bSendFlexpage)
            pFile->PrintIndent("\"orl $2, %%%%eax \\n\\t\"\n");
        pFile->PrintIndent("IPC_SYSENTER\n");
        pFile->PrintIndent("\"popl  %%%%ebp          \\n\\t\"\n");   /* restore ebp, no memory references ("m") before this point */
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());               /* EAX, 0 */
        pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());           /* EDX, 1 */
        pFile->PrintIndent("\"=b\" (%s),\n", sDummy.c_str());           /* EBX, 2 */
        pFile->PrintIndent("\"=c\" (%s)\n", sDummy.c_str());                 /* EDI, 3 */
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str());                   /* dest, 4  */
        pFile->PrintIndent("\"D\" (%s->lh.high),\n", pObjName->GetName().c_str());                   /* dest, 4  */
        pFile->PrintIndent("\"a\" (");
        if (bScheduling)
            *pFile << "(" << sMWord << ")(";
        *pFile << ((bVarBuf) ? "" : "&") << sMsgBuffer;
        if (bScheduling)
            *pFile << ")|" << sScheduling;
        pFile->Print("),\n");           /* EAX, 0 */
        int nIndex = 1;
        *pFile << "\t\"d\" (";
        if (pFunction->FindAttribute(ATTR_NOOPCODE))
        {
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nSndDir, false, pContext))
                pFile->Print("0");
        }
        else
            *pFile << pFunction->GetOpcodeConstName();
        *pFile << "),\n";
        *pFile << "\t\"b\" (";
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nSndDir, false, pContext))
            pFile->Print("0");
        *pFile << "),\n";
        pFile->PrintIndent("\"c\" (%s)\n", sTimeout.c_str());                /* timeout, 5 */
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"memory\"\n");
        pFile->DecIndent();
        pFile->PrintIndent(");\n");
    }
    if (!bSymbols)
    {
        pFile->Print("#endif // PROFILE\n");
        pFile->Print("#endif // __PIC__\n");
    }
}


/** \brief writes the reply IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4V2BEIPC::WriteReply(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
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

/** \brief writes the assembler short reply IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4V2BEIPC::WriteAsmShortReply(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    bool bScheduling = pFunction->FindAttribute(ATTR_L4_SCHED_DECEIT); /* OR further attributes */
    string sScheduling = pContext->GetNameFactory()->GetScheduleClientVariable(pContext);
    CBETypedDeclarator *pException = pFunction->GetExceptionWord();
    string sException;
    if (pException && pException->GetDeclarator())
        sException = pException->GetDeclarator()->GetName();

    CBEDeclarator *pObjName = pFunction->GetObject()->GetDeclarator();
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    int nSndDir = pFunction->GetSendDirection();

    // to increase the confusing code:
    // if we have PROGRAM_USE_SYMBOLS set, we test for __PIC__ and PROFILE
    // then we write the three parts bPIC, bPROF, bNPROF if they are set
    bool bPIC = true;
    bool bPROF = true;
    bool bNPROF = true;
    bool bSymbols = pContext->IsOptionSet(PROGRAM_USE_SYMBOLS);
    bool bSendFlexpage = pFunction->GetMessageBuffer()->GetCount(TYPE_FLEXPAGE, nSndDir) > 0;
    if (bSymbols)
    {
        bPIC = pContext->HasSymbol("__PIC__");
        bPROF = pContext->HasSymbol("PROFILE") && !bPIC;
        bNPROF = !bPROF && !bPIC;
    }

    if (!bSymbols)
        pFile->Print("#ifdef __PIC__\n");
    if (bPIC)
    {
        //      IN:                                     -> OUT
        // eax: scheduling | 0  ( |2 : flexpage)           result
        // ebx: dw1
        // ebp: -1 (no recv)
        // ecx: timeout                                    - (dummy)
        // edx: dw0 (exception | result)                   - (dummy)
        // ESI: dest.lh.low
        // EDI: dest.lh.high

        // scheduling
        // eax: scheduling bit
        // ebx: pushed
        // ebp: pushed           -> -1
        // ecx: timeout
        // edx: exception | dw0
        // ESI: dest             -> EDI/ESI
        // EDI: dw0 | dw1        -> ebx

        // no scheduling
        // eax: dw0 | dw1        -> ebx (set 0)
        // ebx: pushed
        // ebp: pushed           -> -1
        // ecx: timeout
        // edx: exception | dw0
        // ESI: dest.lh.low
        // EDI: dest.lh.high

        // PIC branch
        pFile->PrintIndent("asm volatile(\n");
        pFile->IncIndent();
        pFile->PrintIndent("\"pushl  %%%%ebx          \\n\\t\"\n");
        pFile->PrintIndent("\"pushl  %%%%ebp          \\n\\t\"\n"); // save ebp no memory references ("m") after this point
        if (bScheduling)
        {
            if (bSendFlexpage)
                *pFile << "\t\"orl $2,%%%%eax \\n\\t\"\n";
            *pFile << "\t\"movl %%%%edi,%%%%ebx \\n\\t\"\n";
            *pFile << "\t\"movl 4(%%%%esi),%%%%edi \\n\\t\"\n";
            *pFile << "\t\"movl (%%%%esi),%%%%esi \\n\\t\"\n";
        }
        else
        {
            pFile->PrintIndent("\"movl   %%%%eax,%%%%ebx    \\n\\t\"\n");
            if (bSendFlexpage)
                pFile->PrintIndent("\"movl $2,%%%%eax       \\n\\t\"\n");
            else
                pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
        }
        pFile->PrintIndent("\"movl   $-1,%%%%ebp    \\n\\t\"\n"); // no receive
        pFile->PrintIndent("IPC_SYSENTER\n");
        pFile->PrintIndent("\"popl   %%%%ebp          \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point
        pFile->PrintIndent("\"popl   %%%%ebx          \\n\\t\"\n");
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());                /* EAX, 0 */
        pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());
        pFile->PrintIndent("\"=c\" (%s)\n", sDummy.c_str());
        pFile->PrintIndent(":\n");
        int nIndex = 1;
        *pFile << "\t\"d\" (";
        if (pFunction->FindAttribute(ATTR_NOEXCEPTIONS))
        {
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nSndDir, false, pContext))
                *pFile << "0";
        }
        else
            *pFile << sException;
        *pFile << "),\n";
        if (bScheduling)
        {
            *pFile << "\t\"a\" (" << sScheduling << "),\n";
            *pFile << "\t\"S\" (" << pObjName->GetName() << "),\n";
            *pFile << "\t\"D\" (";
            // get send parameter
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nSndDir, false, pContext))
                *pFile << "0";
            *pFile << "),\n";
        }
        else
        {
            pFile->PrintIndent("\"a\" (");
            // get send parameter
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nSndDir, false, pContext))
                pFile->Print("0");
            pFile->Print("),\n");                 /* EAX, 0 => EBX */
            pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str());          /* ESI    */
            pFile->PrintIndent("\"D\" (%s->lh.high),\n", pObjName->GetName().c_str());          /* EDI    */
        }
        pFile->PrintIndent("\"c\" (%s)\n", sTimeout.c_str());                 /* ECX, 2 */
        pFile->DecIndent();
        pFile->PrintIndent(");\n");
    } // PIC
    if (!bSymbols)
    {
        *pFile << "#else // !__PIC__\n";
        *pFile << "#ifdef PROFILE\n";
    }
    if (bPROF)
        CL4BEIPC::WriteSend(pFile, pFunction, pContext);
    if (!bSymbols)
        *pFile << "#else // !PROFILE\n";
    if (bNPROF) // is !__PIC__ && !PROFILE
    {

        // scheduling
        // eax: scheduling bit
        // ebx: dw1
        // ebp: pushed           -> -1
        // ecx: timeout
        // edx: exception
        // ESI: dest.lh.low
        // EDI: dest.lh.high

        // no scheduling
        // eax:
        // ebx: dw1
        // ebp: pushed           -> -1
        // ecx: timeout
        // edx: exception
        // ESI: dest.lh.low
        // EDI: dest.lh.high

        pFile->PrintIndent("asm volatile(\n");
        pFile->IncIndent();
        pFile->PrintIndent("\"pushl  %%%%ebp          \\n\\t\"\n"); // save ebp no memory references ("m") after this point
        if (bScheduling)
        {
            if (bSendFlexpage)
                *pFile << "\t\"orl $2,%%%%eax  \\n\\t\"\n";
        }
        else
        {
            if (bSendFlexpage)
                pFile->PrintIndent("\"movl $2,%%%%eax       \\n\\t\"\n");
            else
                pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
        }
        pFile->PrintIndent("\"movl   $-1,%%%%ebp    \\n\\t\"\n"); // no receive
        pFile->PrintIndent("IPC_SYSENTER\n");
        pFile->PrintIndent("\"popl   %%%%ebp          \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());                /* EAX, 0 */
        pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());
        pFile->PrintIndent("\"=b\" (%s),\n", sDummy.c_str());
        pFile->PrintIndent("\"=c\" (%s)\n", sDummy.c_str());                /* ECX, 3 */
        pFile->PrintIndent(":\n");
        if (bScheduling)
            *pFile << "\t\"a\" (" << sScheduling << "),\n";
        int nIndex = 1;
        *pFile << "\t\"d\" (";
        if (pFunction->FindAttribute(ATTR_NOEXCEPTIONS))
        {
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nSndDir, false, pContext))
                *pFile << "0";
        }
        else
            *pFile << sException;
        *pFile << "),\n";
        pFile->PrintIndent("\"b\" (");
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nSndDir, false, pContext))
            pFile->Print("0");
        pFile->Print("),\n");                 /* EBX, 2 */
        pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str());                 /* ECX, 3 */
        pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str());          /* ESI    */
        pFile->PrintIndent("\"D\" (%s->lh.high)\n", pObjName->GetName().c_str());          /* EDI    */
        pFile->DecIndent();
        pFile->PrintIndent(");\n");
    }
    if (!bSymbols)
    {
        pFile->Print("#endif // PROFILE\n");
        pFile->Print("#endif // __PIC__\n");
    }
}

/** \brief writes the assembler long send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4V2BEIPC::WriteAsmLongReply(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
    string sResult = pNF->GetResultName(pContext);
    string sTimeout = pNF->GetTimeoutClientVariable(pContext);
    string sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
    string sDummy = pNF->GetDummyVariable(pContext);
    bool bScheduling = pFunction->FindAttribute(ATTR_L4_SCHED_DECEIT); /* OR further attributes */
    string sScheduling = pContext->GetNameFactory()->GetScheduleClientVariable(pContext);
    CBETypedDeclarator *pException = pFunction->GetExceptionWord();
    string sException;
    if (pException && pException->GetDeclarator())
        sException = pException->GetDeclarator()->GetName();

    // l4_fpage_t + 2*l4_msgdope_t
    int nMsgBase = pContext->GetSizes()->GetSizeOfEnvType("l4_fpage_t") +
                pContext->GetSizes()->GetSizeOfEnvType("l4_msgdope_t")*2;

    vector<CBEDeclarator*>::iterator iterO = pFunction->GetObject()->GetFirstDeclarator();
    CBEDeclarator *pObjName = *iterO;
    int nSndDir = pFunction->GetSendDirection();

    bool bSendFlexpage = pFunction->GetMessageBuffer()->GetCount(TYPE_FLEXPAGE, nSndDir) > 0;
    bool bVarBuf = pFunction->GetMessageBuffer()->HasReference();
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);

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
        pFile->Print("#ifdef __PIC__\n");
    if (bPIC)
    {

        // scheduling
        // eax: scheduling bit | msgbuf
        // ebx: pushed
        // ebp: pushed           -> -1
        // ecx: timeout
        // edx: msgbuf
        // ESI: dest.lh.low
        // EDI: dest.lh.high

        // no scheduling
        // eax: msgbuf
        // ebx: pushed
        // ebp: pushed           -> -1
        // ecx: timeout
        // edx:
        // ESI: dest.lh.low
        // EDI: dest.lh.high

        // PIC branch
        pFile->PrintIndent("asm volatile(\n");
        pFile->IncIndent();
        pFile->PrintIndent("\"pushl  %%%%ebx         \\n\\t\"\n");
        pFile->PrintIndent("\"pushl  %%%%ebp         \\n\\t\"\n");
        // if long ipc we can extract the dwords directly from the msg buffer structure in EAX
        if (bScheduling)
        {
            *pFile << "\t\"movl " << nMsgBase+4 << "(%%%%edx),%%%%ebx \\n\\t\"\n";
            *pFile << "\t\"movl " << nMsgBase << "(%%%%edx),%%%%edx \\n\\t\"\n";
        }
        else
        {
            pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+4);
            pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase);
        }
        if (bSendFlexpage)
            *pFile << "\t\"orl $2,%%%%eax    \\n\\t\"\n";
        pFile->PrintIndent("\"movl   $-1, %%%%ebp  \\n\\t\"\n"); // receive short IPC
        pFile->PrintIndent("IPC_SYSENTER\n");
        pFile->PrintIndent("\"popl  %%%%ebp          \\n\\t\"\n");
        pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx    \\n\\t\"\n");
        pFile->PrintIndent("\"popl  %%%%ebx          \\n\\t\"\n");
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());
        pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());
        pFile->PrintIndent("\"=c\" (%s),\n", sDummy.c_str());
        pFile->PrintIndent("\"=S\" (%s),\n", sDummy.c_str());
        pFile->PrintIndent("\"=D\" (%s)\n", sDummy.c_str());
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"a\" (");
        if (bScheduling)
            *pFile << "(" << sMWord << ")(";
        *pFile << ((bVarBuf) ? "" : "&") << sMsgBuffer;
        if (bScheduling)
            *pFile << ")|" << sScheduling;
        pFile->Print("),\n");
        if (bScheduling)
            *pFile << "\t\"d\" (" << ((bVarBuf) ? "" : "&") << sMsgBuffer << "),\n";
        pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str());
        pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str());
        pFile->PrintIndent("\"D\" (%s->lh.high)\n", pObjName->GetName().c_str());
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"memory\"\n");
        pFile->DecIndent();
        pFile->PrintIndent(");\n");
    } // PIC
    if (!bSymbols)
    {
        pFile->Print("#else // !__PIC__\n");
        pFile->Print("#ifndef PROFILE\n");
    }
    if (bNPROF)
    {

        // scheduling
        // eax: msgbuf | scheduling bit
        // ebx: dw0 | dw1
        // ebp: pushed           -> -1
        // ecx: timeout
        // edx: exception | dw0
        // ESI: dest.lh.low
        // EDI: dest.lh.high

        // no scheduling
        // eax: msgbuf
        // ebx: dw0 | dw1
        // ebp: pushed           -> -1
        // ecx: timeout
        // edx: exception | dw0
        // ESI: dest.lh.low
        // EDI: dest.lh.high

        pFile->PrintIndent("asm volatile(\n");
        pFile->IncIndent();
        pFile->PrintIndent("\"pushl %%%%ebp          \\n\\t\"\n");   /* save ebp, no memory references ("m") after this point */
        if (bSendFlexpage)
            pFile->PrintIndent("\"orl $2, %%%%eax \\n\\t\"\n");
        pFile->PrintIndent("\"movl  $-1,%%%%ebp    \\n\\t\"\n"); /* recv msg descriptor = 0 */
        pFile->PrintIndent("IPC_SYSENTER\n");
        pFile->PrintIndent("\"popl  %%%%ebp          \\n\\t\"\n");   /* restore ebp, no memory references ("m") before this point */
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"=a\" (%s),\n", sResult.c_str());               /* EAX, 0 */
        pFile->PrintIndent("\"=d\" (%s),\n", sDummy.c_str());           /* EDX, 1 */
        pFile->PrintIndent("\"=b\" (%s),\n", sDummy.c_str());           /* EBX, 2 */
        pFile->PrintIndent("\"=c\" (%s)\n", sDummy.c_str());                 /* EDI, 3 */
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"S\" (%s->lh.low),\n", pObjName->GetName().c_str());                   /* dest, 4  */
        pFile->PrintIndent("\"D\" (%s->lh.high),\n", pObjName->GetName().c_str());                   /* dest, 4  */
        pFile->PrintIndent("\"a\" (");
        if (bScheduling)
            *pFile << "(" << sMWord << ")(";
        *pFile << ((bVarBuf) ? "" : "&") << sMsgBuffer;
        if (bScheduling)
            *pFile << ")|" << sScheduling;
        pFile->Print("),\n");           /* EAX, 0 */
        pFile->PrintIndent("\"c\" (%s),\n", sTimeout.c_str());                /* timeout, 5 */
        int nIndex = 1;
        *pFile << "\t\"d\" (";
        if (pFunction->FindAttribute(ATTR_NOEXCEPTIONS))
        {
            if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nSndDir, false, pContext))
                *pFile << "0";
        }
        else
            *pFile << sException;
        *pFile << "),\n";
        *pFile << "\t\"b\" (";
        if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nSndDir, false, pContext))
            *pFile << "0";
        *pFile << ")\n";
        pFile->PrintIndent(":\n");
        pFile->PrintIndent("\"memory\"\n");
        pFile->DecIndent();
        pFile->PrintIndent(");\n");
    }
    if (!bSymbols)
    {
        pFile->Print("#endif // PROFILE\n");
        pFile->Print("#endif // __PIC__\n");
    }
}
