/**
 *    \file    dice/src/be/l4/v2/ia32/V2IA32IPC.cpp
 *    \brief   contains the declaration of the class CL4V2IA32BEIPC
 *
 *    \date    04/18/2006
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2006-2007
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

#include "V2IA32IPC.h"
#include "be/l4/v2/L4V2BENameFactory.h"
#include "be/l4/L4BEMarshaller.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEFunction.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEMsgBuffer.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"
#include "Compiler.h"

#include <string>
#include <cassert>

CL4V2IA32BEIPC::CL4V2IA32BEIPC()
 : CL4V2BEIPC()
{
}

/** destructor for IPC class */
CL4V2IA32BEIPC::~CL4V2IA32BEIPC()
{
}

/** \brief test if we could write assembler code for the IPC
 *  \param pFunction the function to write the IPC for
 *  \return true if assembler should be written
 *
 * We may write assembler if we are not forced to write C bindings.  Since we
 * have the short IPC member in the union, we can always access word sized
 * members.
 */
bool 
CL4V2IA32BEIPC::UseAssembler(CBEFunction* /*pFunction*/)
{
    if (CCompiler::IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
        return false;
    return true;
}

/** \brief write L4 V2 specific call code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void 
CL4V2IA32BEIPC::WriteCall(CBEFile * pFile,
	CBEFunction * pFunction)
{
    if (UseAssembler(pFunction))
    {
        if (IsShortIPC(pFunction, DIRECTION_INOUT))
            WriteAsmShortCall(pFile, pFunction);
        else
            WriteAsmLongCall(pFile, pFunction);
    }
    else
        CL4V2BEIPC::WriteCall(pFile, pFunction);
}

/** \brief write the assembler version of the short IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write the call for
 *
 * This is only called if UseAsmShortIPCShortIPC == true, which means that we
 * have a short IPC in both direction. This allows us some optimizations in
 * the assembler code.
 */
void 
CL4V2IA32BEIPC::WriteAsmShortCall(CBEFile *pFile,
	CBEFunction *pFunction)
{
    *pFile << "#ifdef __PIC__\n";
    WriteAsmShortPicCall(pFile, pFunction);
    *pFile << "#else // !__PIC__\n";
    WriteAsmShortNonPicCall(pFile, pFunction);
    *pFile << "#endif // __PIC__\n";
}

/** \brief write the assembler version of the short IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write the call for
 *
 * This is only called if UseAsmShortIPCShortIPC == true, which means that we
 * have a short IPC in both direction. This allows us some optimizations in
 * the assembler code.
 */
void 
CL4V2IA32BEIPC::WriteAsmShortPicCall(CBEFile *pFile,
	CBEFunction *pFunction)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)CCompiler::GetNameFactory();
    string sResult = pNF->GetResultName();
    string sTimeout = pNF->GetTimeoutClientVariable(pFunction);
    bool bDefaultTimeout = pFunction->m_Attributes.Find(ATTR_DEFAULT_TIMEOUT) != 0;
    string sDummy = pNF->GetDummyVariable();
    string sMsgBuffer = pNF->GetMessageBufferVariable();
    bool bScheduling = pFunction->m_Attributes.Find(ATTR_SCHED_DONATE);
    string sScheduling = pNF->GetScheduleClientVariable();

    CL4BEMarshaller *pMarshaller = 
	dynamic_cast<CL4BEMarshaller*>(pFunction->GetMarshaller());
    assert(pMarshaller);
    CMsgStructType nRcvDir = pFunction->GetReceiveDirection();
    CMsgStructType nSndDir = pFunction->GetSendDirection();
    string sObjName = pFunction->GetObject()->m_Declarators.First()->GetName();

    bool bSendFlexpage = 
	pFunction->GetParameterCount(TYPE_FLEXPAGE, nSndDir) > 0;

    // PIC branch
    //      IN:                                     -> OUT
    // eax: scheduling | 0  ( |2 : flexpage)           result
    // ebx: dw1                                        dw1
    // ebp: 0 (SHORT_IPC)
    // ecx: timeout                                   
    // edx: dw0 (opcode)                               dw0
    // ESI: dest.lh.low
    // EDI: dest.lh.high

    // scheduling
    // eax: scheduling bit var ( |2 : flexpage)
    // ebx: pushed                             -> ecx (dw1 out)
    // ebp: pushed           -> 0
    // ecx: timeout
    // edx: dw0 (opcode)                              (dw0 out)
    // ESI: dest -> ESI/EDI
    // EDI: dw1              -> ebx

    // no scheduling
    // eax: dw1              -> ebx (set 0)
    // ebx: pushed                             -> ecx (dw1 out)
    // ebp: pushed           -> 0
    // ecx: timeout
    // edx: dw0 (opcode)                              (dw0 out)
    // ESI: dest.lh.low
    // EDI: dest.lh.high
    // 
    *pFile << "\tasm volatile(\n";
    pFile->IncIndent();
    *pFile << "\t\"pushl %%ebx \\n\\t\"\n";
    *pFile << "\t\"pushl %%ebp \\n\\t\"\n";
    
    if (bScheduling)
    {
	if (bSendFlexpage)
	    *pFile << "\t\"orl $2,%%eax \\n\\t\"\n";
	*pFile << "\t\"movl %%edi,%%ebx \\n\\t\"\n";
	*pFile << "\t\"movl 4(%%esi),%%edi \\n\\t\"\n";
	*pFile << "\t\"movl (%%esi),%%esi \\n\\t\"\n";
    }
    else
    {
	*pFile << "\t\"movl %%eax,%%ebx \\n\\t\"\n";
	if (bSendFlexpage)
	    *pFile << "\t\"movl $2,%%eax \\n\\t\"\n";
	else
	    *pFile << "\t\"xor %%eax,%%eax \\n\\t\"\n";
    }
    if (bDefaultTimeout)
	*pFile << "\t\"xor %%ecx,%%ecx \\n\\t\"\n";
    *pFile << "\t\"xor %%ebp,%%ebp \\n\\t\"\n";
    WriteAsmSyscall(pFile, true);
    *pFile << "\t\"popl %%ebp \\n\\t\"\n";
    *pFile << "\t\"movl %%ebx,%%ecx \\n\\t\"\n";
    *pFile << "\t\"popl %%ebx \\n\\t\"\n";
    *pFile << "\t:\n";
    *pFile << "\t\"=a\" (" << sResult << "),\n";
    *pFile << "\t\"=d\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nRcvDir, 0,
	    false, true))
	*pFile << sDummy;
    *pFile << "),\n";
    *pFile << "\t\"=c\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nRcvDir, 1, 
	    false, true))
	*pFile << sDummy;
    *pFile << ")\n";
    *pFile << "\t:\n";
    if (bScheduling)
    {
	*pFile << "\t\"0\" (" << sScheduling << "),\n";
	*pFile << "\t\"S\" (" << sObjName << "),\n";
	*pFile << "\t\"D\" (";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir, 1,
		false, false))
	    *pFile << "0";
	*pFile << "),\n";
    }
    else
    {
	*pFile << "\t\"0\" (";
	if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir, 1,
		false, false))
	    *pFile << "0";
	*pFile << "),\n";
	*pFile << "\t\"S\" (" << sObjName << "->lh.low),\n";
	*pFile << "\t\"D\" (" << sObjName << "->lh.high),\n";
    }
    *pFile << "\t\"1\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir, 0,
	    false, false))
	*pFile << "0";
    if (bDefaultTimeout)
	*pFile << ")\n";
    else
    {
	*pFile << "),\n";
	*pFile << "\t\"2\" (" << sTimeout << ")\n";
    }
    *pFile << "\t:\n";
    *pFile << "\t\"memory\"\n";
    pFile->DecIndent();
    *pFile << "\t);\n";
}

