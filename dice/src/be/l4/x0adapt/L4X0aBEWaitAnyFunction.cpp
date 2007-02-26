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

#include "be/l4/x0adapt/L4X0aBEWaitAnyFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEFile.h"
#include "be/BEContext.h"

#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CL4X0aBEWaitAnyFunction);

CL4X0aBEWaitAnyFunction::CL4X0aBEWaitAnyFunction()
 : CL4BEWaitAnyFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0aBEWaitAnyFunction, CL4BEWaitAnyFunction);
}

CL4X0aBEWaitAnyFunction::~CL4X0aBEWaitAnyFunction()
{
}

/** \brief write the ipc ode
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * Since we wait for any message, we do not have specific parameters to receive
 * a short msg, so we write only long ipc code.
 */
void CL4X0aBEWaitAnyFunction::WriteIPC(CBEFile * pFile,  CBEContext * pContext)
{
    if (UseAsmLongIPC(pContext))
	    WriteAsmLongIPC(pFile, pContext);
    else
	    CL4BEWaitAnyFunction::WriteIPC(pFile, pContext);
}

/** \brief test if the long ipc can be sent using assembler code
 *  \param pContext the context of the test
 *  \return true if the assembler code can be written
 */
bool CL4X0aBEWaitAnyFunction::UseAsmLongIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    return true;
}

/** \brief writes the assembler code for the long ipc
 *  \param pFile the file to write to
 *  \param pContext the context of the write oepration
 */
void CL4X0aBEWaitAnyFunction::WriteAsmLongIPC(CBEFile *pFile, CBEContext *pContext)
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
	pFile->PrintIndent("\"pushl  %%%%esi            \\n\\t\"\n"); // save thread id address
	pFile->PrintIndent("\"movl   %%%%edx,%%%%ebp    \\n\\t\"\n"); // rcv descr
    pFile->PrintIndent("\"movl   $-1,%%%%eax        \\n\\t\"\n"); // no send	
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl   %%%%edi,%%%%ecx    \\n\\t\"\n"); // save edi in ecx (dw0)
	pFile->PrintIndent("FromId32_Esi\n");
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // get thread id pointer
	pFile->PrintIndent("\"movl   %%%%esi,(%%%%ebp)  \\n\\t\"\n"); // low
	pFile->PrintIndent("\"movl   %%%%edi,4(%%%%ebp) \\n\\t\"\n"); // high
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
	pFile->PrintIndent("\"movl   %%%%ebx,%%%%esi    \\n\\t\"\n"); // save ebx in esi (dw2)
	pFile->PrintIndent("\"popl   %%%%ebx            \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=D\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=S\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"1\" (((int)");
	if (m_pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC),\n");
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"4\" (%s)\n", (const char*)pObjName->GetName());
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
	pFile->PrintIndent("\"movl   %%%%ebx,%%%%ebp    \\n\\t\"\n"); /* rcv short ipc, open wait */
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl   %%%%edi,%%%%ecx    \\n\\t\"\n"); // save edi in ecx (dw0)
	pFile->PrintIndent("FromId32_Esi\n");
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=S\" (%s->lh.low),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (%s->lh.high)\n", (const char*)pObjName->GetName());
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (L4_IPC_NIL_DESCRIPTOR),\n");
	pFile->PrintIndent("\"3\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"2\" (((int)");
	if (m_pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC)\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();	
	pFile->PrintIndent(");\n");	
	
	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

void CL4X0aBEWaitAnyFunction::WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext)
{
    if (UseAsmLongIPC(pContext))
	{
		CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
		String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
		String sDummy = pNF->GetDummyVariable(pContext);
		pFile->Print("#if defined(__PIC__) && !defined(PROFILE)\n");
		pFile->PrintIndent("%s %s;\n", (const char*)sMWord, (const char*)sDummy);
		pFile->Print("#endif\n");
	}
	CL4BEWaitAnyFunction::WriteVariableDeclaration(pFile, pContext);
}
