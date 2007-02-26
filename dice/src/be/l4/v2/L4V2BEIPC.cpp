/**
 *	\file	dice/src/be/l4/v2/L4V2BEIPC.cpp
 *	\brief	contains the declaration of the class CL4V2BEIPC
 *
 *	\date	08/13/2003
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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

#include "be/l4/v2/L4V2BEIPC.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEContext.h"
#include "be/BEFunction.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEMarshaller.h"
#include "TypeSpec-Type.h"

IMPLEMENT_DYNAMIC(CL4V2BEIPC)

CL4V2BEIPC::CL4V2BEIPC()
 : CL4BEIPC()
{
    IMPLEMENT_DYNAMIC_BASE(CL4V2BEIPC, CL4BEIPC);
}

/** destructor for IPC class */
CL4V2BEIPC::~CL4V2BEIPC()
{
}

/** \brief write L4 V2 specific call code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4V2BEIPC::WriteCall(CBEFile * pFile,  CBEFunction * pFunction,  CBEContext * pContext)
{
    if (UseAssembler(pFunction, pContext))
	{
	    if (IsShortIPC(pFunction, pContext))
		    WriteAsmShortCall(pFile, pFunction, pContext);
	    else
		    WriteAsmLongCall(pFile, pFunction, pContext);
	}
	else
	    CL4BEIPC::WriteCall(pFile, pFunction, pContext);
}

/** \brief write the assembler version of the short IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write the call for
 *  \param pContext the context of the write operation
 *
 * This is only called if UseAsmShortIPCShortIPC == true, which means that we have a short IPC
 * in both direction. This allows us some optimizations in the assembler code.
 */
