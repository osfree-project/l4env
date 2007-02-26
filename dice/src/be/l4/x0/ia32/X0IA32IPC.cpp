/**
 *	\file	dice/src/be/l4/x0/ia32/X0IA32IPC.h
 *	\brief	contains the declaration of the class CX0IA32IPC
 *
 *	\date	08/13/2002
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

#include "be/l4/x0/ia32/X0IA32IPC.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BEFile.h"
#include "be/BEFunction.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "be/BEType.h"
#include "be/BEMarshaller.h"
#include "TypeSpec-Type.h"

IMPLEMENT_DYNAMIC(CX0IA32IPC);

CX0IA32IPC::CX0IA32IPC()
 : CL4X0BEIPC()
{
    IMPLEMENT_DYNAMIC_BASE(CX0IA32IPC, CL4X0BEIPC);
}

/** destroys the IPC object */
CX0IA32IPC::~CX0IA32IPC()
{
}

/** \brief test if we can use assembler code
 *  \param pFunction the function to test
 *  \param pContext the context of the test
 *  \return true if assembler can be used
 */
bool CX0IA32IPC::UseAssembler(CBEFunction* pFunction,  CBEContext* pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
	    return false;
	if (pContext->GetOptimizeLevel() < 2)
	    return false;
	// test if the position size fits the parameters
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    if (!pMarshaller->TestPositionSize(pFunction, 4, pFunction->GetReceiveDirection(), false, false, 3 /* must fit 3 registers */, pContext))
	{
	    delete pMarshaller;
	    return false;
	}
	delete pMarshaller;
	// no objections!
	return true;
}

/** \brief writes the ipc code for the call
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteCall(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    if (UseAssembler(pFunction, pContext))
	{
	    if (IsShortIPC(pFunction, pContext))
		    WriteAsmShortCall(pFile, pFunction, pContext);
	    else
		    WriteAsmLongCall(pFile, pFunction, pContext);
	}
	else
	    CL4X0BEIPC::WriteCall(pFile, pFunction, pContext);
}

/** \brief writes the assembler IPC code for the call IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write oepration
 */
