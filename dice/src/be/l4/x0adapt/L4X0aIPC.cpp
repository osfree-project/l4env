/**
 *	\file	dice/src/be/l4/x0adapt/L4X0aIPC.h
 *	\brief	contains the declaration of the class CL4X0aIPC
 *
 *	\date	08/14/2002
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
#include "be/l4/x0adapt/L4X0aIPC.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BEFile.h"
#include "be/BEFunction.h"
#include "be/BEContext.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEDeclarator.h"
#include "be/BEMarshaller.h"

#include "TypeSpec-Type.h"

IMPLEMENT_DYNAMIC(CL4X0aIPC);

CL4X0aIPC::CL4X0aIPC()
 : CL4BEIPC()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0aIPC, CL4BEIPC);
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
	if (pContext->GetOptimizeLevel() < 2)
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
 *  \param bAllowShortIPC true if short is allowed
 *  \param pContext the context of the IPC writing
 */
void CL4X0aIPC::WriteWait(CBEFile* pFile,  CBEFunction* pFunction,  bool bAllowShortIPC,  CBEContext* pContext)
{
	if (UseAssembler(pFunction, pContext))
	{
		if (IsShortIPC(pFunction, pContext, pFunction->GetReceiveDirection()) &&
		    bAllowShortIPC)
			WriteAsmShortWait(pFile, pFunction, pContext);
		else
		    WriteAsmLongWait(pFile, pFunction, pContext);
	}
	else
	    CL4BEIPC::WriteWait(pFile, pFunction, bAllowShortIPC, pContext);
}