void CL4V2BEIPC::WriteAsmShortCall(CBEFile *pFile, CBEFunction *pFunction, CBEContext *pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sDummy = pNF->GetDummyVariable(pContext);

	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
	int nRcvDir = pFunction->GetReceiveDirection();
	int nSndDir = pFunction->GetSendDirection();

	// to increase the confusing code:
	// if we have PROGRAM_USE_SYMBOLS set, we test for __PIC__ and PROFILE
	// then we write the three parts bPIC, bPROF, bNPROF if they are set
	bool bPIC = true;
	bool bPROF = true;
	bool bNPROF = true;
	bool bSymbols = pContext->IsOptionSet(PROGRAM_USE_SYMBOLS);
	bool bSendFlexpage = ((CL4BEMsgBufferType*)pFunction->GetMessageBuffer())->GetFlexpageCount(nSndDir) > 0;
	if (bSymbols)
	{
	    bPIC = pContext->HasSymbol("__PIC__");
		bPROF = pContext->HasSymbol("PROFILE") && !bPIC;
		bNPROF = !bPROF && !bPIC;
	}

	if (!bSymbols)
	    pFile->Print("#ifdef __PIC__\n");
	if (bPIC)
	{
		// PIC branch
		pFile->PrintIndent("asm volatile(\n");
		pFile->IncIndent();
		pFile->PrintIndent("\"pushl  %%%%ebx          \\n\\t\"\n");
		pFile->PrintIndent("\"pushl  %%%%ebp          \\n\\t\"\n"); // save ebp no memory references ("m") after this point
		pFile->PrintIndent("\"movl   %%%%eax,%%%%ebx    \\n\\t\"\n");
		if (bSendFlexpage)
		    pFile->PrintIndent("\"movl $2,%%%%eax       \\n\\t\"\n");
		else
			pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
		pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp    \\n\\t\"\n");
		pFile->PrintIndent("IPC_SYSENTER\n");
		pFile->PrintIndent("\"popl   %%%%ebp          \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point
		pFile->PrintIndent("\"movl   %%%%ebx,%%%%ecx    \\n\\t\"\n");
		pFile->PrintIndent("\"popl   %%%%ebx          \\n\\t\"\n");
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);                /* EAX, 0 */
		pFile->PrintIndent("\"=d\" (");
		if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRcvDir, true, pContext))
			pFile->Print("%s", (const char*)sDummy);
		pFile->Print("),\n");                /* EDX, 1 */
		pFile->PrintIndent("\"=c\" (");
		if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRcvDir, true, pContext))
			pFile->Print("%s", (const char*)sDummy);
		pFile->Print(")\n");                /* ECX, 2 */
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"0\" (");
		// get send parameter
		if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSndDir, false, pContext))
			pFile->Print("0");
		pFile->Print("),\n");                 /* EAX, 0 => EBX */
		pFile->PrintIndent("\"1\" (%s),\n", (const char*)pFunction->GetOpcodeConstName());                 /* EDX, 1 */
		pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);                 /* ECX, 2 */
		pFile->PrintIndent("\"S\" (%s->lh.low),\n", (const char*)pObjName->GetName());          /* ESI    */
		pFile->PrintIndent("\"D\" (%s->lh.high)\n", (const char*)pObjName->GetName());          /* EDI    */
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"memory\"\n");
		pFile->DecIndent();
		pFile->PrintIndent(");\n");
	} // PIC
	if (!bSymbols)
	{
		pFile->Print("#else // !__PIC__\n");
		pFile->Print("#ifdef PROFILE\n");
	}
	if (bPROF) // is !__PIC__ && PROFILE
	{
		// !PIC && PROFILE branch
		// uses ipc_i386_call_static (l4/sys/lib/src/ipc-profile.c)
		CL4BEIPC::WriteCall(pFile, pFunction, pContext);
	}
	if (!bSymbols)
	    pFile->Print("#else // !PROFILE\n");
	if (bNPROF) // is !__PIC__ && !PROFILE
	{
		// else
		pFile->PrintIndent("asm volatile(\n");
		pFile->IncIndent();
		pFile->PrintIndent("\"pushl  %%%%ebp          \\n\\t\"\n"); // save ebp no memory references ("m") after this point
		if (bSendFlexpage)
		    pFile->PrintIndent("\"movl $2,%%%%eax       \\n\\t\"\n");
		else
			pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
		pFile->PrintIndent("\"subl   %%%%ebp,%%%%ebp    \\n\\t\"\n");
		pFile->PrintIndent("IPC_SYSENTER\n");
		pFile->PrintIndent("\"popl   %%%%ebp          \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);                /* EAX, 0 */
		pFile->PrintIndent("\"=d\" (");
		if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRcvDir, true, pContext))
			pFile->Print("%s", (const char*)sDummy);
		pFile->Print("),\n");                /* EDX, 1 */
		pFile->PrintIndent("\"=b\" (");
		if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRcvDir, true, pContext))
			pFile->Print("%s", (const char*)sDummy);
		pFile->Print("),\n");                /* EBX, 2 */
		pFile->PrintIndent("\"=c\" (%s)\n", (const char*)sDummy);                /* ECX, 3 */
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"1\" (%s),\n", (const char*)pFunction->GetOpcodeConstName());                 /* EDX, 1 */
		pFile->PrintIndent("\"2\" (");
		if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSndDir, false, pContext))
			pFile->Print("0");
		pFile->Print("),\n");                 /* EBX, 2 */
		pFile->PrintIndent("\"3\" (%s),\n", (const char*)sTimeout);                 /* ECX, 3 */
		pFile->PrintIndent("\"S\" (%s->lh.low),\n", (const char*)pObjName->GetName());          /* ESI    */
		pFile->PrintIndent("\"D\" (%s->lh.high)\n", (const char*)pObjName->GetName());          /* EDI    */
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"memory\"\n");
		pFile->DecIndent();
		pFile->PrintIndent(");\n");
	}
	if (!bSymbols)
	{
		pFile->Print("#endif // PROFILE\n");
		pFile->Print("#endif // __PIC__\n");
	}
}

/** \brief write the long IPC in assembler
 *  \param pFile the file to write to
 *  \param pFunction the function to write the IPC for
 *  \param pContext the context of the write operation
 */