void CX0IA32IPC::WriteAsmShortCall(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sDummy = pNF->GetDummyVariable(pContext);

	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
	int nRecvDir = pFunction->GetReceiveDirection();
	int nSendDir = pFunction->GetSendDirection();

	// get exception
	CBETypedDeclarator *pException = pFunction->GetExceptionWord();
	CBEDeclarator *pExcDecl = 0;
	if (pException)
	{
		pIter = pException->GetFirstDeclarator();
        pExcDecl = pException->GetNextDeclarator(pIter);
	}
	int nIndex = 0;

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
	if (pExcDecl)
	{
	    pFile->Print("%s", (const char*)pExcDecl->GetName());
		nIndex = 1;
	}
	else
	{
	    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRecvDir, true, pContext))
			pFile->Print("%s", (const char*)sDummy);
	    nIndex = 2;
	}
	pFile->Print("),                /* EDX, 1 */\n");
	pFile->PrintIndent("\"=c\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nRecvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),                /* ECX, 2 */\n");
	pFile->PrintIndent("\"=D\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nRecvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print(")                 /* EDI, 3 */\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSendDir, false, pContext))
		pFile->Print("0");
	pFile->Print("),                 /* EAX, 0 => EBX */\n");
	pFile->PrintIndent("\"1\" (%s),                 /* EDX, 1 */\n", (const char*)pFunction->GetOpcodeConstName());
	pFile->PrintIndent("\"2\" (%s),                 /* ECX, 2 */\n", (const char*)sTimeout);
	pFile->PrintIndent("\"S\" (%s->raw),             /* ESI    */\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"3\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nSendDir, false, pContext))
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
	if (pExcDecl)
	{
	    pFile->Print("%s", (const char*)pExcDecl->GetName());
		nIndex = 1;
	}
	else
	{
	    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRecvDir, true, pContext))
			pFile->Print("%s", (const char*)sDummy);
	    nIndex = 2;
	}
	pFile->Print("),                /* EDX, 1 */\n");
	pFile->PrintIndent("\"=b\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nRecvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),                /* EBX, 2 */\n");
	pFile->PrintIndent("\"=c\" (%s),                /* ECX, 3 */\n", (const char*)sDummy);
	pFile->PrintIndent("\"=D\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, nIndex++, 4, nRecvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print(")                 /* EDI, 4 */\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"1\" (%s),                 /* EDX, 1 */\n", (const char*)pFunction->GetOpcodeConstName());
	pFile->PrintIndent("\"2\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSendDir, false, pContext))
		pFile->Print("0");
	pFile->Print("),                 /* EBX, 2 */\n");
	pFile->PrintIndent("\"3\" (%s),                 /* ECX, 3 */\n", (const char*)sTimeout);
	pFile->PrintIndent("\"S\" (%s->raw),             /* ESI    */\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"4\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nSendDir, false, pContext))
		pFile->Print("0");
	pFile->Print(")                  /* EDI, 4 */\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");
	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief writes the assembler code for the long IPC call
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmLongCall(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	String sDummy = pNF->GetDummyVariable(pContext);
	CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType*)pFunction->GetMessageBuffer();
	int nSendDir = pFunction->GetSendDirection();
	int nRecvDir = pFunction->GetReceiveDirection();

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
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n");
	pFile->PrintIndent("\"=S\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
    if (pMsgBuffer->IsShortIPC(nSendDir, pContext))
        pFile->Print("L4_IPC_SHORT_MSG");
    else
    {
		if (pMsgBuffer->HasReference())
			pFile->Print("%s", (const char *) sMsgBuffer);
		else
			pFile->Print("&%s", (const char *) sMsgBuffer);
	}
	pFile->Print("),\n");
	pFile->PrintIndent("\"1\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])),\n");
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"3\" (");
    if (pMsgBuffer->IsShortIPC(nRecvDir, pContext))
        pFile->Print("L4_IPC_SHORT_MSG");
    else
    {
	    pFile->Print("((int)");
        if (pMsgBuffer->HasReference())
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
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8])))),\n");
	pFile->PrintIndent("\"=c\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"0\" (");
    if (pMsgBuffer->IsShortIPC(nSendDir, pContext))
        pFile->Print("L4_IPC_SHORT_MSG");
    else
    {
		if (pMsgBuffer->HasReference())
			pFile->Print("%s", (const char *) sMsgBuffer);
		else
			pFile->Print("&%s", (const char *) sMsgBuffer);
	}
	pFile->Print("),\n");
	pFile->PrintIndent("\"1\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])),\n");
	pFile->PrintIndent("\"2\" (");
    if (pMsgBuffer->IsShortIPC(nRecvDir, pContext))
        pFile->Print("L4_IPC_SHORT_MSG");
    else
    {
	    pFile->Print("((int)");
        if (pMsgBuffer->HasReference())
            pFile->Print("%s", (const char *) sMsgBuffer);
        else
            pFile->Print("&%s", (const char *) sMsgBuffer);
	    pFile->Print(") & (~L4_IPC_OPEN_IPC)");
    }
	pFile->Print("),\n");
	pFile->PrintIndent("\"4\" (%s)\n", (const char*)sTimeout);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");

	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief writes the receive IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bAllowShortIPC true if short IPC usage is allowed
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteReceive(CBEFile* pFile,  CBEFunction* pFunction,  bool bAllowShortIPC,  CBEContext* pContext)
{
    if (UseAssembler(pFunction, pContext))
	{
	    if (IsShortIPC(pFunction, pContext, pFunction->GetReceiveDirection()) &&
		    bAllowShortIPC)
		    WriteAsmShortReceive(pFile, pFunction, pContext);
	    else
		    WriteAsmLongReceive(pFile, pFunction, pContext);
	}
	else
	    CL4X0BEIPC::WriteReceive(pFile, pFunction, bAllowShortIPC, pContext);
}