/** \brief write the assembler version of a short IPC for wait
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4X0aIPC::WriteAsmShortWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);

	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
	String sDummy = pNF->GetDummyVariable(pContext);
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=d\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRecvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),\n"); // EDX -> dw0
	pFile->PrintIndent("\"=c\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 3, 4, nRecvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),\n"); // ECX (EDI) -> dw2
	pFile->PrintIndent("\"=S\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=D\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRecvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print(")\n"); // EDI (EBX) -> dw1
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"3\" (%s)\n", (const char*)pObjName->GetName());
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=b\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRecvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),\n"); // EBX -> dw1
	pFile->PrintIndent("\"=c\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 3, 4, nRecvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),\n"); // ECX (EDI) -> dw2
	pFile->PrintIndent("\"=d\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRecvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),\n"); // EDX -> dw0
	pFile->PrintIndent("\"=S\" (%s->lh.low),\n", (const char*)pObjName->GetName()); // ESI
	pFile->PrintIndent("\"=D\" (%s->lh.high)\n", (const char*)pObjName->GetName()); // EDI
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"2\" (%s)\n", (const char*)sTimeout);
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
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	String sDummy = pNF->GetDummyVariable(pContext);
	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
	pFile->PrintIndent("\"=S\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4]))))\n"); // EDI (EBX) -> dw1
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC),\n");
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"3\" (%s)\n", (const char*)pObjName->GetName());
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n"); // EBX -> dw1
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
	pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])))),\n"); // EDX -> dw0
	pFile->PrintIndent("\"=S\" (%s->lh.low),\n", (const char*)pObjName->GetName()); // ESI
	pFile->PrintIndent("\"=D\" (%s->lh.high)\n", (const char*)pObjName->GetName()); // EDI
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC),\n");
	pFile->PrintIndent("\"2\" (%s)\n", (const char*)sTimeout);
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
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	String sDummy = pNF->GetDummyVariable(pContext);

	// l4_fpage_t + 2*l4_msgdope_t
	int nMsgBase = pContext->GetSizes()->GetSizeOfEnvType("l4_fpage_t") +
				pContext->GetSizes()->GetSizeOfEnvType("l4_msgdope_t")*2;
	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
	pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])))),\n"); // EDX -> dw0
	pFile->PrintIndent("\"=S\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n"); // ESI (EBX) -> dw1
	pFile->PrintIndent("\"=D\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"a\" (");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print("),\n");
	pFile->PrintIndent("\"c\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"S\" (%s)\n", (const char*)pObjName->GetName()); /* ESI */
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n"); // EBX -> dw1
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
	pFile->PrintIndent("\"=S\" (%s->lh.low),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (%s->lh.high)\n", (const char*)pObjName->GetName());
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"a\" (");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print("),\n"); /* EAX */
	pFile->PrintIndent("\"c\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"S\" (%s->lh.low),\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent("\"D\" (%s->lh.high)\n", (const char*)pObjName->GetName()); /* EDI */
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
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	// l4_fpage_t + 2*l4_msgdope_t
	int nMsgBase = pContext->GetSizes()->GetSizeOfEnvType("l4_fpage_t") +
				pContext->GetSizes()->GetSizeOfEnvType("l4_msgdope_t")*2;
	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	String sDummy = pNF->GetDummyVariable(pContext);
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
	pFile->PrintIndent("\"movl %d(%%%%edx),%%%%ebx  \\n\\t\"\n", nMsgBase+4); // dw1 -> ebx
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
	pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])))),\n"); // EDX -> dw0
	pFile->PrintIndent("\"=S\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n"); // ESI (EBX) -> dw1
	pFile->PrintIndent("\"=D\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"a\" (");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print("),\n"); /* EAX */
	pFile->PrintIndent("\"c\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"S\" (%s)\n", (const char*)pObjName->GetName());
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
	pFile->PrintIndent("\"movl %d(%%%%edx),%%%%edx  \\n\\t\"\n", nMsgBase); // dw0 -> edx
	pFile->PrintIndent("\"orl   $2,%%%%eax          \\n\\t\"\n"); // fpage ipc
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl  %%%%edi,%%%%ecx     \\n\\t\"\n"); // save edi in ecx
	pFile->PrintIndent("FromId32_Esi\n");
	pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n"); // EBX -> dw1
	pFile->PrintIndent("\"=S\" (%s->lh.low),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (%s->lh.high)\n", (const char*)pObjName->GetName());
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"a\" (");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print("),\n"); /* EAX */
	pFile->PrintIndent("\"c\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"S\" (%s->lh.low),\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent("\"D\" (%s->lh.high)\n", (const char*)pObjName->GetName()); /* EDI */
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
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	String sDummy = pNF->GetDummyVariable(pContext);

	// l4_fpage_t + 2*l4_msgdope_t
	int nMsgBase = pContext->GetSizes()->GetSizeOfEnvType("l4_fpage_t") +
				pContext->GetSizes()->GetSizeOfEnvType("l4_msgdope_t")*2;
	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);

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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
	pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])))),\n"); // EDX -> dw0
	pFile->PrintIndent("\"=S\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n"); // ESI (EBX) -> dw1
	pFile->PrintIndent("\"=D\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"a\" (");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print("),\n");
	pFile->PrintIndent("\"c\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"S\" (%s)\n", (const char*)pObjName->GetName()); /* ESI */
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n"); // EBX -> dw1
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
	pFile->PrintIndent("\"=S\" (%s->lh.low),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (%s->lh.high)\n", (const char*)pObjName->GetName());
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"a\" (");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print("),\n"); /* EAX */
	pFile->PrintIndent("\"c\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"S\" (%s->lh.low),\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent("\"D\" (%s->lh.high)\n", (const char*)pObjName->GetName()); /* EDI */
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
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	// l4_fpage_t + 2*l4_msgdope_t
	int nMsgBase = pContext->GetSizes()->GetSizeOfEnvType("l4_fpage_t") +
				pContext->GetSizes()->GetSizeOfEnvType("l4_msgdope_t")*2;
	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	String sDummy = pNF->GetDummyVariable(pContext);
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
	pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])))),\n"); // EDX -> dw0
	pFile->PrintIndent("\"=S\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n"); // ESI (EBX) -> dw1
	pFile->PrintIndent("\"=D\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"a\" (");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print("),\n"); /* EAX */
	pFile->PrintIndent("\"c\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"S\" (%s)\n", (const char*)pObjName->GetName());
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n"); // ECX (EDI) -> dw2
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n"); // EBX -> dw1
	pFile->PrintIndent("\"=S\" (%s->lh.low),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (%s->lh.high)\n", (const char*)pObjName->GetName());
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"a\" (");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print("),\n"); /* EAX */
	pFile->PrintIndent("\"c\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"S\" (%s->lh.low),\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent("\"D\" (%s->lh.high)\n", (const char*)pObjName->GetName()); /* EDI */
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
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sDummy = pNF->GetDummyVariable(pContext);
	CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType*)pFunction->GetMessageBuffer();
	int nSendDir = pFunction->GetSendDirection();
	bool bSendFlexpage = pMsgBuffer->GetFlexpageCount(nSendDir) > 0;

	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
	int nRecvDir = pFunction->GetReceiveDirection();
	String sOpcodeConstName = pFunction->GetOpcodeConstName();

	// get exception
	CBETypedDeclarator *pException = pFunction->GetExceptionWord();
	CBEDeclarator *pExcDecl = 0;
	if (pException)
	{
		pIter = pException->GetFirstDeclarator();
        pExcDecl = pException->GetNextDeclarator(pIter);
	}
	int nIndex = 0;

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
		pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
		pFile->PrintIndent("\"=d\" (");
		// XXX FIXME: what about short fpage IPC
		if (pExcDecl)
		{
			pFile->Print("%s", (const char*)pExcDecl->GetName());
			nIndex = 1;
		}
		else
		{
			if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRecvDir, true, pContext))
				pFile->Print("%s", (const char*)sDummy);
			nIndex = 2;
		}
		pFile->Print("),\n"); // EDX -> dw0
		pFile->PrintIndent("\"=c\" (");
		if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nRecvDir, true, pContext))
			pFile->Print("%s", (const char*)sDummy);
		pFile->Print("),\n"); // ECX (EBX) -> dw1
		pFile->PrintIndent("\"=D\" (");
		if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nRecvDir, true, pContext))
			pFile->Print("%s", (const char*)sDummy);
		pFile->Print("),\n"); // EDI -> dw2
		pFile->PrintIndent("\"=S\" (%s)\n", (const char*)sDummy);
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"0\" (");
		if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nSendDir, false, pContext))
			pFile->Print("0");
		pFile->Print("),\n"); // EAX (EDI) -> dw2
		pFile->PrintIndent("\"1\" (%s),\n", (const char*)sOpcodeConstName); // EDX -> dw0
		pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
		pFile->PrintIndent("\"3\" (");
		if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSendDir, false, pContext))
			pFile->Print("0");
		pFile->Print("),\n"); // EDI (EBX) -> dw1
		pFile->PrintIndent("\"4\" (%s)\n", (const char*)pObjName->GetName());
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
		pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult); // EAX, 0
		pFile->PrintIndent("\"=d\" (");
		if (pExcDecl)
		{
			pFile->Print("%s", (const char*)pExcDecl->GetName());
			nIndex = 1;
		}
		else
		{
			if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRecvDir, true, pContext))
				pFile->Print("%s", (const char*)sDummy);
			nIndex = 2;
		}
		pFile->Print("),\n"); // EDX -> dw0
		pFile->PrintIndent("\"=b\" (");
		if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nRecvDir, true, pContext))
			pFile->Print("%s", (const char*)sDummy);
		pFile->Print("),\n"); // EBX -> dw1
		pFile->PrintIndent("\"=D\" (");
		if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex, 4, nRecvDir, true, pContext))
			pFile->Print("%s", (const char*)sDummy);
		pFile->Print("),\n"); // EDI -> dw2
		pFile->PrintIndent("\"=c\" (%s)\n", (const char*)sDummy); // ECX, 4
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"0\" (");
		if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nSendDir, false, pContext))
			pFile->Print("0");
		pFile->Print("),\n"); // EAX (EDI) -> dw2
		pFile->PrintIndent("\"1\" (%s),\n", (const char*)sOpcodeConstName); // EDX -> dw0
		pFile->PrintIndent("\"2\" (");
		if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSendDir, false, pContext))
			pFile->Print("0"); // EBX -> dw1
		pFile->Print("),\n");
		pFile->PrintIndent("\"3\" (%s),\n", (const char*)pObjName->GetName());
		pFile->PrintIndent("\"4\" (%s)\n", (const char*)sTimeout); // ECX
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
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	String sDummy = pNF->GetDummyVariable(pContext);
    CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType*)pFunction->GetMessageBuffer();
	bool bSendFlexpage = pMsgBuffer->GetFlexpageCount(pFunction->GetSendDirection()) > 0;

	bool bSendShortIPC = pMsgBuffer->IsShortIPC(pFunction->GetSendDirection(), pContext);
	bool bRecvShortIPC = pMsgBuffer->IsShortIPC(pFunction->GetReceiveDirection(), pContext);
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
		pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
		pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
		pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
		pFile->Print("[0])))),\n"); // EDX -> dw0
		pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
		pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
		pFile->Print("[4])))),\n"); // ECX (EBX) -> dw1
		pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
		pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
		pFile->Print("[8])))),\n"); // EDI -> dw2
		pFile->PrintIndent("\"=S\" (%s)\n", (const char*)sDummy);
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"0\" (");
		if (bSendShortIPC)
		{
			pFile->Print("&(");
			pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
			pFile->Print("[0])");
		}
		else
		{
			if (pMsgBuffer->HasReference())
				pFile->Print("%s", (const char *) sMsgBuffer);
			else
				pFile->Print("&%s", (const char *) sMsgBuffer);
		}
		pFile->Print("),\n");
		pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
		if (bRecvShortIPC)
		{
			pFile->PrintIndent("\"3\" (%s->lh.high),\n", (const char*)pObjName->GetName());
			pFile->PrintIndent("\"4\" (%s->lh.low)\n", (const char*)pObjName->GetName());
		}
		else
		{
			pFile->PrintIndent("\"3\" (((int)");
			if (pMsgBuffer->HasReference())
				pFile->Print("%s", (const char *) sMsgBuffer);
			else
				pFile->Print("&%s", (const char *) sMsgBuffer);
			pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n");
			pFile->PrintIndent("\"4\" (%s)\n", (const char*)pObjName->GetName());
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
		pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
		pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
		pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
		pFile->Print("[0])))),\n"); // EDX -> dw0
		pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
		pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
		pFile->Print("[4])))),\n"); // EBX -> dw1
		pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
		pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
		pFile->Print("[8])))),\n"); // EDI -> dw2
		pFile->PrintIndent("\"=S\" (%s)\n", (const char*)sDummy);
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"0\" (");
		if (bSendShortIPC)
		{
			pFile->Print("&(");
			pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
			pFile->Print("[0])");
		}
		else
		{
			if (pMsgBuffer->HasReference())
				pFile->Print("%s", (const char *) sMsgBuffer);
			else
				pFile->Print("&%s", (const char *) sMsgBuffer);
		}
		pFile->Print("),\n");
		pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
		if (bRecvShortIPC)
		{
			pFile->PrintIndent("\"3\" (%s->lh.high),\n", (const char*)pObjName->GetName());
			pFile->PrintIndent("\"4\" (%s->lh.low)\n", (const char*)pObjName->GetName());
		}
		else
		{
			pFile->PrintIndent("\"3\" (((int)");
			if (pMsgBuffer->HasReference())
				pFile->Print("%s", (const char *) sMsgBuffer);
			else
				pFile->Print("&%s", (const char *) sMsgBuffer);
			pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n");
			pFile->PrintIndent("\"4\" (%s)\n", (const char*)pObjName->GetName());
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
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sDummy = pNF->GetDummyVariable(pContext);
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	bool bSendFlexpage = ((CL4BEMsgBufferType*)pFunction->GetMessageBuffer())->GetFlexpageCount(pFunction->GetSendDirection()) > 0;

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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=c\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=d\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=S\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=D\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)pFunction->GetOpcodeConstName()); /* EDX, dw0 (opcode) */
	pFile->PrintIndent("\"1\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"4\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, pFunction->GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n"); /* EDI => EBX, dw1 */
	pFile->PrintIndent("\"3\" (%s),\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent("\"0\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, pFunction->GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print(")\n"); /* EAX => EDI, dw2 */
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=c\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=d\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=S\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)pFunction->GetOpcodeConstName()); /* EDX dw0*/
	pFile->PrintIndent("\"1\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"b\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, pFunction->GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n"); /* EBX, dw1 */
	pFile->PrintIndent("\"3\" (%s->lh.low),\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent("\"D\" (%s->lh.high),\n", (const char*)pObjName->GetName()); /* EDI */
	pFile->PrintIndent("\"0\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, pFunction->GetSendDirection(), false, pContext))
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
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	String sDummy = pNF->GetDummyVariable(pContext);
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	CL4BEMsgBufferType* pMsgBuffer = (CL4BEMsgBufferType*)pFunction->GetMessageBuffer();
	bool bSendFlexpage = pMsgBuffer->GetFlexpageCount(pFunction->GetSendDirection()) > 0;

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
	pFile->PrintIndent("\"movl   $-1,%%%%ebp    \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebx        \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=c\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=d\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=S\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=D\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
	if (bSendFlexpage)
	    pFile->Print("(int)(");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	if (bSendFlexpage)
	    pFile->Print(")|2");
	pFile->Print("),\n");
	pFile->PrintIndent("\"1\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"2\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])),\n");
	pFile->PrintIndent("\"3\" (%s->lh.low),\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent("\"4\" (%s->lh.high)\n", (const char*)pObjName->GetName()); /* EDI */
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
	pFile->PrintIndent("\"movl   %%%%edi,%%%%ebp \\n\\t\"\n");
	pFile->PrintIndent("\"movl 8(%%%%esi),%%%%edi \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi \\n\\t\"\n");
	pFile->PrintIndent("ToId32_EdiEsi\n");
	pFile->PrintIndent("\"movl   %%%%ebp,%%%%edi \\n\\t\"\n");
	pFile->PrintIndent("\"movl   $-1,%%%%ebp  \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=c\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=d\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=S\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
	if (bSendFlexpage)
	    pFile->Print("(int)(");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	if (bSendFlexpage)
	    pFile->Print(")|2");
	pFile->Print("),\n"); /* EAX */
	pFile->PrintIndent("\"1\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"D\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, pFunction->GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n"); /* EDI, dw2 */
	pFile->PrintIndent("\"3\" (%s),\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent("\"b\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, pFunction->GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n"); /* EBX, dw1 */
	pFile->PrintIndent("\"2\" (%s)\n", (const char*)pFunction->GetOpcodeConstName()); /* EDX, dw0 (opcode) */
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"ebx\", \"edi\", \"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");

	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief writes the receive IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bAllowShortIPC true if short IPC is allowed
 *  \param pContext the context of the write operation
 */
void CL4X0aIPC::WriteReceive(CBEFile* pFile,  CBEFunction* pFunction,  bool bAllowShortIPC,  CBEContext* pContext)
{
    if (UseAssembler(pFunction, pContext))
	{
	    if (IsShortIPC(pFunction, pContext, pFunction->GetReceiveDirection()) &&
		    bAllowShortIPC)
		    WriteAsmShortReceive(pFile, pFunction, pContext);
	    else
		    WriteAsmLongReceive(pFile, pFunction, pContext);
	}
	else
	    CL4BEIPC::WriteReceive(pFile, pFunction, bAllowShortIPC, pContext); // its a 2 word C binding
}

/** \brief writes short IPC assembler code for the receive case
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write oepration
 */
void CL4X0aIPC::WriteAsmShortReceive(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sDummy = pNF->GetDummyVariable(pContext);

	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=d\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRecvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),\n"); // EDX -> dw0
	pFile->PrintIndent("\"=c\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRecvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),\n"); // ECX (EBX) -> dw1
	pFile->PrintIndent("\"=S\" (%s),\n", (const char*)sDummy); // Id stays the same
	pFile->PrintIndent("\"=D\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRecvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print(")\n"); // EDI -> dw2
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"3\" (%s->lh.low),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"4\" (%s->lh.high)\n", (const char*)pObjName->GetName());
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=d\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRecvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),\n"); // EDX -> dw0
	pFile->PrintIndent("\"=b\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRecvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),\n"); // EBX -> dw1
	pFile->PrintIndent("\"=c\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=S\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=D\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 3, 4, nRecvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print(")\n"); // EDI -> dw2
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"3\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"4\" (%s->lh.low),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"5\" (%s->lh.high)\n", (const char*)pObjName->GetName());
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
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	String sDummy = pNF->GetDummyVariable(pContext);
	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n"); // ECX (EBX) -> dw1
	pFile->PrintIndent("\"=S\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n"); // EDI -> dw2
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n");
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"3\" (%s->lh.low),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"4\" (%s->lh.high)\n", (const char*)pObjName->GetName());
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n"); // EDX -> dw0
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n"); // EBX -> dw1
	pFile->PrintIndent("\"=c\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=S\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n"); // ED) -> dw2
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n"); /* EAX => EBP */
	pFile->PrintIndent("\"3\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"4\" (%s->lh.low),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"5\" (%s->lh.high)\n", (const char*)pObjName->GetName());
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");

	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}