void CL4V2BEIPC::WriteAsmLongCall(CBEFile *pFile, CBEFunction *pFunction, CBEContext *pContext)
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

	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	int nSndDir = pFunction->GetSendDirection();
	int nRcvDir = pFunction->GetReceiveDirection();

	bool bSendShortIPC = ((CL4BEMsgBufferType*)pFunction->GetMessageBuffer())->IsShortIPC(nSndDir, pContext);
	bool bRecvShortIPC = ((CL4BEMsgBufferType*)pFunction->GetMessageBuffer())->IsShortIPC(nRcvDir, pContext);
	bool bSendFlexpage = ((CL4BEMsgBufferType*)pFunction->GetMessageBuffer())->GetFlexpageCount(nSndDir) > 0;

	// to increase the confusing code:
	// if we have PROGRAM_USE_SYMBOLS set, we test for __PIC__ and PROFILE
	// then we write the three parts bPIC, bPROF, bNPROF if they are set
	bool bPIC = true;
	bool bPROF = true;
	bool bNPROF = true;
	bool bSymbols = pContext->IsOptionSet(PROGRAM_USE_SYMBOLS);
	if (bSymbols)
	{
	    bPIC = pContext->HasSymbol("__PIC__");
		bPROF = pContext->HasSymbol("PROFILE") && !bPIC;
		bNPROF = !bPROF && !bPIC;
	}

	if (!bSymbols)
	    pFile->Print("#ifdef __PIC__\n");
	if (bPIC)
	{
		// PIC branch
		pFile->PrintIndent("asm volatile(\n");
		pFile->IncIndent();
		pFile->PrintIndent("\"pushl  %%%%ebx         \\n\\t\"\n");
		pFile->PrintIndent("\"pushl  %%%%ebp         \\n\\t\"\n");
		if (bSendShortIPC)
		{
			pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx  \\n\\t\"\n");
			pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx  \\n\\t\"\n");
			if (bSendFlexpage)
			    pFile->PrintIndent("\"movl   $2,%%%%eax     \\n\\t\"\n");
			else
				pFile->PrintIndent("\"subl   %%%%eax,%%%%eax   \\n\\t\"\n"); // snd msg descr = 0
		}
		else
		{
			// if long ipc we can extract the dwords directly from the msg buffer structure in EAX
			pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+4);
			pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase);
			if (bSendFlexpage)
			    pFile->PrintIndent("\"orl $2,%%%%eax    \\n\\t\"\n");
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
		pFunction->GetMessageBuffer()->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
		pFile->Print("[0])))),\n");
		pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
		pFunction->GetMessageBuffer()->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
		pFile->Print("[4])))),\n");
		pFile->PrintIndent("\"=S\" (%s),\n", (const char*)sDummy);
		pFile->PrintIndent("\"=D\" (%s)\n", (const char*)sDummy);
		pFile->PrintIndent(":\n");
		// skip this if short IPC
		if (!bSendShortIPC)
		{
			pFile->PrintIndent("\"0\" (");
			if (pFunction->GetMessageBuffer()->HasReference())
				pFile->Print("%s", (const char *) sMsgBuffer);
			else
				pFile->Print("&%s", (const char *) sMsgBuffer);
			pFile->Print("),\n");
		}
		else
		{
			// -> if short IPC we have to set send dwords
			pFile->PrintIndent("\"1\" (&(");
			pFunction->GetMessageBuffer()->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
			pFile->Print("[0])),\n");
		}
		pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
		// only if not short IPC we have to hand this to the assembler code
		if (!bRecvShortIPC)
		{
			pFile->PrintIndent("\"4\" (((int)");
			if (pFunction->GetMessageBuffer()->HasReference())
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
    } // PIC
	if (!bSymbols)
	{
		pFile->Print("#else // !__PIC__\n");
		pFile->Print("#ifdef PROFILE\n");
	}
	if (bPROF)
	{
		// !PIC && PROFILE branch
		// uses ipc_i386_call_static (l4/sys/lib/src/ipc-profile.c)
		CL4BEIPC::WriteCall(pFile, pFunction, pContext);
	}
	if (!bSymbols)
	    pFile->Print("#else // !PROFILE\n");
	if (bNPROF)
	{
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
			if (bSendFlexpage)
			    pFile->PrintIndent("\"movl $2,%%%%eax \\n\\t\"\n");
			else
				pFile->PrintIndent("\"subl   %%%%eax,%%%%eax \\n\\t\"\n"); /* send msg descriptor = 0 */
		}
		else
		{
			// extract dwords directly from msg buffer
			pFile->PrintIndent("\"movl %d(%%%%eax), %%%%ebx \\n\\t\"\n", nMsgBase+4);   /* dest.lh.high -> edi */
			pFile->PrintIndent("\"movl %d(%%%%eax), %%%%edx \\n\\t\"\n", nMsgBase);   /* dest.lh.low  -> esi */
			if (bSendFlexpage)
			    pFile->PrintIndent("\"orl $2, %%%%eax \\n\\t\"\n");
		}
		pFile->PrintIndent("IPC_SYSENTER\n");
		pFile->PrintIndent("\"popl  %%%%ebp          \\n\\t\"\n");   /* restore ebp, no memory references ("m") before this point */
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);               /* EAX, 0 */
		pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
		pFunction->GetMessageBuffer()->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
		pFile->Print("[0])))),\n");           /* EDX, 1 */
		pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
		pFunction->GetMessageBuffer()->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
		pFile->Print("[4])))),\n");           /* EBX, 2 */
		pFile->PrintIndent("\"=c\" (%s)\n", (const char*)sDummy);                 /* EDI, 3 */
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"S\" (%s->lh.low),\n", (const char*)pObjName->GetName());                   /* dest, 4  */
		pFile->PrintIndent("\"D\" (%s->lh.high),\n", (const char*)pObjName->GetName());                   /* dest, 4  */
		if (!bSendShortIPC)
		{
			pFile->PrintIndent("\"0\" (");
			if (pFunction->GetMessageBuffer()->HasReference())
				pFile->Print("%s", (const char *) sMsgBuffer);
			else
				pFile->Print("&%s", (const char *) sMsgBuffer);
			pFile->Print("),\n");           /* EAX, 0 */
		}
		else
		{
			pFile->PrintIndent("\"1\" (&(", (const char*)sMWord);
			pFunction->GetMessageBuffer()->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
			pFile->Print("[0])),\n");             /* EDX, 1 */
		}
		if (!bRecvShortIPC)
		{
			pFile->PrintIndent("\"2\" (((int)");
			if (pFunction->GetMessageBuffer()->HasReference())
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
    }
	if (!bSymbols)
	{
		pFile->Print("#endif // PROFILE\n");
		pFile->Print("#endif // __PIC__\n");
	}
}

