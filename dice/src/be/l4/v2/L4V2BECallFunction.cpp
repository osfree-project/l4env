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
 
#include "be/l4/v2/L4V2BECallFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "be/BEType.h"
#include "be/BEMarshaller.h"

#include "fe/FETypeSpec.h"

IMPLEMENT_DYNAMIC(CL4V2BECallFunction)

CL4V2BECallFunction::CL4V2BECallFunction()
 : CL4BECallFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4V2BECallFunction, CL4BECallFunction);
}


CL4V2BECallFunction::~CL4V2BECallFunction()
{
}

/** \brief writes the invocation code for the short IPC
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * If we have a short IPC in both direction, we write assembler code
 * directly.
 *
 * We have three branches: PIC, !PIC && PROFILE, and else. We always assume 
 * gcc versions above 2.95 -> no 2.7 support. And we ignore the BIGASM support.
 */
void CL4V2BECallFunction::WriteInvocation(CBEFile * pFile,  CBEContext * pContext)
{
    if (UseAsmShortIPC(pContext))
    {
	    pFile->Print("#if !defined(__PIC__) && defined(PROFILE)\n");
	    ((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteSendDopeInit(pFile, DIRECTION_IN, pContext);
        pFile->Print("#endif\n");
	    // skip send dope init
	    WriteIPC(pFile, pContext);
		WriteIPCErrorCheck(pFile, pContext);
    }   
	else       
        CL4BECallFunction::WriteInvocation(pFile, pContext);
}

/* \brief write the marshalling code for the short IPC    
 * \param pFile the file to write to    
 * \param nStartOffset the offset where to start marshalling    
 * \param bUseConstOffset true if nStartOffset can be used    
 * \param pContext the context of the write operation    
 * 
 * If we have a short IPC into both direction, we skip the marshalling.  
 */
void CL4V2BECallFunction::WriteMarshalling(CBEFile * pFile,  int nStartOffset,   
                           bool & bUseConstOffset,  CBEContext * pContext)
{      
    if (UseAsmShortIPC(pContext))
	{
	    // if short IPC consist of more than one parameter (bit-stuffing), then we
		// have to stuff now               
		// if !PIC && PROFILE we use "normal" IPC
		pFile->Print("#if !defined(__PIC__) && defined(PROFILE)\n");
        CL4BECallFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
		pFile->Print("#endif\n");
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
void CL4V2BECallFunction::WriteUnmarshalling(CBEFile * pFile,  int nStartOffset, 
                            bool & bUseConstOffset,  CBEContext * pContext)
{   
    if (UseAsmShortIPC(pContext))       
	{
	    // if short IPC consists of more than two parameters (bit-stuffing), then we
		// have to "unstuff" now
		// if !PIC && PROFILE we use "normal" IPC
		pFile->Print("#if !defined(__PIC__) && defined(PROFILE)\n");
		CL4BECallFunction::WriteUnmarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
		pFile->Print("#endif\n");
	}
	else
        CL4BECallFunction::WriteUnmarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
}

/* \brief test if we can use the assembler code   
 * \param pContext the context of the test   
 * 
 * This function returns true if we have a short IPC in both directions. 
 */
bool CL4V2BECallFunction::UseAsmShortIPC(CBEContext* pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    // test optimization level
	if (pContext->GetOptimizeLevel() < 2)
	    return false;
    CL4BEMsgBufferType *pMsgBuf = (CL4BEMsgBufferType*)m_pMsgBuffer;
    return (pMsgBuf->IsShortIPC(GetSendDirection(), pContext)) && 
           (pMsgBuf->IsShortIPC(GetReceiveDirection(), pContext));
}

/** \brief Writes the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * If we have a short IPC in both direction, we only need the result dope,
 * and two dummy dwords.
 */
void CL4V2BECallFunction::WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext)
{
    if (UseAsmShortIPC(pContext))
	{
		// write dummys
		CBENameFactory *pNF = pContext->GetNameFactory();
		String sDummy = pNF->GetDummyVariable(pContext);
		String sMWord = pNF->GetTypeName(TYPE_MWORD, false, pContext, 0);
		String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
		
		// test if we need dummies
		pFile->Print("#if defined(__PIC__)\n");
		WriteReturnVariableDeclaration(pFile, pContext);
		// write result variable
		pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", (const char *) sResult);
		pFile->PrintIndent("%s %s __attribute__((unused));\n", 
							(const char*)sMWord, (const char*)sDummy);
		
		pFile->Print("#else // !PIC\n");
		pFile->Print("#if defined(PROFILE)\n");
		CL4BECallFunction::WriteVariableDeclaration(pFile, pContext);
		
		pFile->Print("#else // !PROFILE\n");
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
	    if (UseAsmLongIPC(pContext))
		{
		    // need dummies
			CBENameFactory *pNF = pContext->GetNameFactory();
			String sDummy = pNF->GetDummyVariable(pContext);
			String sMWord = pNF->GetTypeName(TYPE_MWORD, false, pContext, 0);
    		pFile->Print("#if defined(__PIC__)\n");
	    	pFile->PrintIndent("%s %s;\n", (const char *)sMWord, (const char*)sDummy);
			pFile->Print("#else // !PIC\n");
			pFile->Print("#if !defined(PROFILE)\n");
    		pFile->PrintIndent("%s %s;\n", (const char *)sMWord, (const char*)sDummy);
			pFile->Print("#endif // !PROFILE\n");
			pFile->Print("#endif // !PIC\n");
		}
	}
}

void CL4V2BECallFunction::WriteIPC(CBEFile * pFile,  CBEContext * pContext)
{
    if (UseAsmShortIPC(pContext))
	    WriteAsmShortIPC(pFile, pContext);
	else if (UseAsmLongIPC(pContext))
	    WriteAsmLongIPC(pFile, pContext);
	else
	    CL4BECallFunction::WriteIPC(pFile, pContext);
}

/** \brief writes the variable initialization
 *  \param pFile the file to write to
 *  \param pContext the context of teh write operation
 *
 * If we use the asm short IPC code we do not init any variables (Its done during declaration).
 */
void CL4V2BECallFunction::WriteVariableInitialization(CBEFile * pFile,  CBEContext * pContext)
{
    if (UseAsmShortIPC(pContext))
	{
	    pFile->Print("#if !defined(__PIC__) && defined(PROFILE)\n");
	    CL4BECallFunction::WriteVariableInitialization(pFile, pContext);
		pFile->Print("#endif // !PIC && PROFILE\n");
	}
	else
	    CL4BECallFunction::WriteVariableInitialization(pFile, pContext);
}

/** \brief write the assembler version of the short IPC
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This is only called if UseAsmShortIPCShortIPC == true, which means that we have a short IPC
 * in both direction. This allows us some optimizations in the assembler code.
 */
void CL4V2BECallFunction::WriteAsmShortIPC(CBEFile *pFile, CBEContext *pContext)
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
	pFile->Print("#ifdef __PIC__\n");
	// PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl  %%%%ebx          \\n\\t\"\n");		
	pFile->PrintIndent("\"pushl  %%%%ebp          \\n\\t\"\n"); // save ebp no memory references ("m") after this point
	pFile->PrintIndent("\"movl   %%%%eax,%%%%ebx    \\n\\t\"\n");
	pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
	pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp    \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp          \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point		
	pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx    \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebx          \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);                /* EAX, 0 */
	pFile->PrintIndent("\"=d\" (");
	if (pRetVar)
	    pFile->Print("%s", (const char*)pRetVar->GetName());
	else
	{
	    if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetReceiveDirection(), true, pContext))
		    pFile->Print("%s", (const char*)sDummy);
	}
	pFile->Print("),\n");                /* EDX, 1 */
	pFile->PrintIndent("\"=c\" (");
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
	pFile->Print(")\n");                /* ECX, 2 */
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
	// get send parameter
	if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n");                 /* EAX, 0 => EBX */
	pFile->PrintIndent("\"1\" (%s),\n", (const char*)m_sOpcodeConstName);                 /* EDX, 1 */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);                 /* ECX, 2 */
	pFile->PrintIndent("\"S\" (%s->lh.low),\n", (const char*)pObjName->GetName());          /* ESI    */
	pFile->PrintIndent("\"D\" (%s->lh.high)\n", (const char*)pObjName->GetName());          /* EDI    */
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");
	pFile->Print("#else // !__PIC__\n");
	pFile->Print("#ifdef PROFILE\n");
	// !PIC && PROFILE branch	
	// uses ipc_i386_call_static (l4/sys/lib/src/ipc-profile.c)	
	CL4BECallFunction::WriteIPC(pFile, pContext);
	pFile->Print("#else // !PROFILE\n");
	// else	
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl  %%%%ebp          \\n\\t\"\n"); // save ebp no memory references ("m") after this point
	pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
	pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp    \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp          \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point		
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);                /* EAX, 0 */
	pFile->PrintIndent("\"=d\" (");
	if (pRetVar)
	    pFile->Print("%s", (const char*)pRetVar->GetName());
	else
	{
	    if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetReceiveDirection(), true, pContext))
			pFile->Print("%s", (const char*)sDummy);
	}
	pFile->Print("),\n");                /* EDX, 1 */
	pFile->PrintIndent("\"=b\" (");
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
	pFile->Print("),\n");                /* EBX, 2 */
	pFile->PrintIndent("\"=c\" (%s)\n", (const char*)sDummy);                /* ECX, 3 */
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"1\" (%s),\n", (const char*)m_sOpcodeConstName);                 /* EDX, 1 */
	pFile->PrintIndent("\"2\" (");
    if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetSendDirection(), false, pContext))
		pFile->Print("0");
	pFile->Print("),\n");                 /* EBX, 2 */
	pFile->PrintIndent("\"3\" (%s),\n", (const char*)sTimeout);                 /* ECX, 3 */
	pFile->PrintIndent("\"S\" (%s->lh.low),\n", (const char*)pObjName->GetName());          /* ESI    */
	pFile->PrintIndent("\"D\" (%s->lh.high)\n", (const char*)pObjName->GetName());          /* EDI    */
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();	
	pFile->PrintIndent(");\n");	
	pFile->Print("#endif // PROFILE\n");	
	pFile->Print("#endif // __PIC__\n");   
}

