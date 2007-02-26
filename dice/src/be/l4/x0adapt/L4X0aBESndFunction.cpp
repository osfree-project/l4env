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

#include "be/l4/x0adapt/L4X0aBESndFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEMarshaller.h"

#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CL4X0aBESndFunction);

CL4X0aBESndFunction::CL4X0aBESndFunction()
 : CL4BESndFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0aBESndFunction, CL4BESndFunction);
}


CL4X0aBESndFunction::~CL4X0aBESndFunction()
{
}

/** \brief tests if this function can send a short asm IPC
 *  \param pContext the context of the test
 *  \return true if we should write assembler code for the short IPC
 */
bool CL4X0aBESndFunction::UseAsmShortIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    return ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetSendDirection(), pContext);
}

/** \brief writes the assembler short IPC code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0aBESndFunction::WriteAsmShortIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sDummy = pNF->GetDummyVariable(pContext);
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);

	pFile->Print("#if defined(__PIC__)\n");
	// PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
	pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
	pFile->PrintIndent("\"movl   $-1,%%%%ebp      \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
	pFile->PrintIndent("\"movl   %%%%edi,%%%%ebx  \\n\\t\"\n"); /* dw2 */
	pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi \\n\\t\"\n");
	pFile->PrintIndent("\"ToId32_EdiEsi\n");
	pFile->PrintIndent("\"movl   %%%%eax,%%%%edi  \\n\\t\"\n"); /* dw0 */
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
	pFile->PrintIndent("\"0\" (%s),\n", (const char*)m_sOpcodeConstName); /* EAX => EDI */
	pFile->PrintIndent("\"1\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"2\" (");
    if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n"); /* EDX, dw1 */
	pFile->PrintIndent("\"3\" (%s),\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent("\"4\" (");
    if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetSendDirection(), false, pContext))
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
	pFile->PrintIndent("\"movl   $-1,%%%%ebp  \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
	pFile->PrintIndent("ToId32_EdiEsi\n"); /* EDI,ESI => ESI */
	pFile->PrintIndent("\"movl   %%%%eax,%%%%edi  \\n\\t\"\n");
	pFile->PrintIndent("\"subl   %%%%eax,%%%%eax  \\n\\t\"\n"); /* EAX = 0 (short IPC) */
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=c\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=d\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=S\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (%s),\n", (const char*)m_sOpcodeConstName); /* EAX => EDI */
	pFile->PrintIndent("\"1\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"2\" (");
    if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n"); /* EDX */ 
	pFile->PrintIndent("\"3\" (%s->lh.low),\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent("\"D\" (%s->lh.high),\n", (const char*)pObjName->GetName()); /* EDI */
	pFile->PrintIndent("\"b\" (");
    if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print(")\n"); /* EBX */
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"ebx\", \"edi\", \"memory\"\n");
	pFile->DecIndent();	
	pFile->PrintIndent(");\n");	
	
	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
    
}

/** \brief tests if this function can send a long assembler IPC
 *  \param pContext the context of the test
 *  \return true if we should write the assembler code for the long IPC
 */
bool CL4X0aBESndFunction::UseAsmLongIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    return true;
}

/** \brief write the assembler code for the long IPC
 *  \param pFile the file to write to
 *  \param pContext the context of the write oepration
 */
void CL4X0aBESndFunction::WriteAsmLongIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	String sDummy = pNF->GetDummyVariable(pContext);
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);

	pFile->Print("#if defined(__PIC__)\n");
	// PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
	pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
	pFile->PrintIndent("ToId32_EdiEsi\n");
	pFile->PrintIndent("\"movl  (%%%%edx),%%%%edi \\n\\t\"\n");
	pFile->PrintIndent("\"movl 8(%%%%edx),%%%%ebx \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%edx),%%%%edx \\n\\t\"\n");
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
	if (m_pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print("),\n");
	pFile->PrintIndent("\"1\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"2\" (&(");
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
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
	pFile->PrintIndent("\"=S\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
	if (m_pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print("),\n"); /* EAX */
	pFile->PrintIndent("\"1\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"2\" (");
    if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n"); /* EDX */
	pFile->PrintIndent("\"3\" (%s),\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent("\"b\" (");
    if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n"); /* EBX */
	pFile->PrintIndent("\"D\" (%s)\n", (const char*)m_sOpcodeConstName); /* EDI */
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"ebx\", \"edi\", \"memory\"\n");
	pFile->DecIndent();	
	pFile->PrintIndent(");\n");	
	
	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief write the IPC code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0aBESndFunction::WriteIPC(CBEFile * pFile,  CBEContext * pContext)
{
    if (UseAsmShortIPC(pContext))
	    WriteAsmShortIPC(pFile, pContext);
    else if (UseAsmLongIPC(pContext))
	    WriteAsmLongIPC(pFile, pContext);
    else   
	    CL4BESndFunction::WriteIPC(pFile, pContext);
}

void CL4X0aBESndFunction::WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext)
{
    if (UseAsmShortIPC(pContext) || UseAsmLongIPC(pContext))
	{
		CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
		String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
		String sDummy = pNF->GetDummyVariable(pContext);
		pFile->PrintIndent("%s %s;\n", (const char*)sMWord, (const char*)sDummy);
	}
	CL4BESndFunction::WriteVariableDeclaration(pFile, pContext);
}