/** \brief test if we could write assembler code for the IPC
 *  \param pFunction the function to write the IPC for
 *  \param pContext the context of the write operation
 *  \return true if assembler should be written
 *
 * We may write assembler if the optimization level is larger than 1 and
 * if we are not forced to write C bindings.
 */
bool CL4V2BEIPC::UseAssembler(CBEFunction* pFunction,  CBEContext* pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
	if (pContext->GetOptimizeLevel() < 2)
	    return false;
	// test position size
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
	bool bReturn = pMarshaller->TestPositionSize(pFunction, 4, pFunction->GetReceiveDirection(), false, false, 2/* must fit 2 registers */, pContext);
	delete pMarshaller;
	// return
	return bReturn;
}

/** \brief writes the send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4V2BEIPC::WriteSend(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    if (UseAssembler(pFunction, pContext))
	{
	    if (IsShortIPC(pFunction, pContext, pFunction->GetSendDirection()))
		    WriteAsmShortSend(pFile, pFunction, pContext);
	    else
		    WriteAsmLongSend(pFile, pFunction, pContext);
	}
	else
	    CL4BEIPC::WriteSend(pFile, pFunction, pContext);
}

/** \brief writes the assembler short send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4V2BEIPC::WriteAsmShortSend(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sDummy = pNF->GetDummyVariable(pContext);

	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
	int nSndDir = pFunction->GetSendDirection();

	// to increase the confusing code:
	// if we have PROGRAM_USE_SYMBOLS set, we test for __PIC__ and PROFILE
	// then we write the three parts bPIC, bPROF, bNPROF if they are set
	bool bPIC = true;
	bool bPROF = true;
	bool bNPROF = true;
	bool bSymbols = pContext->IsOptionSet(PROGRAM_USE_SYMBOLS);
	bool bSendFlexpage = ((CL4BEMsgBufferType*)pFunction->GetMessageBuffer())->GetFlexpageCount(nSndDir) > 0;
	if (bSymbols)
	{
	    bPIC = pContext->HasSymbol("__PIC__");
		bPROF = pContext->HasSymbol("PROFILE") && !bPIC;
		bNPROF = !bPROF && !bPIC;
	}

	if (!bSymbols)
	    pFile->Print("#ifdef __PIC__\n");
	if (bPIC)
	{
		// PIC branch
		pFile->PrintIndent("asm volatile(\n");
		pFile->IncIndent();
		pFile->PrintIndent("\"pushl  %%%%ebx          \\n\\t\"\n");
		pFile->PrintIndent("\"pushl  %%%%ebp          \\n\\t\"\n"); // save ebp no memory references ("m") after this point
		pFile->PrintIndent("\"movl   %%%%eax,%%%%ebx    \\n\\t\"\n");
		if (bSendFlexpage)
		    pFile->PrintIndent("\"movl $2,%%%%eax       \\n\\t\"\n");
		else
			pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
		pFile->PrintIndent("\"movl   $-1,%%%%ebp    \\n\\t\"\n"); // no receive
		pFile->PrintIndent("IPC_SYSENTER\n");
		pFile->PrintIndent("\"popl   %%%%ebp          \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point
		pFile->PrintIndent("\"popl   %%%%ebx          \\n\\t\"\n");
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);                /* EAX, 0 */
		pFile->PrintIndent("\"=d\" (%s),\n", (const char*)sDummy);
		pFile->PrintIndent("\"=c\" (%s)\n", (const char*)sDummy);
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"a\" (");
		// get send parameter
		if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSndDir, false, pContext))
			pFile->Print("0");
		pFile->Print("),\n");                 /* EAX, 0 => EBX */
		pFile->PrintIndent("\"d\" (%s),\n", (const char*)pFunction->GetOpcodeConstName());                 /* EDX, 1 */
		pFile->PrintIndent("\"c\" (%s),\n", (const char*)sTimeout);                 /* ECX, 2 */
		pFile->PrintIndent("\"S\" (%s->lh.low),\n", (const char*)pObjName->GetName());          /* ESI    */
		pFile->PrintIndent("\"D\" (%s->lh.high)\n", (const char*)pObjName->GetName());          /* EDI    */