/** \brief test if we can write an assembler long IPC
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \return true if we can write a assembler version of the long IPC
 *
 * This implementation currently always returns true, since we have no option
 * which proibits the use of assembler.
 */
bool CL4V2BECallFunction::UseAsmLongIPC(CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
    // test optimization level
	if (pContext->GetOptimizeLevel() < 2)
	    return false;
	// test for constructed types
/*	VectorElement *pIter = GetFirstParameter();
	CBETypedDeclarator *pParameter;
	while ((pParameter = GetNextParameter(pIter)) != 0)
	{
	    if (pParameter->GetType()->IsConstructedType())
		    return false;
	}*/
    return true;
}

/** \brief write the long IPC in assembler
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4V2BECallFunction::WriteAsmLongIPC(CBEFile *pFile, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	String sDummy = pNF->GetDummyVariable(pContext);
	
	// l4_fpage_t + 2*l4_msgdope_t
	int nMsgBase = pContext->GetSizes()->GetSizeOfEnvType("l4_fpage_t") +
				pContext->GetSizes()->GetSizeOfEnvType("l4_msgdope_t")*2;
    
	VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
	CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);
	
	bool bSendShortIPC = ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetSendDirection(), pContext);
	bool bRecvShortIPC = ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetReceiveDirection(), pContext);

	pFile->Print("#ifdef __PIC__\n");
	// PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl  %%%%ebx         \\n\\t\"\n");
	pFile->PrintIndent("\"pushl  %%%%ebp         \\n\\t\"\n");
	if (bSendShortIPC)
	{
		pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx  \\n\\t\"\n");
		pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx  \\n\\t\"\n");
		pFile->PrintIndent("\"subl   %%%%eax,%%%%eax   \\n\\t\"\n"); // snd msg descr = 0
	}
	else
	{
	    // if long ipc we can extract the dwords directly from the msg buffer structure in EAX
		pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+4);
		pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase);
	}
    if (bRecvShortIPC)
	    pFile->PrintIndent("\"subl   %%%%ebp, %%%%ebp  \\n\\t\"\n"); // receive short IPC
	else
	    pFile->PrintIndent("\"movl   %%%%edi, %%%%ebp  \\n\\t\"\n");
    pFile->PrintIndent("\"movl 4(%%%%esi),%%%%edi  \\n\\t\"\n");
    pFile->PrintIndent("\"movl  (%%%%esi),%%%%esi  \\n\\t\"\n");
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl  %%%%ebp          \\n\\t\"\n");
    pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx    \\n\\t\"\n");
    pFile->PrintIndent("\"popl  %%%%ebx          \\n\\t\"\n");
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
    pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])))),\n");
    pFile->PrintIndent("\"=S\" (%s),\n", (const char*)sDummy);
    pFile->PrintIndent("\"=D\" (%s)\n", (const char*)sDummy);
    pFile->PrintIndent(":\n");
	// skip this if short IPC
    if (!bSendShortIPC)
    {
        pFile->PrintIndent("\"0\" (");
		if (m_pMsgBuffer->HasReference())
			pFile->Print("%s", (const char *) sMsgBuffer);
		else
			pFile->Print("&%s", (const char *) sMsgBuffer);
    	pFile->Print("),\n");
	}
	else
	{
		// -> if short IPC we have to set send dwords
		pFile->PrintIndent("\"1\" (&(");
		m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
		pFile->Print("[0])),\n");
	}
    pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
	// only if not short IPC we have to hand this to the assembler code
    if (!bRecvShortIPC)
    {
        pFile->PrintIndent("\"4\" (((int)");
        if (m_pMsgBuffer->HasReference())
            pFile->Print("%s", (const char *) sMsgBuffer);
        else
            pFile->Print("&%s", (const char *) sMsgBuffer);
	    pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n");
    }
    pFile->PrintIndent("\"3\" (%s)\n", (const char*)pObjName->GetName());
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();
    pFile->PrintIndent(");\n");
	
	pFile->Print("#else // !__PIC__\n");
	pFile->Print("#ifdef PROFILE\n");
	// !PIC && PROFILE branch	
	// uses ipc_i386_call_static (l4/sys/lib/src/ipc-profile.c)	
	CL4BECallFunction::WriteIPC(pFile, pContext);
	pFile->Print("#else // !PROFILE\n");
	// else	
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
    pFile->PrintIndent("\"pushl %%%%ebp          \\n\\t\"\n");   /* save ebp, no memory references ("m") after this point */
	if (bRecvShortIPC)
	    pFile->PrintIndent("\"subl  %%%%ebp,%%%%ebp    \\n\\t\"\n"); /* recv msg descriptor = 0 */
	else
		pFile->PrintIndent("\"movl  %%%%ebx, %%%%ebp   \\n\\t\"\n");
	if (bSendShortIPC)
	{
		pFile->PrintIndent("\"movl 4(%%%%edx), %%%%ebx \\n\\t\"\n");   /* dest.lh.high -> edi */
		pFile->PrintIndent("\"movl  (%%%%edx), %%%%edx \\n\\t\"\n");   /* dest.lh.low  -> esi */
	    pFile->PrintIndent("\"subl   %%%%eax,%%%%eax \\n\\t\"\n"); /* send msg descriptor = 0 */
	}
	else
	{
	    // extract dwords directly from msg buffer
		pFile->PrintIndent("\"movl %d(%%%%eax), %%%%ebx \\n\\t\"\n", nMsgBase+4);   /* dest.lh.high -> edi */
		pFile->PrintIndent("\"movl %d(%%%%eax), %%%%edx \\n\\t\"\n", nMsgBase);   /* dest.lh.low  -> esi */
	}
    pFile->PrintIndent("IPC_SYSENTER\n");
    pFile->PrintIndent("\"popl  %%%%ebp          \\n\\t\"\n");   /* restore ebp, no memory references ("m") before this point */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);               /* EAX, 0 */
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
	m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])))),\n");           /* EDX, 1 */
    pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
	m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");           /* EBX, 2 */
    pFile->PrintIndent("\"=c\" (%s)\n", (const char*)sDummy);                 /* EDI, 3 */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"S\" (%s->lh.low),\n", (const char*)pObjName->GetName());                   /* dest, 4  */
    pFile->PrintIndent("\"D\" (%s->lh.high),\n", (const char*)pObjName->GetName());                   /* dest, 4  */
    if (!bSendShortIPC)
    {
		pFile->PrintIndent("\"0\" (");
        if (m_pMsgBuffer->HasReference())
            pFile->Print("%s", (const char *) sMsgBuffer);
        else
            pFile->Print("&%s", (const char *) sMsgBuffer);
    	pFile->Print("),\n");           /* EAX, 0 */
    }
	else
	{
		pFile->PrintIndent("\"1\" (&(", (const char*)sMWord);
		m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
		pFile->Print("[0])),\n");             /* EDX, 1 */
	}
    if (!bRecvShortIPC)
    {
        pFile->PrintIndent("\"2\" (((int)");
        if (m_pMsgBuffer->HasReference())
            pFile->Print("%s", (const char *) sMsgBuffer);
        else
            pFile->Print("&%s", (const char *) sMsgBuffer);
	    pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n"); /* EDI, 3 rcv msg -> ebp */
    }
    pFile->PrintIndent("\"3\" (%s)\n", (const char*)sTimeout);                /* timeout, 5 */
    pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();
    pFile->PrintIndent(");\n");
	
	pFile->Print("#endif // PROFILE\n");	
	pFile->Print("#endif // __PIC__\n");   
}
