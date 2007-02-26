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

#include "be/l4/x0adapt/L4X0aBEReplyWaitFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEMarshaller.h"
#include "be/BEType.h"

#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CL4X0aBEReplyWaitFunction);

CL4X0aBEReplyWaitFunction::CL4X0aBEReplyWaitFunction()
 : CL4BEReplyWaitFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0aBEReplyWaitFunction, CL4BEReplyWaitFunction);
}


CL4X0aBEReplyWaitFunction::~CL4X0aBEReplyWaitFunction()
{
}

/** \brief test if we can write the assembler code for a short ipc
 *  \param pContext the context of the test
 *  \return true if we can write the short ipc code
 */
bool CL4X0aBEReplyWaitFunction::UseAsmShortIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    return ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetSendDirection(), pContext) &&
	       ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetReceiveDirection(), pContext);
}

/** \brief test if we can write the long ipc using assembler
 *  \param pContext the context of the write operation
 *  \return true if we can write the assembler version of the long ipc
 */
bool CL4X0aBEReplyWaitFunction::UseAsmLongIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    return true;
}

/** \brief write the short ipc code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This function replies to a specific function and waits for any message from
 * any client. Therefore do we always receive a long IPC. The difference between
 * WriteAsmShortIPC and WriteAsmLongIPC is the send part.
 */