// 		pFile->PrintIndent(":\n");
// 		pFile->PrintIndent("\"memory\"\n");
		pFile->DecIndent();
		pFile->PrintIndent(");\n");
	} // PIC
	if (!bSymbols)
	{
		pFile->Print("#else // !__PIC__\n");
		pFile->Print("#ifndef PROFILE\n");
	}
	if (bNPROF) // is !__PIC__ && !PROFILE
	{
		// else
		pFile->PrintIndent("asm volatile(\n");
		pFile->IncIndent();
		pFile->PrintIndent("\"pushl  %%%%ebp          \\n\\t\"\n"); // save ebp no memory references ("m") after this point
		if (bSendFlexpage)
		    pFile->PrintIndent("\"movl $2,%%%%eax       \\n\\t\"\n");
		else
			pFile->PrintIndent("\"subl   %%%%eax,%%%%eax    \\n\\t\"\n");
		pFile->PrintIndent("\"movl   $-1,%%%%ebp    \\n\\t\"\n"); // no receive
		pFile->PrintIndent("IPC_SYSENTER\n");
		pFile->PrintIndent("\"popl   %%%%ebp          \\n\\t\"\n"); // restore ebp, no memory references ("m") before this point
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);                /* EAX, 0 */
		pFile->PrintIndent("\"=d\" (%s),\n", (const char*)sDummy);
		pFile->PrintIndent("\"=b\" (%s),\n", (const char*)sDummy);
		pFile->PrintIndent("\"=c\" (%s)\n", (const char*)sDummy);                /* ECX, 3 */
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"d\" (%s),\n", (const char*)pFunction->GetOpcodeConstName());                 /* EDX, 1 */
		pFile->PrintIndent("\"b\" (");
		if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSndDir, false, pContext))
			pFile->Print("0");
		pFile->Print("),\n");                 /* EBX, 2 */
		pFile->PrintIndent("\"c\" (%s),\n", (const char*)sTimeout);                 /* ECX, 3 */
		pFile->PrintIndent("\"S\" (%s->lh.low),\n", (const char*)pObjName->GetName());          /* ESI    */
		pFile->PrintIndent("\"D\" (%s->lh.high)\n", (const char*)pObjName->GetName());          /* EDI    */
