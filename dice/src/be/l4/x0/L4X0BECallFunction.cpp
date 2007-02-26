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

#include "be/l4/x0/L4X0BECallFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEDeclarator.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEMarshaller.h"
#include "be/BEType.h"

#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CL4X0BECallFunction);

CL4X0BECallFunction::CL4X0BECallFunction()
 : CL4BECallFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0BECallFunction, CL4BECallFunction);
}


CL4X0BECallFunction::~CL4X0BECallFunction()
{
}

/** \brief test if we can use the assembler short IPC
 *  \param pContext the context of the test
 *  \return true if the assembler short IPC can be used
 */
bool CL4X0BECallFunction::UseAsmShortIPC(CBEContext *pContext)
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
void CL4X0BECallFunction::WriteIPC(CBEFile * pFile,  CBEContext * pContext)
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
void CL4X0BECallFunction::WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext)
{
    if (UseAsmShortIPC(pContext))
	{
		// write dummys
		CBENameFactory *pNF = pContext->GetNameFactory();
		String sDummy1 = pNF->GetDummyVariable(pContext);
		String sDummy2 = sDummy1;
		String sDummy3 = sDummy1;
		String sDummy4 = sDummy1;
		sDummy1 += "0";
		sDummy2 += "1";
		sDummy3 += "2";
		sDummy4 += "3";
		String sMWord = pNF->GetTypeName(TYPE_MWORD, false, pContext, 0);
		String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
		
		// test if we need dummies
		pFile->Print("#if defined(__PIC__)\n");
		WriteReturnVariableDeclaration(pFile, pContext);
		// write result variable
		pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", (const char *) sResult);
		pFile->PrintIndent("%s %s __attribute__((unused));\n", 
		                    (const char*)sMWord, (const char*)sDummy2);
		pFile->PrintIndent("%s %s __attribute__((unused));\n", 
		                    (const char*)sMWord, (const char*)sDummy3);
		pFile->PrintIndent("%s %s __attribute__((unused));\n", 
		                    (const char*)sMWord, (const char*)sDummy4);
		
		pFile->Print("#else // !PIC\n");
		pFile->Print("#if !defined(PROFILE)\n");
		WriteReturnVariableDeclaration(pFile, pContext);
		// write result variable
		pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", (const char *) sResult);
		pFile->PrintIndent("%s %s = 0;\n", (const char *)sMWord, (const char*)sDummy1);
		pFile->PrintIndent("%s %s __attribute__((unused));\n", 
		                    (const char*)sMWord, (const char*)sDummy2);
		pFile->PrintIndent("%s %s __attribute__((unused));\n", 
		                    (const char*)sMWord, (const char*)sDummy3);
		pFile->PrintIndent("%s %s __attribute__((unused));\n", 
		                    (const char*)sMWord, (const char*)sDummy4);
		pFile->Print("#endif // PROFILE\n");
		pFile->Print("#endif // !PIC\n");
		// if we have in either direction some bit-stuffing, we need more dummies
	    // finished with declaration
	}
	else
	    CL4BECallFunction::WriteVariableDeclaration(pFile, pContext);
}

/** \brief writes the variable initialization
 *  \param pFile the file to write to
 *  \param pContext the context of teh write operation
 *
 * If we use the asm short IPC code we do not init any variables (Its done during declaration).
 */
void CL4X0BECallFunction::WriteVariableInitialization(CBEFile * pFile,  CBEContext * pContext)
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
void CL4X0BECallFunction::WriteMarshalling(CBEFile * pFile,  int nStartOffset,   
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
void CL4X0BECallFunction::WriteUnmarshalling(CBEFile * pFile,  int nStartOffset, 
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
void CL4X0BECallFunction::WriteInvocation(CBEFile * pFile,  CBEContext * pContext)
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
void CL4X0BECallFunction::WriteAsmShortIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sDummy1 = pNF->GetDummyVariable(pContext);
	String sDummy2 = sDummy1;
	String sDummy3 = sDummy1;
	String sDummy4 = sDummy1;
	sDummy1 += "0";
	sDummy2 += "1";
	sDummy3 += "2";
	sDummy4 += "3";

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
	pFile->PrintIndent("\"movl   %%%%eax,%%%%ebx    \\n\\t\"\n");
	pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
	pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp    \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point		
	pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx    \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebx            \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),                /* EAX, 0 */\n", (const char*)sResult);
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
	pFile->PrintIndent("\"0\" (");
    if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),                 /* EAX, 0 => EBX */\n");
	pFile->PrintIndent("\"1\" (%s),                 /* EDX, 1 */\n", (const char*)m_sOpcodeConstName);
	pFile->PrintIndent("\"2\" (%s),                 /* ECX, 2 */\n", (const char*)sTimeout);
	pFile->PrintIndent("\"S\" (%s->raw),             /* ESI    */\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"3\" (");
    if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print(")                  /* EDI    */\n");
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
	pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
	pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp    \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point		
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),                /* EAX, 0 */\n", (const char*)sResult);
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
	pFile->Print("),                /* EBX, 2 */\n");
	pFile->PrintIndent("\"=c\" (%s),                /* ECX, 3 */\n", (const char*)sDummy1);
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
	pFile->Print(")                 /* EDI, 4 */\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"1\" (%s),                 /* EDX, 1 */\n", (const char*)m_sOpcodeConstName);
	pFile->PrintIndent("\"2\" (");
    if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),                 /* EBX, 2 */\n");
	pFile->PrintIndent("\"3\" (%s),                 /* ECX, 3 */\n", (const char*)sTimeout);
	pFile->PrintIndent("\"S\" (%s->raw),             /* ESI    */\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"4\" (");
    if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print(")                  /* EDI, 4 */\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
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
bool CL4X0BECallFunction::UseAsmLongIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    return true;
}

/** \brief writes assembler code for long IPC
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0BECallFunction::WriteAsmLongIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);
	String sDummy1 = pNF->GetDummyVariable(pContext);
	sDummy1 += "0";
	
	pFile->Print("#if defined(__PIC__)\n");
	// PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl  %%%%ebx            \\n\\t\"\n");		
	pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n");
	pFile->PrintIndent("\"movl   %%%%edi,%%%%ebp      \\n\\t\"\n");
	pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi     \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx     \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx     \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx      \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebx            \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n");
	pFile->PrintIndent("\"=S\" (%s)\n", (const char*)sDummy1);
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
	pFile->PrintIndent("\"4\" (%s->raw)\n", (const char*)pObjName->GetName());
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
	pFile->PrintIndent("\"movl   %%%%ebx,%%%%ebp      \\n\\t\"\n");
	pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi     \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx     \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx     \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point		
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n");
	pFile->PrintIndent("\"=c\" (%s),\n", (const char*)sDummy1);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"S\" (%s->raw),\n", (const char*)pObjName->GetName());
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
	    pFile->Print(") & (~L4_IPC_OPEN_IPC)");
    }
	pFile->Print("),\n");
	pFile->PrintIndent("\"4\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");
	
	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}