void CL4X0aBEReplyWaitFunction::WriteAsmShortIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sDummy = pNF->GetDummyVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);

	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);

	pFile->Print("#if defined(__PIC__)\n");
	// PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl  %%%%ebx            \\n\\t\"\n");		
	pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n"); // save ebp no memory references ("m") after this point
	pFile->PrintIndent("\"movl   %%%%edi,%%%%ebx    \\n\\t\"\n"); // dw2
	pFile->PrintIndent("\"pushl  %%%%esi            \\n\\t\"\n"); // save address of thread id
	pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi   \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi   \\n\\t\"\n");
	pFile->PrintIndent("ToId32_EdiEsi\n"); // EDI,ESI => ESI
	pFile->PrintIndent("\"movl   %%%%eax,%%%%edi    \\n\\t\"\n"); // dw0
	pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n"); // short ipc send
	pFile->PrintIndent("\"movl   $1,%%%%ebp    \\n\\t\"\n"); // short ipc recv, from anybody
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl   %%%%edi,%%%%ecx    \\n\\t\"\n"); // save edi -> ecx
	pFile->PrintIndent("FromId32_Esi\n"); // restore 64 Bit Id
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // get address of thread id
	pFile->PrintIndent("\"movl   %%%%esi,(%%%%ebp)  \\n\\t\"\n"); 
	pFile->PrintIndent("\"movl   %%%%edi,4(%%%%ebp) \\n\\t\"\n");
	pFile->PrintIndent("\"movl   %%%%ebx,%%%%edi    \\n\\t\"\n"); // save ebx -> edi
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point		
	pFile->PrintIndent("\"popl   %%%%ebx            \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])))),\n");
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[8])))),\n");
	pFile->PrintIndent("\"=S\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
	if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n");
	pFile->PrintIndent("\"1\" (");
	if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
    pFile->Print("),\n");
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"3\" (");
	if (!pMarshaller->MarshalToPosition(pFile, this, 3, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n");
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
	pFile->PrintIndent("\"pushl  %%%%esi            \\n\\t\"\n"); // save address of thread id
	pFile->PrintIndent("\"movl  4(%%%%esi),%%%%edi  \\n\\t\"\n");
	pFile->PrintIndent("\"movl  0(%%%%esi),%%%%esi  \\n\\t\"\n");
	pFile->PrintIndent("ToId32_EdiEsi\n"); /* EDI,ESI = ESI */
	pFile->PrintIndent("\"movl   %%%%eax,%%%%edi    \\n\\t\"\n"); // dw0
	pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
	pFile->PrintIndent("\"movl   $1,%%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl   %%%%edi,%%%%ecx    \\n\\t\"\n");   // save edi -> ecx
	pFile->PrintIndent("FromId32_Esi\n"); // restore 64 Bit thread id
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");   // restore address of thread id
	pFile->PrintIndent("\"movl   %%%%esi,(%%%%ebp)  \\n\\t\"\n");
	pFile->PrintIndent("\"movl   %%%%edi,4(%%%%ebp) \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point		
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult); // EAX, 0
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])))),\n");
    pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[8])))),\n");
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=S\" (%s)\n", (const char*)sDummy); // ESI, 4
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
	if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n");
	pFile->PrintIndent("\"1\" (");
	if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
    pFile->Print("),\n");
	pFile->PrintIndent("\"2\" (");
	if (!pMarshaller->MarshalToPosition(pFile, this, 3, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n");
	pFile->PrintIndent("\"3\" (%s),\n", (const char*)sTimeout); // ECX
	pFile->PrintIndent("\"4\" (%s)\n", (const char*)pObjName->GetName()); // ESI
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"edi\", \"memory\"\n");
	pFile->DecIndent();	
	pFile->PrintIndent(");\n");	
	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief write the long ipc code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0aBEReplyWaitFunction::WriteAsmLongIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);
	String sDummy = pNF->GetDummyVariable(pContext);
	bool bSendShortIPC = ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetSendDirection(), pContext);
	bool bRecvShortIPC = ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetReceiveDirection(), pContext);
	// l4_fpage_t + 2*l4_msgdope_t
	int nMsgBase = pContext->GetSizes()->GetSizeOfEnvType("l4_fpage_t") +
				pContext->GetSizes()->GetSizeOfEnvType("l4_msgdope_t")*2;
	
	pFile->Print("#if defined(__PIC__)\n");
	// PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl  %%%%ebx            \\n\\t\"\n");		
	pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n");
	pFile->PrintIndent("\"pushl  %%%%esi            \\n\\t\"\n");
	pFile->PrintIndent("\"movl   %%%%edi,%%%%ebp      \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi     \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi     \\n\\t\"\n");
	pFile->PrintIndent("ToId32_EdiEsi\n");
	pFile->PrintIndent("\"movl  (%%%%edx),%%%%edi     \\n\\t\"\n");
	pFile->PrintIndent("\"movl 8(%%%%edx),%%%%ebx     \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%edx),%%%%edx     \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl   %%%%edi,%%%%ecx      \\n\\t\"\n"); // save edi -> ecx
	pFile->PrintIndent("FromId32_Esi\n"); // restore 64 Bit thread id
	pFile->PrintIndent("\"popl   %%%%ebp              \\n\\t\"\n"); // restore address of thread id
	pFile->PrintIndent("\"movl   %%%%esi,(%%%%ebp)    \\n\\t\"\n");
	pFile->PrintIndent("\"movl   %%%%edi,4(%%%%ebp)   \\n\\t\"\n");
	pFile->PrintIndent("\"movl   %%%%ebx,%%%%edi      \\n\\t\"\n"); // save ebx -> edi
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebx            \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n");
	pFile->PrintIndent("\"=S\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
    if (bSendShortIPC)
        pFile->Print("L4_IPC_SHORT_MSG");
    else
    {
		if (m_pMsgBuffer->HasReference())
			pFile->Print("%s", (const char *) sMsgBuffer);
		else
			pFile->Print("&%s", (const char *) sMsgBuffer);
	}
	pFile->Print("),\n");
	pFile->PrintIndent("\"1\" (&(");
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])),\n");
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"3\" (");
    if (bRecvShortIPC)
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
	pFile->Print("),\n");
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
	if (bRecvShortIPC)
	    pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp    \\n\\t\"\n"); // short recv
	else
	{
		pFile->PrintIndent("\"movl   %%%%edi,%%%%ebp    \\n\\t\"\n");
		pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi   \\n\\t\"\n");
		pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi   \\n\\t\"\n");
	}
	pFile->PrintIndent("ToId32_EdiEsi\n");
	if (bSendShortIPC)
	{
		pFile->PrintIndent("\"movl  (%%%%eax),%%%%edi     \\n\\t\"\n");
		pFile->PrintIndent("\"movl 8(%%%%eax),%%%%ebx     \\n\\t\"\n");
		pFile->PrintIndent("\"movl 4(%%%%eax),%%%%edx     \\n\\t\"\n");
		pFile->PrintIndent("\"subl  %%%%eax,%%%%eax       \\n\\t\"\n"); // short IPC
	}
	else
	{
		pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi     \\n\\t\"\n", nMsgBase);
		pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx     \\n\\t\"\n", nMsgBase+8);
		pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx     \\n\\t\"\n", nMsgBase+4);
	}
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl   %%%%edi,%%%%ecx    \\n\\t\"\n"); // save edi -> ecx
	pFile->PrintIndent("FromId32_Esi\n");   // restore 64 Bit thread id
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point		
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
    if (bSendShortIPC)
	{
		pFile->PrintIndent("\"a\" (&(");
		m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
		pFile->Print("[0])),\n");
	}
    else
    {
    	pFile->PrintIndent("\"a\" (");
		if (m_pMsgBuffer->HasReference())
			pFile->Print("%s", (const char *) sMsgBuffer);
		else
			pFile->Print("&%s", (const char *) sMsgBuffer);
    	pFile->Print("),\n");
	}
	pFile->PrintIndent("\"c\" (%s),\n", (const char*)sTimeout);
    if (bRecvShortIPC)
	{
    	pFile->PrintIndent("\"D\" (%s->lh.high),", (const char*)pObjName->GetName());
    	pFile->PrintIndent("\"S\" (%s->lh.low)\n", (const char*)pObjName->GetName());
	}
    else
    {
    	pFile->PrintIndent("\"D\" (((int)");
        if (m_pMsgBuffer->HasReference())
            pFile->Print("%s", (const char *) sMsgBuffer);
        else
            pFile->Print("&%s", (const char *) sMsgBuffer);
	    pFile->Print(") | L4_IPC_OPEN_IPC),\n");
    	pFile->PrintIndent("\"S\" (%s)\n", (const char*)pObjName->GetName());
    }
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
void CL4X0aBEReplyWaitFunction::WriteIPC(CBEFile * pFile,  CBEContext * pContext)
{
    if (UseAsmShortIPC(pContext))
	    WriteAsmShortIPC(pFile, pContext);
    else if (UseAsmLongIPC(pContext))
	    WriteAsmLongIPC(pFile, pContext);
    else
	    CL4BEReplyWaitFunction::WriteIPC(pFile, pContext);
}

/** \brief write the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0aBEReplyWaitFunction::WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext)
{
	CL4BEReplyWaitFunction::WriteVariableDeclaration(pFile, pContext);
    if (UseAsmShortIPC(pContext) || UseAsmLongIPC(pContext))
    {
		CBENameFactory *pNF = pContext->GetNameFactory();
		String sDummy = pNF->GetDummyVariable(pContext);
		String sMWord = pNF->GetTypeName(TYPE_MWORD, false, pContext, 0);
		if (!UseAsmShortIPC(pContext))
			pFile->Print("#if defined(__PIC__)\n");
		pFile->PrintIndent("%s %s;\n", (const char*)sMWord, (const char*)sDummy);
		if (!UseAsmShortIPC(pContext))
			pFile->Print("#endif\n");
    }
}