/** \brief writes the assembler version of the short receive IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmShortReceive(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sDummy = pNF->GetDummyVariable(pContext);
    int nRcvDir = pFunction->GetReceiveDirection();
	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);

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
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRcvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),                /* EDX, 1 */\n");
	pFile->PrintIndent("\"=c\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRcvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),                /* ECX, 2 */\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 3, 4, nRcvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
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
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRcvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),                /* EDX, 1 */\n");
	pFile->PrintIndent("\"=b\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRcvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),                /* ECX, 2 */\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 3, 4, nRcvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
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

/** \brief writes the assembler version of the long receive IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operaion
 */
void CX0IA32IPC::WriteAsmLongReceive(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType*)pFunction->GetMessageBuffer();

	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);

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
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n");
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
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") & (~L4_IPC_OPEN_IPC)),\n"); /* EAX => EBP */
	pFile->PrintIndent("\"2\" (%s)\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");

	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief writes the reply and wait IPC code
 *  \param pFile the file to write to
 *  \param pFunction the funtion to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteReplyAndWait(CBEFile* pFile,  CBEFunction* pFunction,  bool bSendFlexpage,  bool bSendShortIPC,  CBEContext* pContext)
{
    if (UseAssembler(pFunction, pContext))
	{
	    if (bSendFlexpage)
		{
		    if (bSendShortIPC)
			    WriteAsmShortFpageReplyAndWait(pFile, pFunction, pContext);
		    else
			    WriteAsmLongFpageReplyAndWait(pFile, pFunction, pContext);
		}
		else
		{
		    if (bSendShortIPC)
			    WriteAsmShortReplyAndWait(pFile, pFunction, pContext);
		    else
			    WriteAsmLongReplyAndWait(pFile, pFunction, pContext);
		}
	}
	else
	    CL4X0BEIPC::WriteReplyAndWait(pFile, pFunction, bSendFlexpage, bSendShortIPC, pContext);
}

