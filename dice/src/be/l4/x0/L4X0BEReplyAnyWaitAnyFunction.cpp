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

#include "be/l4/x0/L4X0BEReplyAnyWaitAnyFunction.h"
#include "fe/FETypeSpec.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEFile.h"
#include "be/BEContext.h"

IMPLEMENT_DYNAMIC(CL4X0BEReplyAnyWaitAnyFunction);

CL4X0BEReplyAnyWaitAnyFunction::CL4X0BEReplyAnyWaitAnyFunction()
 : CL4BEReplyAnyWaitAnyFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0BEReplyAnyWaitAnyFunction, CL4BEReplyAnyWaitAnyFunction);
}

CL4X0BEReplyAnyWaitAnyFunction::~CL4X0BEReplyAnyWaitAnyFunction()
{
}

/** \brief writes the ipc code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0BEReplyAnyWaitAnyFunction::WriteIPC(CBEFile * pFile,  CBEContext * pContext)
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
		WriteAsmShortFpageIPC(pFile, pContext);

		pFile->PrintIndent("if (");
		m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, pContext);
		pFile->Print(".md.fpage_received == 1)\n");
		pFile->IncIndent();
		// short IPC
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
void CL4X0BEReplyAnyWaitAnyFunction::WriteAsmShortIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);

	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);

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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (m_pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC),\n");
	pFile->PrintIndent("\"1\" (&(");
	m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"3\" (%s->raw)\n", (const char*)pObjName->GetName()); /* ESI */
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (m_pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC),\n");
	pFile->PrintIndent("\"1\" (&(");
	m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout); /* EAX => ECX */
	pFile->PrintIndent("\"3\" (%s->raw)\n", (const char*)pObjName->GetName());
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
void CL4X0BEReplyAnyWaitAnyFunction::WriteAsmShortFpageIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);

	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);

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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (m_pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC),\n"); /* EAX => EBP (open ipc) */
	pFile->PrintIndent("\"1\" (&(");
	m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"3\" (%s->raw)\n", (const char*)pObjName->GetName()); /* ESI */
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (m_pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC),\n"); /* EAX => EBP (open ipc) */
	pFile->PrintIndent("\"1\" (&(");
	m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"3\" (%s->raw)\n", (const char*)pObjName->GetName());
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
void CL4X0BEReplyAnyWaitAnyFunction::WriteAsmLongIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);

	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);

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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
	if (m_pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print("),\n"); /* EAX => EBP (open ipc) */
	pFile->PrintIndent("\"1\" (&(");
	m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"3\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"4\" (((int)");
	if (m_pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC),\n"); /* EAX => EBP (open ipc) */
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
	if (m_pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print("),\n"); /* EAX => EBP (open ipc) */
	pFile->PrintIndent("\"1\" (&(");
	m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout); /* EBX => ECX */
	pFile->PrintIndent("\"3\" (%s->raw),\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent("\"4\" (((int)");
	if (m_pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC),\n"); /* EAX => EBP (open ipc) */
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
void CL4X0BEReplyAnyWaitAnyFunction::WriteAsmLongFpageIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);

	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);

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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (m_pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(")|2),\n"); /* EAX => EBP (open ipc) */
	pFile->PrintIndent("\"1\" (&(");
	m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"3\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"4\" (((int)");
	if (m_pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC),\n"); /* EAX => EBP (open ipc) */
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
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (m_pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(")|2),\n"); /* EAX => EBP (open ipc) */
	pFile->PrintIndent("\"1\" (&(");
	m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"3\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"4\" (((int)");
	if (m_pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC),\n"); /* EAX => EBP (open ipc) */
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();	
	pFile->PrintIndent(");\n");	
	
	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}