/** \brief write the assembler version of the short IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write the call for
 *
 * This is only called if UseAsmShortIPCShortIPC == true, which means that we
 * have a short IPC in both direction. This allows us some optimizations in
 * the assembler code.
 */
void 
CL4V2IA32BEIPC::WriteAsmShortNonPicCall(CBEFile *pFile,
    CBEFunction *pFunction)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)CCompiler::GetNameFactory();
    string sResult = pNF->GetResultName();
    string sTimeout = pNF->GetTimeoutClientVariable(pFunction);
    bool bDefaultTimeout = pFunction->m_Attributes.Find(ATTR_DEFAULT_TIMEOUT) != 0;
    string sDummy = pNF->GetDummyVariable();
    bool bScheduling = pFunction->m_Attributes.Find(ATTR_SCHED_DONATE);
    string sScheduling = pNF->GetScheduleClientVariable();

    CL4BEMarshaller *pMarshaller = 
	dynamic_cast<CL4BEMarshaller*>(pFunction->GetMarshaller());
    assert(pMarshaller);
    CMsgStructType nRcvDir = pFunction->GetReceiveDirection();
    CMsgStructType nSndDir = pFunction->GetSendDirection();
    string sObjName = pFunction->GetObject()->m_Declarators.First()->GetName();

    bool bSendFlexpage = 
	pFunction->GetParameterCount(TYPE_FLEXPAGE, nSndDir) > 0;

    // !PIC branch
    //      IN:                                     -> OUT
    // eax: scheduling | 0  ( |2 : flexpage)           result
    // ebx: dw1                                        dw1
    // ebp: 0 (SHORT_IPC)
    // ecx: timeout                                   
    // edx: dw0 (opcode)                               dw0
    // ESI: dest.lh.low
    // EDI: dest.lh.high

    // scheduling
    // eax: scheduling bit var ( |2 : flexpage)
    // ebx: dw1                                       (dw1 out)
    // ebp: pushed           -> 0
    // ecx: timeout
    // edx: dw0 (opcode)                              (dw0 out)
    // ESI: dest.lh.low
    // EDI: dest.lh.high

    // no scheduling
    // eax: 0 ( |2 : flexpage)
    // ebx: dw1                                       (dw1 out)
    // ebp: pushed           -> 0
    // ecx: timeout
    // edx: dw0 (opcode)                              (dw0 out)
    // ESI: dest.lh.low
    // EDI: dest.lh.high
    // 
    *pFile << "\tasm volatile(\n";
    pFile->IncIndent();
    *pFile << "\t\"pushl %%ebp \\n\\t\"\n"; 
    if (bScheduling)
    {
	if (bSendFlexpage)
	    *pFile << "\t\"orl $2,%%eax \\n\\t\"\n";
    }
    else
    {
	if (bSendFlexpage)
	    *pFile << "\t\"movl $2,%%eax \\n\\t\"\n";
	else
	    *pFile << "\t\"xor %%eax,%%eax \\n\\t\"\n";
    }
    if (bDefaultTimeout)
	*pFile << "\t\"xor %%ecx,%%ecx \\n\\t\"\n";
    *pFile << "\t\"xor %%ebp,%%ebp \\n\\t\"\n";
    WriteAsmSyscall(pFile, false);
    *pFile << "\t\"popl %%ebp \\n\\t\"\n";
    *pFile << "\t:\n";
    *pFile << "\t\"=a\" (" << sResult << "),\n"; /* EAX, 0 */
    *pFile << "\t\"=d\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nRcvDir, 0, 
	    false, true))
	*pFile << sDummy;
    *pFile << "),\n";  /* EDX, 1 */
    *pFile << "\t\"=b\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nRcvDir, 1,
	    false, true))
	*pFile << sDummy;
    *pFile << "),\n";  /* EBX, 2 */
    *pFile << "\t\"=c\" (" << sDummy << ")\n"; /* ECX, 3 */
    *pFile << "\t:\n";
    if (bScheduling)
	*pFile << "\t\"a\" (" << sScheduling << "),\n";
    // if opcode is set, its in the first word
    *pFile << "\t\"d\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir, 0, 
	    false, false))
	*pFile << "0";
    *pFile << "),\n";  /* EDX, 1 */
    *pFile << "\t\"b\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir, 1, 
	    false, false))
	*pFile << "0";
    *pFile << "),\n";  /* EBX, 2 */
    if (!bDefaultTimeout)
	*pFile << "\t\"c\" (" << sTimeout << "),\n"; /* ECX, 3 */
    *pFile << "\t\"S\" (" << sObjName << "->lh.low),\n"; /* ESI */
    *pFile << "\t\"D\" (" << sObjName << "->lh.high)\n"; /* EDI */
    *pFile << "\t:\n";
    *pFile << "\t\"memory\"\n";
    pFile->DecIndent();
    *pFile << "\t);\n";
}