// 		pFile->PrintIndent(":\n");
// 		pFile->PrintIndent("\"memory\"\n");
		pFile->DecIndent();
		pFile->PrintIndent(");\n");
	}
	if (!bSymbols)
	{
		pFile->Print("#endif // PROFILE\n");
		pFile->Print("#endif // __PIC__\n");
	}
}

/** \brief writes the assembler long send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CL4V2BEIPC::WriteAsmLongSend(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
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

	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	int nSndDir = pFunction->GetSendDirection();

	bool bSendShortIPC = ((CL4BEMsgBufferType*)pFunction->GetMessageBuffer())->IsShortIPC(nSndDir, pContext);
	bool bSendFlexpage = ((CL4BEMsgBufferType*)pFunction->GetMessageBuffer())->GetFlexpageCount(nSndDir) > 0;

	// to increase the confusing code:
	// if we have PROGRAM_USE_SYMBOLS set, we test for __PIC__ and PROFILE
	// then we write the three parts bPIC, bPROF, bNPROF if they are set
	bool bPIC = true;
	bool bPROF = true;
	bool bNPROF = true;
	bool bSymbols = pContext->IsOptionSet(PROGRAM_USE_SYMBOLS);
	if (bSymbols)
	{
	    bPIC = pContext->HasSymbol("__PIC__");
		bPROF = pContext->HasSymbol("PROFILE") && !bPIC;
		bNPROF = !bPROF && !bPIC;
	}

	if (!bSymbols)
	    pFile->Print("#ifdef __PIC__\n");
	if (bPIC)
	{
		// PIC branch
		pFile->PrintIndent("asm volatile(\n");
		pFile->IncIndent();
		pFile->PrintIndent("\"pushl  %%%%ebx         \\n\\t\"\n");
		pFile->PrintIndent("\"pushl  %%%%ebp         \\n\\t\"\n");
		if (bSendShortIPC)
		{
			pFile->PrintIndent("\"movl %%%%eax,%%%%ebx  \\n\\t\"\n");
			if (bSendFlexpage)
			    pFile->PrintIndent("\"movl   $2,%%%%eax     \\n\\t\"\n");
			else
				pFile->PrintIndent("\"subl   %%%%eax,%%%%eax   \\n\\t\"\n"); // snd msg descr = 0
		}
		else
		{
			// if long ipc we can extract the dwords directly from the msg buffer structure in EAX
			pFile->PrintIndent("\"movl %d(%%%%eax),%%%%ebx  \\n\\t\"\n", nMsgBase+4);
			pFile->PrintIndent("\"movl %d(%%%%eax),%%%%edx  \\n\\t\"\n", nMsgBase);
			if (bSendFlexpage)
			    pFile->PrintIndent("\"orl $2,%%%%eax    \\n\\t\"\n");
		}
		pFile->PrintIndent("\"movl   $-1, %%%%ebp  \\n\\t\"\n"); // receive short IPC
		pFile->PrintIndent("IPC_SYSENTER\n");
		pFile->PrintIndent("\"popl  %%%%ebp          \\n\\t\"\n");
		pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx    \\n\\t\"\n");
		pFile->PrintIndent("\"popl  %%%%ebx          \\n\\t\"\n");
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
		pFile->PrintIndent("\"=d\" (%s),\n", (const char*)sDummy);
		pFile->PrintIndent("\"=c\" (%s),\n", (const char*)sDummy);
		pFile->PrintIndent("\"=S\" (%s),\n", (const char*)sDummy);
		pFile->PrintIndent("\"=D\" (%s)\n", (const char*)sDummy);
		pFile->PrintIndent(":\n");
		// skip this if short IPC
		if (!bSendShortIPC)
		{
			pFile->PrintIndent("\"a\" (");
			if (pFunction->GetMessageBuffer()->HasReference())
				pFile->Print("%s", (const char *) sMsgBuffer);
			else
				pFile->Print("&%s", (const char *) sMsgBuffer);
			pFile->Print("),\n");
		}
		else
		{
			// -> if short IPC we have to set send dwords
			pFile->PrintIndent("\"a\" (*((%s*)(&(", (const char*)sMWord);
			pFunction->GetMessageBuffer()->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
			pFile->Print("[4])),\n");
			pFile->PrintIndent("\"d\" (*((%s*)(&(", (const char*)sMWord);
			pFunction->GetMessageBuffer()->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
			pFile->Print("[0])),\n");
		}
		pFile->PrintIndent("\"c\" (%s),\n", (const char*)sTimeout);
		pFile->PrintIndent("\"S\" (%s->lh.low),\n", (const char*)pObjName->GetName());
		pFile->PrintIndent("\"D\" (%s->lh.high)\n", (const char*)pObjName->GetName());
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"memory\"\n");
		pFile->DecIndent();
		pFile->PrintIndent(");\n");
    } // PIC
	if (!bSymbols)
	{
		pFile->Print("#else // !__PIC__\n");
		pFile->Print("#ifndef PROFILE\n");
	}
	if (bNPROF)
	{
		// else
		pFile->PrintIndent("asm volatile(\n");
		pFile->IncIndent();
		pFile->PrintIndent("\"pushl %%%%ebp          \\n\\t\"\n");   /* save ebp, no memory references ("m") after this point */
		pFile->PrintIndent("\"movl  $-1,%%%%ebp    \\n\\t\"\n"); /* recv msg descriptor = 0 */
		if (bSendShortIPC)
		{
			if (bSendFlexpage)
			    pFile->PrintIndent("\"movl $2,%%%%eax \\n\\t\"\n");
			else
				pFile->PrintIndent("\"subl   %%%%eax,%%%%eax \\n\\t\"\n"); /* send msg descriptor = 0 */
		}
		else
		{
			// extract dwords directly from msg buffer
			pFile->PrintIndent("\"movl %d(%%%%eax), %%%%ebx \\n\\t\"\n", nMsgBase+4);   /* dest.lh.high -> edi */
			pFile->PrintIndent("\"movl %d(%%%%eax), %%%%edx \\n\\t\"\n", nMsgBase);   /* dest.lh.low  -> esi */
			if (bSendFlexpage)
			    pFile->PrintIndent("\"orl $2, %%%%eax \\n\\t\"\n");
		}
		pFile->PrintIndent("IPC_SYSENTER\n");
		pFile->PrintIndent("\"popl  %%%%ebp          \\n\\t\"\n");   /* restore ebp, no memory references ("m") before this point */
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);               /* EAX, 0 */
		pFile->PrintIndent("\"=d\" (%s),\n", (const char*)sDummy);           /* EDX, 1 */
		pFile->PrintIndent("\"=b\" (%s),\n", (const char*)sDummy);           /* EBX, 2 */
		pFile->PrintIndent("\"=c\" (%s)\n", (const char*)sDummy);                 /* EDI, 3 */
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"S\" (%s->lh.low),\n", (const char*)pObjName->GetName());                   /* dest, 4  */
		pFile->PrintIndent("\"D\" (%s->lh.high),\n", (const char*)pObjName->GetName());                   /* dest, 4  */
		if (bSendShortIPC)
		{
			pFile->PrintIndent("\"b\" (*((%s*)(&(", (const char*)sMWord);
			pFunction->GetMessageBuffer()->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
			pFile->Print("[4])),\n");
			pFile->PrintIndent("\"d\" (*((%s*)(&(", (const char*)sMWord);
			pFunction->GetMessageBuffer()->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
			pFile->Print("[0])),\n");
		}
		else
		{
			pFile->PrintIndent("\"a\" (");
			if (pFunction->GetMessageBuffer()->HasReference())
				pFile->Print("%s", (const char *) sMsgBuffer);
			else
				pFile->Print("&%s", (const char *) sMsgBuffer);
			pFile->Print("),\n");           /* EAX, 0 */
		}
		pFile->PrintIndent("\"c\" (%s)\n", (const char*)sTimeout);                /* timeout, 5 */
		pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"memory\"\n");
		pFile->DecIndent();
		pFile->PrintIndent(");\n");
    }
	if (!bSymbols)
	{
		pFile->Print("#endif // PROFILE\n");
		pFile->Print("#endif // __PIC__\n");
	}
}

