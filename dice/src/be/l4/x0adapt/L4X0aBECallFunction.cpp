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

#include "be/l4/x0adapt/L4X0aBECallFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEDeclarator.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEMarshaller.h"
#include "be/BEType.h"

#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CL4X0aBECallFunction);

CL4X0aBECallFunction::CL4X0aBECallFunction()
 : CL4BECallFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0aBECallFunction, CL4BECallFunction);
}


CL4X0aBECallFunction::~CL4X0aBECallFunction()
{
}

/** \brief test if we can use the assembler short IPC
 *  \param pContext the context of the test
 *  \return true if the assembler short IPC can be used
 */
bool CL4X0aBECallFunction::UseAsmShortIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType*)m_pMsgBuffer;
	return pMsgBuffer->IsShortIPC(GetSendDirection(), pContext) &&
	       pMsgBuffer->IsShortIPC(GetReceiveDirection(), pContext);
}

/** \brief prints the IPC call
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * We only write for PIC and !PIC && !PROFILE (anything else is currently not
 * supported.
 */
void CL4X0aBECallFunction::WriteIPC(CBEFile * pFile,  CBEContext * pContext)
{
    if (UseAsmShortIPC(pContext))
	    WriteAsmShortIPC(pFile, pContext);
	else if (UseAsmLongIPC(pContext))
	    WriteAsmLongIPC(pFile, pContext);
	else
	    CL4BECallFunction::WriteIPC(pFile, pContext);
}

/** \brief Writes the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * If we have a short IPC in both direction, we only need the result dope,
 * and two dummy dwords.
 */
void CL4X0aBECallFunction::WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext)
{
	CBENameFactory *pNF = pContext->GetNameFactory();
	String sDummy = pNF->GetDummyVariable(pContext);
	String sMWord = pNF->GetTypeName(TYPE_MWORD, false, pContext, 0);
    if (UseAsmShortIPC(pContext))
	{
		// write dummys
		String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
		
		// test if we need dummies
		pFile->Print("#if defined(__PIC__)\n");
		WriteReturnVariableDeclaration(pFile, pContext);
		// write result variable
		pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", (const char *) sResult);
		pFile->PrintIndent("%s %s __attribute__((unused));\n", 
		                    (const char*)sMWord, (const char*)sDummy);
		
		pFile->Print("#else // !PIC\n");
		pFile->Print("#if !defined(PROFILE)\n");
		WriteReturnVariableDeclaration(pFile, pContext);
		// write result variable
		pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", (const char *) sResult);
		pFile->PrintIndent("%s %s = 0;\n", (const char *)sMWord, (const char*)sDummy);
		pFile->Print("#endif // PROFILE\n");
		pFile->Print("#endif // !PIC\n");
		// if we have in either direction some bit-stuffing, we need more dummies
	    // finished with declaration
	}
	else
	{
	    CL4BECallFunction::WriteVariableDeclaration(pFile, pContext);
	    // we need the dummy for the assembler statements
        if (UseAsmLongIPC(pContext))
			pFile->PrintIndent("%s %s;\n", (const char*)sMWord, (const char*)sDummy);
	}
}

/** \brief writes the variable initialization
 *  \param pFile the file to write to
 *  \param pContext the context of teh write operation
 *
 * If we use the asm short IPC code we do not init any variables (Its done during declaration).
 */
void CL4X0aBECallFunction::WriteVariableInitialization(CBEFile * pFile,  CBEContext * pContext)
{
    if (UseAsmShortIPC(pContext))
	{
	}
	else
	    CL4BECallFunction::WriteVariableInitialization(pFile, pContext);
}

/* \brief write the marshalling code for the short IPC    
 * \param pFile the file to write to    
 * \param nStartOffset the offset where to start marshalling    
 * \param bUseConstOffset true if nStartOffset can be used    
 * \param pContext the context of the write operation    
 * 
 * If we have a short IPC into both direction, we skip the marshalling.  
 */
void CL4X0aBECallFunction::WriteMarshalling(CBEFile * pFile,  int nStartOffset,   
                           bool & bUseConstOffset,  CBEContext * pContext)
{      
    if (UseAsmShortIPC(pContext))
	{
	    // if short IPC consist of more than one parameter (bit-stuffing), then we
		// have to stuff now               
	}
	else
	    CL4BECallFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
}

/* \brief write the unmarshalling code for the short IPC   
 * \param pFile the file to write to   
 * \param nStartOffset the offset where to start marshalling   
 * \param bUseConstOffset true if nStartOffset can be used   
 * \param pContext the context of the write operation   
 *
 * If we have a short IPC in both direction, we can skip the unmarshalling. 
 */
void CL4X0aBECallFunction::WriteUnmarshalling(CBEFile * pFile,  int nStartOffset, 
                            bool & bUseConstOffset,  CBEContext * pContext)
{   
    if (UseAsmShortIPC(pContext))       
	{
	    // if short IPC consists of more than two parameters (bit-stuffing), then we
		// have to "unstuff" now
	}
	else
        CL4BECallFunction::WriteUnmarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
}

/** \brief writes the invocation code for the short IPC
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * If we have a short IPC in both direction, we write assembler code
 * directly.
 */
void CL4X0aBECallFunction::WriteInvocation(CBEFile * pFile,  CBEContext * pContext)
{
    if (UseAsmShortIPC(pContext))
    {
	    // skip send dope init
	    WriteIPC(pFile, pContext);
		WriteIPCErrorCheck(pFile, pContext);
    }   
	else       
        CL4BECallFunction::WriteInvocation(pFile, pContext);
}

