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

#include "be/l4/x0adapt/L4X0aBEWaitFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEMarshaller.h"
#include "be/BEType.h"

#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CL4X0aBEWaitFunction);

CL4X0aBEWaitFunction::CL4X0aBEWaitFunction()
 : CL4BEWaitFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0aBEWaitFunction, CL4BEWaitFunction);
}

CL4X0aBEWaitFunction::~CL4X0aBEWaitFunction()
{
}

/** \brief writes the ipc code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0aBEWaitFunction::WriteIPC(CBEFile * pFile,  CBEContext * pContext)
{
    if (UseAsmShortIPC(pContext))
	    WriteAsmShortIPC(pFile, pContext);
    else if (UseAsmLongIPC(pContext))
	    WriteAsmLongIPC(pFile, pContext);
    else
	    CL4BEWaitFunction::WriteIPC(pFile, pContext);
}

/** \brief test if we can use the short ipc assembler code
 *  \param pContext the context of the test
 *  \return true if the short ipc code can be written
 */
bool CL4X0aBEWaitFunction::UseAsmShortIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    return ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetReceiveDirection(), pContext);
}

/** \brief test if we can write the long ipc assembler code
 *  \param pContext the context of the write operation
 *  \return true if the assembler code can be used
 */
bool CL4X0aBEWaitFunction::UseAsmLongIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    return true;
}

/** \brief write the short ipc assembler code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This implementation is the same as CL4X0aBEWaitAnyFunction::WriteAsmShortIPC, because
 * the difference between these two classes is implemented in other methods.
 */
void CL4X0aBEWaitFunction::WriteAsmShortIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);

	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
	CBEDeclarator *pRetVar = 0;
    if (m_pReturnVar && !GetReturnType()->IsVoid())
    {
	    pIter = m_pReturnVar->GetFirstDeclarator();
		pRetVar = m_pReturnVar->GetNextDeclarator(pIter);
	}
	String sDummy = pNF->GetDummyVariable(pContext);

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
	if (pRetVar)
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy);
	}
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy);
	}
	pFile->Print("),\n");
	pFile->PrintIndent("\"=c\" (");
    if (pRetVar)
	    pFile->Print("%s", (const char*)pRetVar->GetName());
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy);
	}
	pFile->Print("),\n");
	pFile->PrintIndent("\"=S\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=D\" (");
	if (pRetVar)
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy);
	}
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 3, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy);
	}
	pFile->Print(")\n");
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
	if (pRetVar)
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy);
	}
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 3, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy);
	}
	pFile->Print("),\n"); // EBX
	pFile->PrintIndent("\"=c\" (");
    if (pRetVar)
	    pFile->Print("%s", (const char*)pRetVar->GetName());
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy);
	}
	pFile->Print("),\n"); // ECX
	pFile->PrintIndent("\"=d\" (");
	if (pRetVar)
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy);
	}
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy);
	}
	pFile->Print("),\n"); // EDX
	pFile->PrintIndent("\"=S\" (%s->id.low),\n", (const char*)pObjName->GetName()); // ESI
	pFile->PrintIndent("\"=D\" (%s->id.high)\n", (const char*)pObjName->GetName()); // EDI
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"2\" (%s)\n", (const char*)sTimeout);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();	
	pFile->PrintIndent(");\n");	
	
	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief write the long ipc assembler code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This code is the same as in CL4X0aBEWaitAnyFunction::WriteAsmLongIPC, because the
 * difference between these two classes is handled outside these methods.
 */
void CL4X0aBEWaitFunction::WriteAsmLongIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	String sDummy = pNF->GetDummyVariable(pContext);
	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);

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
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=S\" (%s),\n", (const char*)sDummy);
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
	pFile->PrintIndent("\"movl   %%eax,%%%%ebp      \\n\\t\"\n"); // rcv msg descr
	pFile->PrintIndent("\"movl   $-1,%%%%eax        \\n\\t\"\n"); // no send
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl   %%%%edi,%%%%ecx    \\n\\t\"\n"); // save dw0
	pFile->PrintIndent("FromId32_Esi\n");
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n"); // EBX
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n"); // ECX 
	pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n"); // EDX
	pFile->PrintIndent("\"=S\" (%s->id.low),\n", (const char*)pObjName->GetName()); // ESI
	pFile->PrintIndent("\"=D\" (%s->id.high)\n", (const char*)pObjName->GetName()); // EDI
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (m_pMsgBuffer->HasReference())
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

void CL4X0aBEWaitFunction::WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext)
{
    CL4BEWaitFunction::WriteVariableDeclaration(pFile, pContext);
    if (UseAsmShortIPC(pContext) || UseAsmLongIPC(pContext))
	{
		CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
		String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
		String sDummy = pNF->GetDummyVariable(pContext);
		pFile->Print("#if defined(__PIC__) && !defined(PROFILE)\n");
		pFile->PrintIndent("%s %s;\n", (const char*)sMWord, (const char*)sDummy);
		pFile->Print("#endif\n");
	}
}
