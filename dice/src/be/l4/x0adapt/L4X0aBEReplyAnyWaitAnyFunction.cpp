/* Copyright (C) 2001-2003 by
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

#include "be/l4/x0adapt/L4X0aBEReplyAnyWaitAnyFunction.h"
#include "fe/FETypeSpec.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEFile.h"
#include "be/BEContext.h"

IMPLEMENT_DYNAMIC(CL4X0aBEReplyAnyWaitAnyFunction);

CL4X0aBEReplyAnyWaitAnyFunction::CL4X0aBEReplyAnyWaitAnyFunction()
 : CL4BEReplyAnyWaitAnyFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0aBEReplyAnyWaitAnyFunction, CL4BEReplyAnyWaitAnyFunction);
}

CL4X0aBEReplyAnyWaitAnyFunction::~CL4X0aBEReplyAnyWaitAnyFunction()
{
}

/** \brief writes the ipc code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0aBEReplyAnyWaitAnyFunction::WriteIPC(CBEFile * pFile,  CBEContext * pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    CL4BEReplyAnyWaitAnyFunction::WriteIPC(pFile, pContext);
    else
	{
		// to determine if we can send a short IPC we have to test the size dope of the message
		pFile->PrintIndent("if ((");
		m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, pContext);
		pFile->Print(".md.dwords <= 2) && (");
		m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, pContext);
		pFile->Print(".md.strings == 0))\n");
		pFile->IncIndent();
		// if fpage

		pFile->PrintIndent("if (");
		m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, pContext);
		pFile->Print(".md.fpage_received == 1)\n");
		pFile->IncIndent();
		// short IPC
		WriteAsmShortFpageIPC(pFile, pContext);
		// else (fpage)
		pFile->DecIndent();
		pFile->PrintIndent("else\n");
		pFile->IncIndent();
		// !fpage
		WriteAsmShortIPC(pFile, pContext);

		pFile->DecIndent();
		pFile->DecIndent();
		pFile->PrintIndent("else\n");
		pFile->IncIndent();
		// if fpage
		pFile->PrintIndent("if (");
		m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, pContext);
		pFile->Print(".md.fpage_received == 1)\n");
		pFile->IncIndent();
		// long IPC
		WriteAsmLongFpageIPC(pFile, pContext);
		// else (fpage)
		pFile->DecIndent();
		pFile->PrintIndent("else\n");
		pFile->IncIndent();
		// ! fpage
		WriteAsmLongIPC(pFile, pContext);

		pFile->DecIndent();
		pFile->DecIndent();
	}
}

/** \brief write a short ipc with assembler code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0aBEReplyAnyWaitAnyFunction::WriteAsmShortIPC(CBEFile *pFile, CBEContext *pContext)
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
	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);

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
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi  \\n\\t\"\n", nMsgBase); // dw0 => edi
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+8); // dw2 => ebx
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase+4); // dw1 => edx
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
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n");
	pFile->PrintIndent("\"=D\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"a\" (");
	if (m_pMsgBuffer->HasReference())
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
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi  \\n\\t\"\n", nMsgBase); // dw0 => edi
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+8); // dw2 => ebx
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase+4); // dw1 => edx
	pFile->PrintIndent("\"subl  %%%%eax,%%%%eax     \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl  %%%%edi,%%%%ecx     \\n\\t\"\n"); // edi => ecx (dw0)
	pFile->PrintIndent("FromId32_Esi\n");
	pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->lh.low),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (%s->lh.high)\n", (const char*)pObjName->GetName());
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"a\" (");
	if (m_pMsgBuffer->HasReference())
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

/** \brief write a short flexpage ipc with assembler code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0aBEReplyAnyWaitAnyFunction::WriteAsmShortFpageIPC(CBEFile *pFile, CBEContext *pContext)
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
	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);

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
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi  \\n\\t\"\n", nMsgBase); // dw0 => edi
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+8); // dw2 => ebx
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase+4); // dw1 => edx
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
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n");
	pFile->PrintIndent("\"=D\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"a\" (");
	if (m_pMsgBuffer->HasReference())
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
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi  \\n\\t\"\n", nMsgBase); // dw0 => edi
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+8); // dw2 => ebx
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase+4); // dw1 => edx
	pFile->PrintIndent("\"movl  $2,%%%%eax          \\n\\t\"\n"); // fpage
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl  %%%%edi,%%%%ecx     \\n\\t\"\n"); // edi => ecx (dw0)
	pFile->PrintIndent("FromId32_Esi\n");
	pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->lh.low),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (%s->lh.high)\n", (const char*)pObjName->GetName());
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"a\" (");
	if (m_pMsgBuffer->HasReference())
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

/** \brief write a long ipc with assembler code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0aBEReplyAnyWaitAnyFunction::WriteAsmLongIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	// l4_fpage_t + 2*l4_msgdope_t
	int nMsgBase = pContext->GetSizes()->GetSizeOfEnvType("l4_fpage_t") +
				pContext->GetSizes()->GetSizeOfEnvType("l4_msgdope_t")*2;
	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);
	String sDummy = pNF->GetDummyVariable(pContext);

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
	pFile->PrintIndent("\"movl %d(%%%%edx),%%%%ebx  \\n\\t\"\n", nMsgBase+8); // dw2 -> ebx
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase+4); // dw1 -> edx
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi  \\n\\t\"\n", nMsgBase);   // dw0 -> edi
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
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent("\"=D\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"a\" (");
	if (m_pMsgBuffer->HasReference())
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
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi  \\n\\t\"\n", nMsgBase); // dw0 -> edi
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+8); // dw2 -> ebx
	pFile->PrintIndent("\"movl %d(%%%%edx),%%%%edx  \\n\\t\"\n", nMsgBase+4); // dw1 -> edx
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl  %%%%edi,%%%%ecx     \\n\\t\"\n"); // save edi in ecx
	pFile->PrintIndent("FromId32_Esi\n");
	pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n");
	pFile->PrintIndent("\"=S\" (%s->lh.low),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (%s->lh.high)\n", (const char*)pObjName->GetName());
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"a\" (");
	if (m_pMsgBuffer->HasReference())
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

/** \brief write a long flexpage ipc with assembler code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0aBEReplyAnyWaitAnyFunction::WriteAsmLongFpageIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	// l4_fpage_t + 2*l4_msgdope_t
	int nMsgBase = pContext->GetSizes()->GetSizeOfEnvType("l4_fpage_t") +
				pContext->GetSizes()->GetSizeOfEnvType("l4_msgdope_t")*2;
	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);
	String sDummy = pNF->GetDummyVariable(pContext);

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
	pFile->PrintIndent("\"movl %d(%%%%edx),%%%%ebx  \\n\\t\"\n", nMsgBase+8); // dw2 -> ebx
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase+4); // dw1 -> edx
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi  \\n\\t\"\n", nMsgBase);   // dw0 -> edi
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
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent("\"=D\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"a\" (");
	if (m_pMsgBuffer->HasReference())
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
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi  \\n\\t\"\n", nMsgBase); // dw0 -> edi
	pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+8); // dw2 -> ebx
	pFile->PrintIndent("\"movl %d(%%%%edx),%%%%edx  \\n\\t\"\n", nMsgBase+4); // dw1 -> edx
	pFile->PrintIndent("\"orl   $2,%%%%eax          \\n\\t\"\n"); // fpage ipc
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl  %%%%edi,%%%%ecx     \\n\\t\"\n"); // save edi in ecx
	pFile->PrintIndent("FromId32_Esi\n");
	pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n");
	pFile->PrintIndent("\"=S\" (%s->lh.low),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (%s->lh.high)\n", (const char*)pObjName->GetName());
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"a\" (");
	if (m_pMsgBuffer->HasReference())
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

void CL4X0aBEReplyAnyWaitAnyFunction::WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext)
{
    CL4BEReplyAnyWaitAnyFunction::WriteVariableDeclaration(pFile, pContext);
    if (!pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	{
		CBENameFactory *pNF = pContext->GetNameFactory();
		String sDummy = pNF->GetDummyVariable(pContext);
		String sMWord = pNF->GetTypeName(TYPE_MWORD, false, pContext, 0);
    	pFile->Print("#if defined(__PIC__)\n");
		pFile->PrintIndent("%s %s;\n", (const char*)sMWord, (const char*)sDummy);
		pFile->Print("#endif // PIC\n");
	}
}
