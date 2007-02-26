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

#include "be/l4/x0adapt/L4X0aBEReplyRcvFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEMarshaller.h"
#include "be/BEType.h"

#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CL4X0aBEReplyRcvFunction);

CL4X0aBEReplyRcvFunction::CL4X0aBEReplyRcvFunction()
 : CL4BEReplyRcvFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0aBEReplyRcvFunction, CL4BEReplyRcvFunction);
}

CL4X0aBEReplyRcvFunction::~CL4X0aBEReplyRcvFunction()
{
}

/** \brief test if we can use a short ipc
 *  \param pContext the context of the test
 *  \return true if we can write the short ipc assembler code
 */
bool CL4X0aBEReplyRcvFunction::UseAsmShortIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    return ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetSendDirection(), pContext) &&
	       ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetReceiveDirection(), pContext);
}

/** \brief test if we can use the long ipc assembler code
 *  \param pContext the context of the test
 *  \return true if we can write assembler code
 */
bool CL4X0aBEReplyRcvFunction::UseAsmLongIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    return true;
}

/** \brief write assembler code for the short ipc
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * Since the reply and receive function is the same (on ipc level) as a call
 * we use the same code as CL4X0aBECallFunction::WriteAsmShortIPC.
 *
 * Reply-Rcv does reply to a message from a specific client and wait for
 * another message from the same client.
 * Call does send a message to a specific server and wait for the reply to
 * this message.
 *
 * In terms of IPC its the same: communicate with dedicated partner.
 */
void CL4X0aBEReplyRcvFunction::WriteAsmShortIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sDummy = pNF->GetDummyVariable(pContext);

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
	pFile->PrintIndent("\"pushl  %%%%ebx            \\n\\t\"\n");		
	pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n"); // save ebp no memory references ("m") after this point
	pFile->PrintIndent("\"movl   %%%%edi,%%%%ebx    \\n\\t\"\n"); // dw2
	pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi   \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi   \\n\\t\"\n");
	pFile->PrintIndent("ToId32_EdiEsi\n"); // EDI,ESI => ESI
	pFile->PrintIndent("\"movl   %%%%eax,%%%%edi    \\n\\t\"\n"); // dw0
	pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n"); // short ipc send
	pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp    \\n\\t\"\n"); // short ipc recv
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx    \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point		
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
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy);
	}
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 3, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy);
	}
	pFile->Print("),\n");
	pFile->PrintIndent("\"=D\" (");
    if (pRetVar)
	    pFile->Print("%s", (const char*)pRetVar->GetName());
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy);
	}
	pFile->Print("),\n");
	pFile->PrintIndent("\"=S\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (%s),\n", (const char*)m_sOpcodeConstName);
	pFile->PrintIndent("\"1\" (");
    if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n");
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"3\" (");
    if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetSendDirection(), false, pContext))
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
	pFile->PrintIndent("\"movl  4(%%%%esi),%%%%edi  \\n\\t\"\n");
	pFile->PrintIndent("\"movl  0(%%%%esi),%%%%esi  \\n\\t\"\n");
	pFile->PrintIndent("ToId32_EdiEsi\n"); /* EDI,ESI = ESI */
	pFile->PrintIndent("\"movl   %%%%eax,%%%%edi            \\n\\t\"\n"); // dw0
	pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
	pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp    \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point		
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult); // EAX, 0
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
	pFile->Print("),\n"); // EDX, 1
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
	pFile->Print("),\n"); // EBX, 2
	pFile->PrintIndent("\"=D\" (");
    if (pRetVar)
	    pFile->Print("%s", (const char*)pRetVar->GetName());
	else
	{
		if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy);
	}
	pFile->Print("),\n"); // EDI, 3
	pFile->PrintIndent("\"=c\" (%s)\n", (const char*)sDummy); // ECX, 4
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"S\" (%s),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"0\" (%s),\n", (const char*)m_sOpcodeConstName); // EAX, dw0
	pFile->PrintIndent("\"1\" (");
    if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n"); // EDX, dw1
	pFile->PrintIndent("\"2\" (");
    if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n"); // EDI, 3
	pFile->PrintIndent("\"4\" (%s)\n", (const char*)sTimeout); // ECX
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();	
	pFile->PrintIndent(");\n");	
	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief write assembler code for the long ipc
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * Since the reply and receive function is the same (on ipc level) as a call
 * we use the same code as CL4X0aBECallFunction::WriteAsmLongIPC.
 *
 * Reply-Rcv does reply to a message from a specific client and wait for
 * another message from the same client.
 * Call does send a message to a specific server and wait for the reply to
 * this message.
 *
 * In terms of IPC its the same: communicate with dedicated partner.
 */
void CL4X0aBEReplyRcvFunction::WriteAsmLongIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);
	String sDummy = pNF->GetDummyVariable(pContext);
	
	pFile->Print("#if defined(__PIC__)\n");
	// PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl  %%%%ebx            \\n\\t\"\n");		
	pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n");
	pFile->PrintIndent("\"movl   %%%%edi,%%%%ebp      \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi     \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi     \\n\\t\"\n");
	pFile->PrintIndent("ToId32_EdiEsi\n");
	pFile->PrintIndent("\"movl  (%%%%edx),%%%%edi     \\n\\t\"\n");
	pFile->PrintIndent("\"movl 8(%%%%edx),%%%%ebx     \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%edx),%%%%edx     \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx      \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebx            \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n");
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=S\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
    if (((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetSendDirection(), pContext))
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
	pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx    \\n\\t\"\n");
	pFile->PrintIndent("\"movl   %%%%edi,%%%%ebp    \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi   \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi   \\n\\t\"\n");
	pFile->PrintIndent("ToId32_EdiEsi\n");
	pFile->PrintIndent("\"movl  (%%%%edx),%%%%edi     \\n\\t\"\n");
	pFile->PrintIndent("\"movl 8(%%%%edx),%%%%ebx     \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%edx),%%%%edx     \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point		
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n");
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=S\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
    if (((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetSendDirection(), pContext))
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
	pFile->PrintIndent("\"4\" (%s)\n", (const char*)pObjName->GetName());
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"ecx\", \"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");
	
	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief writes the ipc code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0aBEReplyRcvFunction::WriteIPC(CBEFile *pFile, CBEContext *pContext)
{
    if (UseAsmShortIPC(pContext))
	    WriteAsmShortIPC(pFile, pContext);
    else if (UseAsmLongIPC(pContext))
	    WriteAsmLongIPC(pFile, pContext);
    else   
	    CL4BEReplyRcvFunction::WriteIPC(pFile, pContext);
}

/** \brief write the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0aBEReplyRcvFunction::WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext)
{
	CL4BEReplyRcvFunction::WriteVariableDeclaration(pFile, pContext);
    if (UseAsmShortIPC(pContext) || UseAsmLongIPC(pContext))
    {
		CBENameFactory *pNF = pContext->GetNameFactory();
		String sDummy = pNF->GetDummyVariable(pContext);
		String sMWord = pNF->GetTypeName(TYPE_MWORD, false, pContext, 0);
		pFile->PrintIndent("%s %s;\n", (const char*)sMWord, (const char*)sDummy);
    }
}