/** \brief write the long IPC in assembler
 *  \param pFile the file to write to
 *  \param pFunction the function to write the IPC for
 */
void 
CL4V2IA32BEIPC::WriteAsmLongCall(CBEFile *pFile, 
    CBEFunction *pFunction)
{
    *pFile << "#ifdef __PIC__\n";
    WriteAsmLongPicCall(pFile, pFunction);
    *pFile << "#else // !__PIC__\n";
    WriteAsmLongNonPicCall(pFile, pFunction);
    *pFile << "#endif // __PIC__\n";
}

/** \brief write the long IPC in assembler
 *  \param pFile the file to write to
 *  \param pFunction the function to write the IPC for
 */
void 
CL4V2IA32BEIPC::WriteAsmLongPicCall(CBEFile *pFile, 
    CBEFunction *pFunction)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)CCompiler::GetNameFactory();
    string sResult = pNF->GetResultName();
    string sTimeout = pNF->GetTimeoutClientVariable(pFunction);
    string sMsgBuffer = pNF->GetMessageBufferVariable();
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true);
    string sDummy = pNF->GetDummyVariable();
    bool bScheduling = pFunction->m_Attributes.Find(ATTR_SCHED_DONATE);
    string sScheduling = pNF->GetScheduleClientVariable();
    CL4BEMarshaller *pMarshaller = 
	dynamic_cast<CL4BEMarshaller*>(pFunction->GetMarshaller());
    assert(pMarshaller);
    string sObjName = pFunction->GetObject()->m_Declarators.First()->GetName();
    CMsgStructType nRcvDir = pFunction->GetReceiveDirection();
    CMsgStructType nSndDir = pFunction->GetSendDirection();

    bool bSendShortIPC = IsShortIPC(pFunction, nSndDir);
    bool bRecvShortIPC = IsShortIPC(pFunction, nRcvDir);
    bool bSendFlexpage = 
	pFunction->GetParameterCount(TYPE_FLEXPAGE, nSndDir) > 0;


    // PIC branch
    //      IN:                                     -> OUT
    // eax: msgbuf | scheduling | 0  ( |2 : flexpage)  result
    // ebx: dw1                                        dw1
    // ebp: msgbuf
    // ecx: timeout                                   
    // edx: dw0 (opcode)                               dw0
    // ESI: dest.lh.low
    // EDI: dest.lh.high

    // short send
    // eax: msgbuf | scheduling ( |2 : flexpage) -> mask
    // ebx: pushed                             -> ecx (dw1 out)
    // ebp: pushed -> copy from eax & mask
    // ecx: timeout
    // edx: dw0 (opcode)                              (dw0 out)
    // ESI: dest -> ESI/EDI
    // EDI: dw1              -> ebx

    // short recv
    // eax: msgbuf 
    // ebx: pushed                             -> ecx (dw1 out)
    // ebp: pushed           -> set 0
    // ecx: timeout
    // edx: dw0                                       (dw0 out)
    // ESI: dest -> ESI/EDI
    // EDI: dw1              -> ebx

    // long both ways
    // eax: msgbuf
    // ebx: pushed                             -> ecx (dw1 out)
    // ebp: pushed -> copy from eax & mask
    // ecx: timeout
    // edx: dw0                                       (dw0 out)
    // ESI: dest -> ESI/EDI
    // EDI: dw1              -> ebx
    // 
    *pFile << "\tasm volatile(\n";
    pFile->IncIndent();
    *pFile << "\t\"pushl  %%ebx  \\n\\t\"\n";
    *pFile << "\t\"pushl  %%ebp  \\n\\t\"\n";
    
    if (bSendShortIPC)
    {
	*pFile << "\t\"movl %%eax,%%ebp \\n\\t\"\n";
	if (bScheduling)
	{
	    *pFile << "\t\"andl $0xfffffffc,%%ebp \\n\\t\"\n";
	    *pFile << "\t\"andl $0x1,%%eax \\n\\t\"\n";
	    if (bSendFlexpage)
		*pFile << "\t\"orl $0x2,%%eax \\n\\t\"\n";
	}
	else
	{
	    if (bSendFlexpage)
		*pFile << "\t\"movl $0x2,%%eax \\n\\t\"\n";
	    else
		*pFile << "\t\"subl %%eax,%%eax \\n\\t\"\n";
	}
    }
    else if (bRecvShortIPC)
    {
	*pFile << "\t\"subl %%ebp,%%ebp \\n\\t\"\n";
	if (bSendFlexpage)
	    *pFile << "\t\"orl $0x2,%%eax \\n\\t\"\n";
    }
    else // long both ways
    {
	*pFile << "\t\"movl %%eax,%%ebp \\n\\t\"\n";
	if (bScheduling)
	    *pFile << "\t\"andl $0xfffffffc,%%ebp \\n\\t\"\n";
	if (bSendFlexpage)
	    *pFile << "\t\"orl $0x2,%%eax \\n\\t\"\n";
    }
    *pFile << "\t\"movl %%edi,%%ebx \\n\\t\"\n";
    *pFile << "\t\"movl 4(%%esi),%%edi \\n\\t\"\n";
    *pFile << "\t\"movl (%%esi),%%esi \\n\\t\"\n";
    WriteAsmSyscall(pFile, true);
    *pFile << "\t\"popl %%ebp \\n\\t\"\n";
    *pFile << "\t\"movl %%ebx,%%ecx \\n\\t\"\n";
    *pFile << "\t\"popl %%ebx \\n\\t\"\n";
    *pFile << "\t:\n";
    *pFile << "\t\"=a\" (" << sResult << "),\n";
    *pFile << "\t\"=d\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nRcvDir, 0, 
	    false, true))
	*pFile << sDummy;
    *pFile << "),\n";
    *pFile << "\t\"=c\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nRcvDir, 1,
	    false, true))
	*pFile << sDummy;
    *pFile << "),\n";
    *pFile << "\t\"=S\" (" << sDummy << "),\n";
    *pFile << "\t\"=D\" (" << sDummy << ")\n";
    *pFile << "\t:\n";
    *pFile << "\t\"a\" (";
    if (bScheduling)
	*pFile << "(unsigned long)(";
    if (!pFunction->GetMessageBuffer()->HasReference())
	*pFile << "&";
    *pFile << sMsgBuffer;
    if (bScheduling)
	*pFile << ")|" << sScheduling;
    *pFile << "),\n";
    *pFile << "\t\"c\" (" << sTimeout << "),\n";
    *pFile << "\t\"d\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir, 0, 
	    false, false))
	*pFile << "0";
    *pFile << "),\n";
    *pFile << "\t\"S\" (" << sObjName << "),\n";
    *pFile << "\t\"D\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir, 1, 
	    false, false))
	*pFile << "0";
    *pFile << ")\n";
    *pFile << "\t:\n";
    *pFile << "\t\"memory\"\n";
    pFile->DecIndent();
    *pFile << "\t);\n";
}