/** \brief writes the short fpage IPC for the reply-and-wait IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmShortFpageReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);

	CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType*)pFunction->GetMessageBuffer();
	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);

	pFile->Print("#if defined(__PIC__)\n");
	// PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl %%%%ebx         \\n\\t\"\n");
	pFile->PrintIndent("\"pushl %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent("\"movl  %%%%eax,%%%%ebp   \\n\\t\"\n");
	pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
	pFile->PrintIndent("\"movl  $2,%%%%eax      \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx   \\n\\t\"\n");
	pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent("\"popl  %%%%ebx         \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC),\n"); /* EAX => EBP (open ipc) */
	pFile->PrintIndent("\"1\" (&(");
	pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"3\" (%s->raw)\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");

	pFile->Print("#else // !PIC\n");
	pFile->Print("#if !defined(PROFILE)\n");
	// !PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent("\"movl  %%%%eax,%%%%ebp   \\n\\t\"\n"); // rcv descr
	pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx   \\n\\t\"\n"); // timeout
	pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
	pFile->PrintIndent("\"movl  $2,%%%%eax      \\n\\t\"\n"); /* fpage */
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC),\n"); /* EAX => EBP (open ipc) */
	pFile->PrintIndent("\"1\" (&(");
	pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"3\" (%s->raw)\n", (const char*)pObjName->GetName());
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");

	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief writes the long fpage IPC for the reply-and-wait IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmLongFpageReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);

	CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType*)pFunction->GetMessageBuffer();
	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);

	pFile->Print("#if defined(__PIC__)\n");
	// PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl %%%%ebx         \\n\\t\"\n");
	pFile->PrintIndent("\"pushl %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent("\"movl  %%%%edi,%%%%ebp   \\n\\t\"\n");
	pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx   \\n\\t\"\n");
	pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent("\"popl  %%%%ebx         \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(")|2),\n"); /* EAX => EBP (open ipc) */
	pFile->PrintIndent("\"1\" (&(");
	pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"3\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"4\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC)\n"); /* EAX => EBP (open ipc) */
	pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");

	pFile->Print("#else // !PIC\n");
	pFile->Print("#if !defined(PROFILE)\n");
	// !PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx   \\n\\t\"\n");
	pFile->PrintIndent("\"movl  %%%%edi,%%%%ebp   \\n\\t\"\n");
	pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(")|2),\n"); /* EAX => EBP (open ipc) */
	pFile->PrintIndent("\"1\" (&(");
	pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"3\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"4\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC)\n"); /* EAX => EBP (open ipc) */
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");

	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief writes the short IPC for the reply-and-wait IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmShortReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);

	CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType*)pFunction->GetMessageBuffer();
	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);

	pFile->Print("#if defined(__PIC__)\n");
	// PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl %%%%ebx         \\n\\t\"\n");
	pFile->PrintIndent("\"pushl %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent("\"movl  %%%%eax,%%%%ebp   \\n\\t\"\n");
	pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
	pFile->PrintIndent("\"subl  %%%%eax,%%%%eax   \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx   \\n\\t\"\n");
	pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent("\"popl  %%%%ebx         \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC),\n");
	pFile->PrintIndent("\"1\" (&(");
	pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"3\" (%s->raw)\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");

	pFile->Print("#else // !PIC\n");
	pFile->Print("#if !defined(PROFILE)\n");
	// !PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx   \\n\\t\"\n"); // timeout
	pFile->PrintIndent("\"movl  %%%%eax,%%%%ebp   \\n\\t\"\n"); // rcv descr
	pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
	pFile->PrintIndent("\"subl  %%%%eax,%%%%eax   \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC),\n");
	pFile->PrintIndent("\"1\" (&(");
	pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout); /* EAX => ECX */
	pFile->PrintIndent("\"3\" (%s->raw)\n", (const char*)pObjName->GetName());
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");

	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief writes the long IPC for the reply-and-wait IPC
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmLongReplyAndWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);

	CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType*)pFunction->GetMessageBuffer();
	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);

	pFile->Print("#if defined(__PIC__)\n");
	// PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl %%%%ebx         \\n\\t\"\n");
	pFile->PrintIndent("\"pushl %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent("\"movl  %%%%edi,%%%%ebp   \\n\\t\"\n");
	pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx   \\n\\t\"\n");
	pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent("\"popl  %%%%ebx         \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print("),\n"); /* EAX => EBP (open ipc) */
	pFile->PrintIndent("\"1\" (&(");
	pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"3\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"4\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC)\n"); /* EAX => EBP (open ipc) */
	pFile->PrintIndent(":\n");
    pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");

	pFile->Print("#else // !PIC\n");
	pFile->Print("#if !defined(PROFILE)\n");
	// !PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent("\"movl  %%%%ebx,%%%%ecx   \\n\\t\"\n");
	pFile->PrintIndent("\"movl  %%%%edi,%%%%ebp   \\n\\t\"\n");
	pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl  %%%%ebp         \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print("),\n"); /* EAX => EBP (open ipc) */
	pFile->PrintIndent("\"1\" (&(");
	pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("),\n"); /* EDX => EDI,EBX,EDX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)sTimeout); /* EBX => ECX */
	pFile->PrintIndent("\"3\" (%s->raw),\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent("\"4\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC)\n"); /* EAX => EBP (open ipc) */
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");

	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief writes the send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteSend(CBEFile* pFile,  CBEFunction* pFunction,  CBEContext* pContext)
{
    if (UseAssembler(pFunction, pContext))
    {
	    if (IsShortIPC(pFunction, pContext, pFunction->GetSendDirection()))
		    WriteAsmShortSend(pFile, pFunction, pContext);
        else
		    WriteAsmLongSend(pFile, pFunction, pContext);
    }
	else
	    CL4X0BEIPC::WriteSend(pFile, pFunction, pContext);
}

