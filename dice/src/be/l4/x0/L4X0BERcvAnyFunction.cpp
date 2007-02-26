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

#include "be/l4/x0/L4X0BERcvAnyFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEFile.h"
#include "be/BEContext.h"

#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CL4X0BERcvAnyFunction);

CL4X0BERcvAnyFunction::CL4X0BERcvAnyFunction()
 : CL4BERcvAnyFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0BERcvAnyFunction, CL4BERcvAnyFunction);
}

CL4X0BERcvAnyFunction::~CL4X0BERcvAnyFunction()
{
}

/** \brief test if we can use the assembler code for long IPC
 *  \param pContext the context of the test
 *  \return true if we can use the assembler code
 */
bool CL4X0BERcvAnyFunction::UseAsmLongIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    return true;
}

/** \brief write the assembler long IPC code 
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0BERcvAnyFunction::WriteAsmLongIPC(CBEFile *pFile, CBEContext *pContext)
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
	pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
	pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
	pFile->PrintIndent("\"movl   %%%%eax,%%%%ebp  \\n\\t\"\n");
	pFile->PrintIndent("\"movl   $-1,%%%%eax    \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx  \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebx        \\n\\t\"\n");
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
    if (((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetReceiveDirection(), pContext))
        pFile->Print("L4_IPC_SHORT_MSG");
    else
    {
	    pFile->Print("((int)");
        if (m_pMsgBuffer->HasReference())
            pFile->Print("%s", (const char *) sMsgBuffer);
        else
            pFile->Print("&%s", (const char *) sMsgBuffer);
	    pFile->Print(") & (~L4_IPC_OPEN_IPC)");
    }
	pFile->Print("),\n");
	pFile->PrintIndent("\"2\" (%s)\n", (const char*)sTimeout); /* ECX */
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
    if (((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetReceiveDirection(), pContext))
        pFile->Print("L4_IPC_SHORT_MSG");
    else
    {
	    pFile->Print("((int)");
        if (m_pMsgBuffer->HasReference())
            pFile->Print("%s", (const char *) sMsgBuffer);
        else
            pFile->Print("&%s", (const char *) sMsgBuffer);
	    pFile->Print(") & (~L4_IPC_OPEN_IPC)");
    }
	pFile->Print("),\n"); /* EAX => EBP */
	pFile->PrintIndent("\"2\" (%s)\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();	
	pFile->PrintIndent(");\n");	
	
	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief writes the ipc code 
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This function may receive any message from a specific sender. It is similar to the
 * ipc_receive function. Since it may be any message, we always have a message buffer,
 * and thus we can only write the long IPC code.
 */
void CL4X0BERcvAnyFunction::WriteIPC(CBEFile * pFile,  CBEContext * pContext)
{
    if (UseAsmLongIPC(pContext))
	    WriteAsmLongIPC(pFile, pContext);
    else
	    CL4BERcvAnyFunction::WriteIPC(pFile, pContext);
}