/** \brief write the long IPC in assembler
 *  \param pFile the file to write to
 *  \param pFunction the function to write the IPC for
 */
void 
CL4V2IA32BEIPC::WriteAsmLongNonPicCall(CBEFile *pFile, 
    CBEFunction *pFunction)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)CCompiler::GetNameFactory();
    string sResult = pNF->GetResultName();
    string sTimeout = pNF->GetTimeoutClientVariable(pFunction);
    string sMsgBuffer = pNF->GetMessageBufferVariable();
    string sMWord = pNF->GetTypeName(TYPE_MWORD, true);
    string sDummy = pNF->GetDummyVariable();
    bool bScheduling = pFunction->m_Attributes.Find(ATTR_SCHED_DONATE); 
    string sScheduling = pNF->GetScheduleClientVariable();
    CL4BEMarshaller *pMarshaller = 
	dynamic_cast<CL4BEMarshaller*>(pFunction->GetMarshaller());
    assert(pMarshaller);
    string sObjName = pFunction->GetObject()->m_Declarators.First()->GetName();
    CMsgStructType nRcvDir = pFunction->GetReceiveDirection();
    CMsgStructType nSndDir = pFunction->GetSendDirection();

    bool bSendShortIPC = IsShortIPC(pFunction, nSndDir);
    bool bRecvShortIPC = IsShortIPC(pFunction, nRcvDir);
    bool bSendFlexpage = 
	pFunction->GetParameterCount(TYPE_FLEXPAGE, nSndDir) > 0;


    // PIC branch
    //      IN:                                     -> OUT
    // eax: msgbuf | scheduling | 0  ( |2 : flexpage)  result
    // ebx: dw1                                        dw1
    // ebp: msgbuf
    // ecx: timeout                                   
    // edx: dw0 (opcode)                               dw0
    // ESI: dest.lh.low
    // EDI: dest.lh.high

    // short send
    // eax: msgbuf | scheduling ( |2 : flexpage) -> mask
    // ebx: dw1                                       (dw1 out)
    // ebp: pushed -> copy from eax & mask
    // ecx: timeout
    // edx: dw0 (opcode)                              (dw0 out)
    // ESI: dest.lh.low
    // EDI: dest.lh.high

    // short recv
    // eax: msgbuf 
    // ebx: dw1
    // ebp: pushed           -> set 0
    // ecx: timeout
    // edx: dw0                                       (dw0 out)
    // ESI: dest.lh.low
    // EDI: dest.lh.high

    // long both ways
    // eax: msgbuf
    // ebx: dw1                                       (dw1 out)
    // ebp: pushed -> copy from eax & mask
    // ecx: timeout
    // edx: dw0                                       (dw0 out)
    // ESI: dest.lh.low
    // EDI: dest.lh.high
    // 
    *pFile << "\tasm volatile(\n";
    pFile->IncIndent();
    *pFile << "\t\"pushl %%ebp  \\n\\t\"\n";

    if (bSendShortIPC)
    {
	*pFile << "\t\"movl %%eax,%%ebp \\n\\t\"\n";
	if (bScheduling)
	{
	    *pFile << "\t\"andl $0xfffffffc,%%ebp \\n\\t\"\n";
	    *pFile << "\t\"andl $0x1,%%eax \\n\\t\"\n";
	    if (bSendFlexpage)
		*pFile << "\t\"orl $0x2,%%eax \\n\\t\"\n";
	}
	else
	{
	    if (bSendFlexpage)
		*pFile << "\t\"movl $0x2,%%eax \\n\\t\"\n";
	    else
		*pFile << "\t\"subl %%eax,%%eax \\n\\t\"\n";
	}
    }
    else if (bRecvShortIPC)
    {
	*pFile << "\t\"subl %%ebp,%%ebp \\n\\t\"\n";
	if (bSendFlexpage)
	    *pFile << "\t\"orl $0x2,%%eax \\n\\t\"\n";
    }
    else // long both ways
    {
	*pFile << "\t\"movl %%eax,%%ebp \\n\\t\"\n";
	if (bScheduling)
	    *pFile << "\t\"andl $0xfffffffc,%%ebp \\n\\t\"\n";
	if (bSendFlexpage)
	    *pFile << "\t\"orl $0x2,%%eax \\n\\t\"\n";
    }
    WriteAsmSyscall(pFile, false);
    *pFile << "\t\"popl  %%ebp  \\n\\t\"\n";
    *pFile << "\t:\n";
    *pFile << "\t\"=a\" (" << sResult << "),\n";
    *pFile << "\t\"=d\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nRcvDir, 0,
	    false, true))
	*pFile << sDummy;
    *pFile << "),\n";
    *pFile << "\t\"=b\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nRcvDir, 1,
	    false, true))
	*pFile << sDummy;
    *pFile << "),\n";
    *pFile << "\t\"=c\" (" << sDummy << ")\n";
    *pFile << "\t:\n";
    *pFile << "\t\"a\" (";
    if (bScheduling)
	*pFile << "(unsigned long)(";
    if (!pFunction->GetMessageBuffer()->HasReference())
	*pFile << "&";
    *pFile << sMsgBuffer;
    if (bScheduling)
	*pFile << ")|" << sScheduling;
    *pFile << "),\n";
    *pFile << "\t\"b\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir, 1,
	    false, false))
	*pFile << "0";
    *pFile << "),\n";
    *pFile << "\t\"c\" (" << sTimeout << "),\n";
    *pFile << "\t\"d\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir, 0,
	    false, false))
	*pFile << "0";
    *pFile << "),\n";
    *pFile << "\t\"S\" (" << sObjName << "->lh.low),\n";
    *pFile << "\t\"D\" (" << sObjName << "->lh.high)\n";
    *pFile << "\t:\n";
    *pFile << "\t\"memory\"\n";
    pFile->DecIndent();
    *pFile << "\t);\n";
}

