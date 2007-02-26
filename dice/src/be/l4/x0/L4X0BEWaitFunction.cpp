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

#include "be/l4/x0/L4X0BEWaitFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEMarshaller.h"
#include "be/BEType.h"

#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CL4X0BEWaitFunction);

CL4X0BEWaitFunction::CL4X0BEWaitFunction()
 : CL4BEWaitFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0BEWaitFunction, CL4BEWaitFunction);
}

CL4X0BEWaitFunction::~CL4X0BEWaitFunction()
{
}

/** \brief writes the ipc code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0BEWaitFunction::WriteIPC(CBEFile * pFile,  CBEContext * pContext)
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
bool CL4X0BEWaitFunction::UseAsmShortIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    return ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetReceiveDirection(), pContext);
}

/** \brief test if we can write the long ipc assembler code
 *  \param pContext the context of the write operation
 *  \return true if the assembler code can be used
 */
bool CL4X0BEWaitFunction::UseAsmLongIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    return true;
}

/** \brief write the short ipc assembler code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This implementation is the same as CL4X0BEWaitAnyFunction::WriteAsmShortIPC, because
 * the difference between these two classes is implemented in other methods.
 */
void CL4X0BEWaitFunction::WriteAsmShortIPC(CBEFile *pFile, CBEContext *pContext)
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
	String sDummy1 = pNF->GetDummyVariable(pContext);
	String sDummy2 = sDummy1;
	String sDummy3 = sDummy1;
	String sDummy4 = sDummy1;
	sDummy1 += "0";
	sDummy2 += "1";
	sDummy3 += "2";
	sDummy4 += "3";

	pFile->Print("#if defined(__PIC__)\n");
	// PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl  %%%%ebx            \\n\\t\"\n");		
	pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n"); // save ebp no memory references ("m") after this point
	pFile->PrintIndent("\"movl   $1,%%%%ebp           \\n\\t\"\n"); /* rcv short ipc, open wait */
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
	pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx      \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebx            \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=d\" (");
    if (pRetVar)
	    pFile->Print("%s", (const char*)pRetVar->GetName());
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy2);
	}
	pFile->Print("),                /* EDX, 1 */\n");
	pFile->PrintIndent("\"=c\" (");
	if (pRetVar)
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy2);
	}
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy3);
	}
	pFile->Print("),                /* ECX, 2 */\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (");
	if (pRetVar)
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy3);
	}
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 3, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy4);
	}
	pFile->Print(")                 /* EDI, 3 */\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (L4_IPC_NIL_DESCRIPTOR),\n");
	pFile->PrintIndent("\"1\" (%s)\n", (const char*)sTimeout);
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
	pFile->PrintIndent("\"movl   %%%%edx,%%%%ecx      \\n\\t\"\n"); /* timeout, ecx */
	pFile->PrintIndent("\"movl   $1,%%%%ebp           \\n\\t\"\n"); /* rcv short ipc, open wait */
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=d\" (");
    if (pRetVar)
	    pFile->Print("%s", (const char*)pRetVar->GetName());
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy2);
	}
	pFile->Print("),                /* EDX, 1 */\n");
	pFile->PrintIndent("\"=b\" (");
	if (pRetVar)
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy2);
	}
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy3);
	}
	pFile->Print("),                /* ECX, 2 */\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (");
	if (pRetVar)
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy3);
	}
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 3, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy4);
	}
	pFile->Print(")                 /* EDI, 3 */\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (L4_IPC_NIL_DESCRIPTOR),\n");
	pFile->PrintIndent("\"1\" (%s)\n", (const char*)sTimeout);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"ecx\", \"memory\"\n");
	pFile->DecIndent();	
	pFile->PrintIndent(");\n");	
	
	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief write the long ipc assembler code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This code is the same as in CL4X0BEWaitAnyFunction::WriteAsmLongIPC, because the
 * difference between these two classes is handled outside these methods.
 */
void CL4X0BEWaitFunction::WriteAsmLongIPC(CBEFile *pFile, CBEContext *pContext)
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
	pFile->PrintIndent("\"pushl  %%%%ebx            \\n\\t\"\n");		
	pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n"); // save ebp no memory references ("m") after this point
	pFile->PrintIndent("\"movl   %%%%edx,%%%%ebp      \\n\\t\"\n"); /* rcv msg, open wait */
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
	pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx      \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebx            \\n\\t\"\n");
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
	pFile->PrintIndent("\"0\" (L4_IPC_NIL_DESCRIPTOR),\n");
	pFile->PrintIndent("\"1\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"2\" (");
    if (((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetReceiveDirection(), pContext))
        pFile->Print("L4_IPC_SHORT_MSG");
    else
    {
	    pFile->Print("((int)");
        if (m_pMsgBuffer->HasReference())
            pFile->Print("%s", (const char *) sMsgBuffer);
        else
            pFile->Print("&%s", (const char *) sMsgBuffer);
	    pFile->Print(") | L4_IPC_OPEN_IPC");
    }
	pFile->Print(")\n");
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
	pFile->PrintIndent("\"movl   %%%%edx,%%%%ecx      \\n\\t\"\n"); /* timeout, ecx */
	pFile->PrintIndent("\"movl   %%%%ebx,%%%%ebp      \\n\\t\"\n"); /* rcv short ipc, open wait */
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
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
	pFile->PrintIndent("\"0\" (L4_IPC_NIL_DESCRIPTOR),\n");
	pFile->PrintIndent("\"1\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"2\" (");
    if (((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetReceiveDirection(), pContext))
        pFile->Print("L4_IPC_SHORT_MSG");
    else
    {
	    pFile->Print("((int)");
        if (m_pMsgBuffer->HasReference())
            pFile->Print("%s", (const char *) sMsgBuffer);
        else
            pFile->Print("&%s", (const char *) sMsgBuffer);
	    pFile->Print(") | L4_IPC_OPEN_IPC");
    }
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"ecx\", \"memory\"\n");
	pFile->DecIndent();	
	pFile->PrintIndent(");\n");	
	
	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}