/** \brief write the assembler short send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmShortSend(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sDummy = pNF->GetDummyVariable(pContext);
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	int nSendDir = pFunction->GetSendDirection();

	pFile->Print("#if defined(__PIC__)\n");
	// PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
	pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
	pFile->PrintIndent("\"movl   $-1,%%%%ebp  \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
	pFile->PrintIndent("\"movl   %%%%eax,%%%%ebx  \\n\\t\"\n");
	pFile->PrintIndent("\"subl   %%%%eax,%%%%eax  \\n\\t\"\n"); /* EAX = 0 (short IPC) */
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebx        \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=c\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=d\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=S\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=D\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
		pFile->PrintIndent("\"0\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSendDir, false, pContext))
		pFile->Print("0");
	pFile->Print("),\n"); /* EAX */
	pFile->PrintIndent("\"1\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)pFunction->GetOpcodeConstName()); /* EDX */
	pFile->PrintIndent("\"3\" (%s->raw),\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent("\"4\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nSendDir, false, pContext))
		pFile->Print("0");
	pFile->Print(")\n"); /* EDI */
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
	pFile->PrintIndent("\"movl   %%%%eax,%%%%ebx  \\n\\t\"\n");
	pFile->PrintIndent("\"movl   $-1,%%%%ebp  \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
	pFile->PrintIndent("\"subl   %%%%eax,%%%%eax  \\n\\t\"\n"); /* EAX = 0 (short IPC) */
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=c\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=d\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=S\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=D\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSendDir, false, pContext))
		pFile->Print("0");
	pFile->Print("),\n"); /* EAX => EBX */ // kann man das direkt in EBX laden?
	pFile->PrintIndent("\"1\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)pFunction->GetOpcodeConstName()); /* EDX */
	pFile->PrintIndent("\"3\" (%s->raw),\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent("\"4\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nSendDir, false, pContext))
		pFile->Print("0");
	pFile->Print(")\n"); /* EDI */
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"ebx\", \"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");

	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief write the assembler long send IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmLongSend(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
	String sDummy = pNF->GetDummyVariable(pContext);
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType*)pFunction->GetMessageBuffer();
	int nSendDir = pFunction->GetSendDirection();

	pFile->Print("#if defined(__PIC__)\n");
	// PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl  %%%%ebx        \\n\\t\"\n");
	pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
	pFile->PrintIndent("\"movl 8(%%%%edx),%%%%edi \\n\\t\"\n");
	pFile->PrintIndent("\"movl 4(%%%%edx),%%%%ebx \\n\\t\"\n");
	pFile->PrintIndent("\"movl  (%%%%edx),%%%%edx \\n\\t\"\n");
	pFile->PrintIndent("\"movl   $-1,%%%%ebp    \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
	pFile->PrintIndent("\"popl   %%%%ebx        \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=c\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=d\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=S\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print("),\n");
	pFile->PrintIndent("\"1\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"2\" (&(");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[0])),\n");
	pFile->PrintIndent("\"3\" (%s->raw)\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"edi\", \"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");

	pFile->Print("#else // !PIC\n");
	pFile->Print("#if !defined(PROFILE)\n");
	// !PIC branch
	pFile->PrintIndent("asm volatile(\n");
	pFile->IncIndent();
	pFile->PrintIndent("\"pushl  %%%%ebp        \\n\\t\"\n");
	pFile->PrintIndent("\"movl   $-1,%%%%ebp  \\n\\t\"\n"); /* EBP = 0xffffffff (no reply) */
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp        \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
	pFile->PrintIndent("\"=c\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=d\" (%s),\n", (const char*)sDummy);
	pFile->PrintIndent("\"=S\" (%s)\n", (const char*)sDummy);
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print("),\n");
	pFile->PrintIndent("\"1\" (%s),\n", (const char*)sTimeout); /* ECX */
	pFile->PrintIndent("\"2\" (%s),\n", (const char*)pFunction->GetOpcodeConstName()); /* EDX */
	pFile->PrintIndent("\"3\" (%s->raw),\n", (const char*)pObjName->GetName()); /* ESI */
	pFile->PrintIndent("\"b\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSendDir, false, pContext))
		pFile->Print("0");
	pFile->Print("),\n"); /* EBX */
	pFile->PrintIndent("\"D\" (");
    if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nSendDir, false, pContext))
		pFile->Print("0");
	pFile->Print(")\n"); /* EDI */
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"ebx\", \"edi\", \"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");

	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}