/** \brief writes the reply IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void 
CL4V2IA32BEIPC::WriteReply(CBEFile* pFile,
	CBEFunction* pFunction)
{
    if (UseAssembler(pFunction))
	WriteAsmSend(pFile, pFunction);
    else
        CL4V2BEIPC::WriteReply(pFile, pFunction);
}

/** \brief writes the send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void 
CL4V2IA32BEIPC::WriteSend(CBEFile* pFile,
	CBEFunction* pFunction)
{
    if (UseAssembler(pFunction))
	WriteAsmSend(pFile, pFunction);
    else
        CL4V2BEIPC::WriteSend(pFile, pFunction);
}

/** \brief writes the assembler short send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void
CL4V2IA32BEIPC::WriteAsmSend(CBEFile* pFile,
    CBEFunction* pFunction)
{
    *pFile << "#ifdef __PIC__\n";
    WriteAsmPicSend(pFile, pFunction);
    *pFile << "#else // !__PIC__\n";
    WriteAsmNonPicSend(pFile, pFunction);
    *pFile << "#endif // __PIC__\n";
}

/** \brief writes the assembler short send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void
CL4V2IA32BEIPC::WriteAsmPicSend(CBEFile* pFile,
    CBEFunction* pFunction)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)CCompiler::GetNameFactory();
    string sResult = pNF->GetResultName();
    string sTimeout = pNF->GetTimeoutClientVariable(pFunction);
    string sDummy = pNF->GetDummyVariable();
    string sMsgBuffer = pNF->GetMessageBufferVariable();
    bool bScheduling = pFunction->m_Attributes.Find(ATTR_SCHED_DONATE);
    string sScheduling = pNF->GetScheduleClientVariable();

    string sObjName = pFunction->GetObject()->m_Declarators.First()->GetName();
    CL4BEMarshaller *pMarshaller = 
	dynamic_cast<CL4BEMarshaller*>(pFunction->GetMarshaller());
    assert(pMarshaller);
    CMsgStructType nSndDir = pFunction->GetSendDirection();

    bool bSendFlexpage = 
	pFunction->GetParameterCount(TYPE_FLEXPAGE, nSndDir) > 0;
    bool bSendShortIPC = IsShortIPC(pFunction, nSndDir);

    // PIC branch
    //      IN:                                     -> OUT
    // eax: msgbuf | scheduling | 0  ( |2 : flexpage)  result
    // ebx: dw1                                        dw1
    // ebp: -1 (no recv)
    // ecx: timeout                                   
    // edx: dw0                                        dw0
    // ESI: dest.lh.low
    // EDI: dest.lh.high

    // short send
    // eax: msgbuf | scheduling ( |2 : flexpage)
    // ebx: pushed                                    (dw1 out)
    // ebp: pushed -> -1
    // ecx: timeout
    // edx: dw0                                       (dw0 out)
    // ESI: dest -> ESI/EDI
    // EDI: dw1 -> ebx
    //
    *pFile << "\tasm volatile(\n";
    pFile->IncIndent();
    *pFile << "\t\"pushl  %%ebx  \\n\\t\"\n";
    *pFile << "\t\"pushl  %%ebp  \\n\\t\"\n";
    
    if (bSendFlexpage)
	*pFile << "\t\"orl $0x2,%%eax \\n\\t\"\n";
    *pFile << "\t\"movl $0xffffffff,%%ebp \\n\\t\"\n";
    *pFile << "\t\"movl %%edi,%%ebx \\n\\t\"\n";
    *pFile << "\t\"movl 4(%%esi),%%edi \\n\\t\"\n";
    *pFile << "\t\"movl (%%esi),%%esi \\n\\t\"\n";
    WriteAsmSyscall(pFile, true);
    *pFile << "\t\"popl   %%ebp  \\n\\t\"\n";
    *pFile << "\t\"popl   %%ebx  \\n\\t\"\n";
    *pFile << "\t:\n";
    *pFile << "\t\"=a\" (" << sResult << "),\n";
    *pFile << "\t\"=d\" (" << sDummy << "),\n";
    *pFile << "\t\"=c\" (" << sDummy << ")\n";
    *pFile << "\t:\n";
    *pFile << "\t\"a\" (";
    if (bSendShortIPC)
    {
	if (bScheduling)
	    *pFile << sScheduling;
	else
	    *pFile << "0";
    }
    else
    {
	if (bScheduling)
	    *pFile << "(unsigned long)(";
        if (!pFunction->GetMessageBuffer()->HasReference())
	    *pFile << "&";
	*pFile << sMsgBuffer;
	if (bScheduling)
	    *pFile << ")|" << sScheduling;
    }
    *pFile << "),\n";
    *pFile << "\t\"d\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir, 0,
	    false, false))
	*pFile << "0";
    *pFile << "),\n";
    *pFile << "\t\"D\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir, 1,
	    false, false))
	*pFile << "0";
    *pFile << "),\n";
    *pFile << "\t\"c\" (" << sTimeout << "),\n";
    *pFile << "\t\"S\" (" << sObjName << ")\n";
    pFile->DecIndent();
    *pFile << "\t);\n";
}

/** \brief writes the assembler short send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 */
void
CL4V2IA32BEIPC::WriteAsmNonPicSend(CBEFile* pFile,
    CBEFunction* pFunction)
{
    CL4BENameFactory *pNF = (CL4BENameFactory*)CCompiler::GetNameFactory();
    string sResult = pNF->GetResultName();
    string sTimeout = pNF->GetTimeoutClientVariable(pFunction);
    string sDummy = pNF->GetDummyVariable();
    string sMsgBuffer = pNF->GetMessageBufferVariable();
    bool bScheduling = pFunction->m_Attributes.Find(ATTR_SCHED_DONATE);
    string sScheduling = pNF->GetScheduleClientVariable();

    string sObjName = pFunction->GetObject()->m_Declarators.First()->GetName();
    CL4BEMarshaller *pMarshaller = 
	dynamic_cast<CL4BEMarshaller*>(pFunction->GetMarshaller());
    assert(pMarshaller);
    CMsgStructType nSndDir = pFunction->GetSendDirection();
    bool bSendFlexpage = 
	pFunction->GetParameterCount(TYPE_FLEXPAGE, nSndDir) > 0;
    bool bSendShortIPC = IsShortIPC(pFunction, nSndDir);

    // !PIC branch
    //      IN:                                     -> OUT
    // eax: msgbuf | scheduling | 0  ( |2 : flexpage)  result
    // ebx: dw1                                        dw1
    // ebp: msgbuf
    // ecx: timeout                                   
    // edx: dw0 (opcode)                               dw0
    // ESI: dest.lh.low
    // EDI: dest.lh.high

    // short send
    // eax: scheduling ( |2 : flexpage)
    // ebx: dw1                                       (dw1 out)
    // ebp: pushed -> -1
    // ecx: timeout
    // edx: dw0 (opcode)                              (dw0 out)
    // ESI: dest.lh.low
    // EDI: dest.lh.high
    //
    *pFile << "\tasm volatile(\n";
    pFile->IncIndent();
    *pFile << "\t\"pushl  %%ebp  \\n\\t\"\n";
    if (bSendFlexpage)
	*pFile << "\t\"orl $0x2,%%eax  \\n\\t\"\n";
    *pFile << "\t\"movl   $-1,%%ebp  \\n\\t\"\n";
    WriteAsmSyscall(pFile, false);
    *pFile << "\t\"popl   %%ebp  \\n\\t\"\n";
    *pFile << "\t:\n";
    *pFile << "\t\"=a\" (" << sResult << "),\n";
    *pFile << "\t\"=d\" (" << sDummy << "),\n";
    *pFile << "\t\"=b\" (" << sDummy << "),\n";
    *pFile << "\t\"=c\" (" << sDummy << ")\n";
    *pFile << "\t:\n";
    *pFile << "\t\"a\" (";
    if (bSendShortIPC)
    {
	if (bScheduling)
	    *pFile << sScheduling;
	else
	    *pFile << "0";
    }
    else
    {
	if (bScheduling)
	    *pFile << "(unsigned long)(";
        if (!pFunction->GetMessageBuffer()->HasReference())
	    *pFile << "&";
	*pFile << sMsgBuffer;
	if (bScheduling)
	    *pFile << ")|" << sScheduling;
    }
    *pFile << "),\n";
    *pFile << "\t\"b\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir, 1,
	    false, false))
	*pFile << "0";
    *pFile << "),\n";
    *pFile << "\t\"c\" (" << sTimeout << "),\n";
    // if opcode is set, its in the first word
    *pFile << "\t\"d\" (";
    if (!pMarshaller->MarshalWordMember(pFile, pFunction, nSndDir, 0,
	    false, false))
	*pFile << "0";
    *pFile << "),\n";
    *pFile << "\t\"S\" (" << sObjName << "->lh.low),\n";
    *pFile << "\t\"D\" (" << sObjName << "->lh.high)\n";
    pFile->DecIndent();
    *pFile << "\t);\n";
}

