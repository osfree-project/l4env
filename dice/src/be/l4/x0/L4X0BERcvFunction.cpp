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

#include "be/l4/x0/L4X0BERcvFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEType.h"
#include "be/BEMarshaller.h"

#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CL4X0BERcvFunction);

CL4X0BERcvFunction::CL4X0BERcvFunction()
 : CL4BERcvFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0BERcvFunction, CL4BERcvFunction);
}

CL4X0BERcvFunction::~CL4X0BERcvFunction()
{
}

/** \brief test if we can write the assembler version of the short ipc
 *  \param pContext the context of the test
 *  \return true if the assembler version can be written
 */
bool CL4X0BERcvFunction::UseAsmShortIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    return ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetReceiveDirection(), pContext);
}

/** \brief test if we can write the assembler version of the long ipc
 *  \param pContext the context of the test
 *  \return true if the assembler version can be written
 */
bool CL4X0BERcvFunction::UseAsmLongIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    return true;
}

/** \brief write the short IPC in assembler
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This is the same code as in CL4X0BERcvAnyFunction::WriteAsmShortIPC, since these
 * classes differentiate in other functions than this one, but have no common
 * X0 class, wich could implement this IPC.
 */
void CL4X0BERcvFunction::WriteAsmShortIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sDummy0 = pNF->GetDummyVariable(pContext);
	String sDummy1 = sDummy0;
	String sDummy2 = sDummy0;
	sDummy0 += "0";
	sDummy1 += "1";
	sDummy2 += "2";

	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
	CBEDeclarator *pRetVar = 0;
    if (m_pReturnVar && !GetReturnType()->IsVoid())
    {
	    pIter = m_pReturnVar->GetFirstDeclarator();
		pRetVar = m_pReturnVar->GetNextDeclarator(pIter);
	}

	pFile->Print("#if defined(__PIC__)\n");
	// PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
	pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
	pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp   \\n\\t\"\n");
	pFile->PrintIndent("\"movl   $-1,%%%%eax    \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx  \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebx        \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=d\" (");
    if (pRetVar)
	    pFile->Print("%s", (const char*)pRetVar->GetName());
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy0);
	}
	pFile->Print("),                /* EDX, 1 */\n");
	pFile->PrintIndent("\"=c\" (");
	if (pRetVar)
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy0);
	}
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy1);
	}
	pFile->Print("),                /* ECX, 2 */\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (");
	if (pRetVar)
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy1);
	}
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 3, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy2);
	}
	pFile->Print(")                 /* EDI, 3 */\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"2\" (%s)\n", (const char*)sTimeout);
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
	pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp   \\n\\t\"\n");
	pFile->PrintIndent("\"movl   $-1,%%%%eax    \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=d\" (");
    if (pRetVar)
	    pFile->Print("%s", (const char*)pRetVar->GetName());
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy0);
	}
	pFile->Print("),                /* EDX, 1 */\n");
	pFile->PrintIndent("\"=b\" (");
	if (pRetVar)
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy0);
	}
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy1);
	}
	pFile->Print("),                /* ECX, 2 */\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (");
	if (pRetVar)
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy1);
	}
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 3, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy2);
	}
	pFile->Print(")                 /* EDI, 3 */\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"c\" (%s)\n", (const char*)sTimeout);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();	
	pFile->PrintIndent(");\n");	
	
	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief write the long IPC in assembler
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This is the same code as in CL4X0BERcvAnyFunction::WriteAsmShortIPC, since these
 * classes differentiate in other functions than this one, but have no common
 * X0 class, wich could implement this IPC.
 */
void CL4X0BERcvFunction::WriteAsmLongIPC(CBEFile *pFile, CBEContext *pContext)
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
	pFile->Print("),\n"); /* EAX => EBP */
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

/** \brief write the ipc code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0BERcvFunction::WriteIPC(CBEFile *pFile, CBEContext *pContext)
{
    if (UseAsmShortIPC(pContext))
	    WriteAsmShortIPC(pFile, pContext);
    else if (UseAsmLongIPC(pContext))
	    WriteAsmLongIPC(pFile, pContext);
    else
	    CL4BERcvFunction::WriteIPC(pFile, pContext);
}