/** \brief writes the wait IPC code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param bAllowShortIPC true if short IPC is allowed
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteWait(CBEFile* pFile,  CBEFunction* pFunction,  bool bAllowShortIPC,  CBEContext* pContext)
{
    if (UseAssembler(pFunction, pContext))
    {
	    if (IsShortIPC(pFunction, pContext, pFunction->GetReceiveDirection()) &&
		    bAllowShortIPC)
		    WriteAsmShortWait(pFile, pFunction, pContext);
        else
		    WriteAsmLongWait(pFile, pFunction, pContext);
    }
	else
	    CL4X0BEIPC::WriteWait(pFile, pFunction, bAllowShortIPC, pContext);
}

/** \brief writes the assembler version of the short IPC wait code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmShortWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);

	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
	String sDummy = pNF->GetDummyVariable(pContext);
	int nRcvDir = pFunction->GetReceiveDirection();

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
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRcvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),                /* EDX, 1 */\n");
	pFile->PrintIndent("\"=c\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRcvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),                /* ECX, 2 */\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 3, 4, nRcvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
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
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 1, 4, nRcvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),                /* EDX, 1 */\n");
	pFile->PrintIndent("\"=b\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 2, 4, nRcvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
	pFile->Print("),                /* ECX, 2 */\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (");
	if (!pMarshaller->MarshalToPosition(pFile, pFunction, 3, 4, nRcvDir, true, pContext))
		pFile->Print("%s", (const char*)sDummy);
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

/** \brief writes the assembler version of the long IPC wait code
 *  \param pFile the file to write to
 *  \param pFunction the function to write for
 *  \param pContext the context of the write operation
 */
void CX0IA32IPC::WriteAsmLongWait(CBEFile* pFile, CBEFunction* pFunction, CBEContext* pContext)
{
	CL4BENameFactory *pNF = (CL4BENameFactory*)pContext->GetNameFactory();
	String sResult = pNF->GetResultName(pContext);
	String sTimeout = pNF->GetTimeoutClientVariable(pContext);
	String sMsgBuffer = pNF->GetMessageBufferVariable(pContext);
    String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);

	VectorElement *pIter = pFunction->GetObject()->GetFirstDeclarator();
	CBEDeclarator *pObjName = pFunction->GetObject()->GetNextDeclarator(pIter);
	CBEMsgBufferType *pMsgBuffer = pFunction->GetMessageBuffer();

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
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=c\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (L4_IPC_NIL_DESCRIPTOR),\n");
	pFile->PrintIndent("\"1\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC),\n");
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
	pFile->PrintIndent("\"pushl  %%%%ebp            \\n\\t\"\n"); // save ebp no memory references ("m") after this point
	pFile->PrintIndent("\"movl   %%%%edx,%%%%ecx      \\n\\t\"\n"); /* timeout, ecx */
	pFile->PrintIndent("\"movl   %%%%ebx,%%%%ebp      \\n\\t\"\n"); /* rcv short ipc, open wait */
	pFile->PrintIndent("IPC_SYSENTER\n");
	pFile->PrintIndent("\"popl   %%%%ebp            \\n\\t\"\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"=a\" (%s),\n", (const char*)sResult);
    pFile->PrintIndent("\"=d\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])))),\n");
	pFile->PrintIndent("\"=b\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[4])))),\n");
	pFile->PrintIndent("\"=S\" (%s->raw),\n", (const char*)pObjName->GetName());
	pFile->PrintIndent("\"=D\" (*((%s*)(&(", (const char*)sMWord);
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
	pFile->Print("[8]))))\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"0\" (L4_IPC_NIL_DESCRIPTOR),\n");
	pFile->PrintIndent("\"1\" (%s),\n", (const char*)sTimeout);
	pFile->PrintIndent("\"2\" (((int)");
	if (pMsgBuffer->HasReference())
		pFile->Print("%s", (const char *) sMsgBuffer);
	else
		pFile->Print("&%s", (const char *) sMsgBuffer);
	pFile->Print(") | L4_IPC_OPEN_IPC)\n");
	pFile->PrintIndent(":\n");
	pFile->PrintIndent("\"ecx\", \"memory\"\n");
	pFile->DecIndent();
	pFile->PrintIndent(");\n");

	pFile->Print("#endif // !PROFILE\n");
	pFile->Print("#endif // PIC\n");
}