/** \brief writes the actual instruction to invoke the IPC syscall
 *  \param pFile the file to write to
 *  \param bPic true if currently writing __PIC__ code
 *
 * This function evaluates the -fsyscall=xx option of Dice.
 */
void CL4V2IA32BEIPC::WriteAsmSyscall(CBEFile *pFile,
    bool bPic)
{
    string sSyscall("IPC_SYSENTER");
    CCompiler::GetBackEndOption("syscall", sSyscall);
    if (sSyscall == "int30")
    {
	*pFile << "\t\"int $0x30 \\n\\t\"\n";
	return;
    }
    if (sSyscall == "abs-syscall")
    {
	if (bPic)
	    *pFile << "\t\"call __l4sys_abs_ipc_fixup \\n\\t\"\n";
	else
	    *pFile << "\t\"call __l4sys_ipc_direct \\n\\t\"\n";
	return;
    }
    if (sSyscall == "sysenter")
    {
	if (bPic)
	{
	    *pFile << "\t\"push   %%ecx  \\n\\t\"\n";
	    *pFile << "\t\"push   %%ebp  \\n\\t\"\n";
	    *pFile << "\t\"push   $0x1b  \\n\\t\"\n";
	    *pFile << "\t\"call   0f     \\n\\t\"\n";
	    *pFile << "\t\"0:            \\n\\t\"\n";
	    *pFile << "\t\"addl   $(1f-0b),(%%esp) \\n\\t\"\n";
	    *pFile << "\t\"mov    %%esp,%%ecx      \\n\\t\"\n";
	    *pFile << "\t\"sysenter      \\n\\t\"\n";
	    *pFile << "\t\"mov    %%ebp,%%edx      \\n\\t\"\n";
	    *pFile << "\t\"1:            \\n\\t\"\n";
	} else {
	    *pFile << "\t\"push   %%ecx  \\n\\t\"\n";
	    *pFile << "\t\"push   %%ebp  \\n\\t\"\n";
	    *pFile << "\t\"push   $0x1b  \\n\\t\"\n";
	    *pFile << "\t\"push   $0f    \\n\\t\"\n";
	    *pFile << "\t\"mov    %%esp,%%ecx      \\n\\t\"\n";
	    *pFile << "\t\"sysenter      \\n\\t\"\n";
	    *pFile << "\t\"mov    %%ebp,%%edx      \\n\\t\"\n";
	    *pFile << "\t\"0:            \\n\\t\"\n";
	}
    }
    *pFile << "\tIPC_SYSENTER\n";
}