/** \brief writes the assembler version of the short IPC
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This is only called for UseAsmShortIPC == true, which means that we
 * have a short IPC into both directions, which allows us some optimizations.
 */
void CL4X0aBECallFunction::WriteAsmShortIPC(CBEFile *pFile, CBEContext *pContext)
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
	pFile->PrintIndent("\"movl  0(%%%%edi),%%%%esi  \\n\\t\"\n");
	pFile->PrintIndent("\"movl  4(%%%%edi),%%%%edi  \\n\\t\"\n");
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
	pFile->PrintIndent("\"0\" (%s),\n", (const char*)m_sOpcodeConstName); // EAX, dw0
	pFile->PrintIndent("\"1\" (");
    if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n"); // EDX, dw1
	pFile->PrintIndent("\"2\" (");
    if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n"); // EDI, 3
	pFile->PrintIndent("\"3\" (%s),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"4\" (%s)\n", (const char*)sTimeout); // ECX
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"esi\", \"memory\"\n");
	pFile->DecIndent();	
	pFile->PrintIndent(");\n");	
	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief checks if we should write assembler code for long IPCs
 *  \param pContext the context of this test
 *  \return true if assembler code should be written
 *
 * This implementation currently always returns true.
 */
bool CL4X0aBECallFunction::UseAsmLongIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    return true;
}

/** \brief writes assembler code for long IPC
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0aBECallFunction::WriteAsmLongIPC(CBEFile *pFile, CBEContext *pContext)
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
	if (bRecvShortIPC)
	    pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp \\n\\t\"\n");
	else
	{
	    pFile->PrintIndent("\"movl   %%%%edi,%%%%ebp      \\n\\t\"\n");
		pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi     \\n\\t\"\n");
		pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi     \\n\\t\"\n");
	}
	pFile->PrintIndent("ToId32_EdiEsi\n");
	if (bSendShortIPC)
	{
		pFile->PrintIndent("\"movl  (%%%%eax),%%%%edi     \\n\\t\"\n");
		pFile->PrintIndent("\"movl 8(%%%%eax),%%%%ebx     \\n\\t\"\n");
		pFile->PrintIndent("\"movl 4(%%%%eax),%%%%edx     \\n\\t\"\n");
		pFile->PrintIndent("\"subl  %%%%eax,%%%%eax      \\n\\t\"\n");
	}
	else
	{
		pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi     \\n\\t\"\n", nMsgBase);
		pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx     \\n\\t\"\n", nMsgBase+8);
		pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx     \\n\\t\"\n", nMsgBase+4);
	}
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
    if (bSendShortIPC)
	{
		pFile->Print("&(");
		m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
		pFile->Print("[0])");
	}
    else
    {
		if (m_pMsgBuffer->HasReference())
			pFile->Print("%s", (const char *) sMsgBuffer);
		else
			pFile->Print("&%s", (const char *) sMsgBuffer);
	}
	pFile->Print("),\n");
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
    if (bRecvShortIPC)
	{
    	pFile->PrintIndent("\"3\" (%s->lh.high),\n", (const char*)pObjName->GetName());
    	pFile->PrintIndent("\"4\" (%s->lh.low)\n", (const char*)pObjName->GetName());
	}
	else
    {
    	pFile->PrintIndent("\"3\" (((int)");
        if (m_pMsgBuffer->HasReference())
            pFile->Print("%s", (const char *) sMsgBuffer);
        else
            pFile->Print("&%s", (const char *) sMsgBuffer);
	    pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n");
    	pFile->PrintIndent("\"4\" (%s)\n", (const char*)pObjName->GetName());
    }
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
	if (bRecvShortIPC)
	    pFile->PrintIndent("\"subl  %%%%ebp,%%%%ebp \\n\\t\"\n"); // EDI, ESI are already in position
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
		pFile->PrintIndent("\"subl  %%%%eax,%%%%eax     \\n\\t\"\n");
	}
	else
	{
		pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edi     \\n\\t\"\n", nMsgBase);
		pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx     \\n\\t\"\n", nMsgBase+8);
		pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx     \\n\\t\"\n", nMsgBase+4);
	}
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
    if (bSendShortIPC)
	{
		pFile->PrintIndent("&(");
		m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
		pFile->Print("[0])");
	}
    else
    {
		if (m_pMsgBuffer->HasReference())
			pFile->Print("%s", (const char *) sMsgBuffer);
		else
			pFile->Print("&%s", (const char *) sMsgBuffer);
	}
	pFile->Print("),\n");
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
    if (bRecvShortIPC)
	{
		pFile->PrintIndent("\"3\" (%s->lh.high),\n", (const char*)pObjName->GetName());
		pFile->PrintIndent("\"4\" (%s->lh.low)\n", (const char*)pObjName->GetName());
	}
	else
    {
    	pFile->PrintIndent("\"3\" (((int)");
        if (m_pMsgBuffer->HasReference())
            pFile->Print("%s", (const char *) sMsgBuffer);
        else
            pFile->Print("&%s", (const char *) sMsgBuffer);
	    pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n");
    	pFile->PrintIndent("\"4\" (%s)\n", (const char*)pObjName->GetName());
    }
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"ecx\", \"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");
	
	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}
