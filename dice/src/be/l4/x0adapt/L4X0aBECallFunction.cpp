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
#include "be/l4/L4BEIPC.h"
#include "be/BEDeclarator.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEMarshaller.h"
#include "be/BEType.h"

#include "TypeSpec-Type.h"

IMPLEMENT_DYNAMIC(CL4X0aBECallFunction);

CL4X0aBECallFunction::CL4X0aBECallFunction()
 : CL4BECallFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0aBECallFunction, CL4BECallFunction);
}


CL4X0aBECallFunction::~CL4X0aBECallFunction()
{
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
    if (pContext->IsOptionSet(PROGRAM_TRACE_CLIENT))
	{
	    // check if we use assembler
		bool bAssembler = ((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext);
	    // dump
		VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
		CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);
		String sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, false, pContext, 0);
		String sFunc = pContext->GetTraceClientFunc();
		if (sFunc.IsEmpty())
		    sFunc = String("printf");
	    pFile->PrintIndent("%s(\"%s: server %%x.%%x%s\", %s->id.task, %s->id.lthread);\n",
		                    (const char*)sFunc, (const char*)GetName(),
							(sFunc=="LOG")?"":"\\n",
							(const char*)(pObjName->GetName()),
							(const char*)(pObjName->GetName()));
        pFile->PrintIndent("%s(\"%s: with dw0=%%x, dw1=%%x, dw2=%%x%s\", ",
		                    (const char*)sFunc, (const char*)GetName(),
							(sFunc=="LOG")?"":"\\n");
		if (bAssembler &&
		    ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(0, pContext))
		{
        	pFile->Print("%s, ", (const char*)m_sOpcodeConstName);                 /* EDX, 1 */
			CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
			if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetSendDirection(), false, pContext))
				pFile->Print("0");
			pFile->Print(", ");
			if (!pMarshaller->MarshalToPosition(pFile, this, 2, 4, GetSendDirection(), false, pContext))
				pFile->Print("0");
        }
		else
		{
			pFile->Print("(*((%s*)(&(", (const char*)sMWord);
			m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
			pFile->Print("[0])))), ");
			pFile->Print("(*((%s*)(&(", (const char*)sMWord);
			m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
			pFile->Print("[4])))), ");
			pFile->Print("(*((%s*)(&(", (const char*)sMWord);
			m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
			pFile->Print("[8]))))");
		}
		pFile->Print(");\n");
	}
    CL4BEMsgBufferType *pMsgBuf = (CL4BEMsgBufferType*)m_pMsgBuffer;
    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
	    !(pMsgBuf->IsShortIPC(GetSendDirection(), pContext) &&
          pMsgBuf->IsShortIPC(GetReceiveDirection(), pContext)))
	{
		String sFunc = pContext->GetTraceClientFunc();
		if (sFunc.IsEmpty())
		    sFunc = String("printf");
		pFile->PrintIndent("%s(\"%s before call%s\");\n", (const char*)sFunc,
		                    (const char*)GetName(), (sFunc=="LOG")?"":"\\n");
		((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteDump(pFile, String(), pContext);
	}

	CL4BECallFunction::WriteIPC(pFile, pContext); // will call IPC class

	if (pContext->IsOptionSet(PROGRAM_TRACE_CLIENT))
	{
		String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
		String sFunc = pContext->GetTraceClientFunc();
		if (sFunc.IsEmpty())
		    sFunc = String("printf");
	    pFile->PrintIndent("%s(\"%s: return dope %%x (ipc error %%x)%s\", %s.msgdope, L4_IPC_ERROR(%s));\n",
		                    (const char*)sFunc, (const char*)GetName(),
							(sFunc=="LOG")?"":"\\n",
							(const char*)sResult, (const char*)sResult);
	}
    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
	    !(pMsgBuf->IsShortIPC(GetSendDirection(), pContext) &&
          pMsgBuf->IsShortIPC(GetReceiveDirection(), pContext)))
	{
		String sFunc = pContext->GetTraceClientFunc();
		if (sFunc.IsEmpty())
		    sFunc = String("printf");
		pFile->PrintIndent("%s(\"%s before call%s\");\n", (const char*)sFunc,
		                    (const char*)GetName(), (sFunc=="LOG")?"":"\\n");
		((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteDump(pFile, String(), pContext);
	}
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
    CL4BEMsgBufferType *pMsgBuf = (CL4BEMsgBufferType*)m_pMsgBuffer;
    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
	    !(pMsgBuf->IsShortIPC(GetSendDirection(), pContext) &&
          pMsgBuf->IsShortIPC(GetReceiveDirection(), pContext)))
	    pFile->PrintIndent("int _i;\n");

	// check if we use assembler
	bool bAssembler = ((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext);
	CBENameFactory *pNF = pContext->GetNameFactory();
	String sDummy = pNF->GetDummyVariable(pContext);
	String sMWord = pNF->GetTypeName(TYPE_MWORD, false, pContext, 0);
    if (bAssembler &&
	    pMsgBuf->IsShortIPC(0, pContext))
	{
		// write dummys
		String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
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

		// test if we need dummies
		if (!bSymbols)
			pFile->Print("#if defined(__PIC__)\n");
		if (bPIC)
		{
			WriteReturnVariableDeclaration(pFile, pContext);
			// write result variable
			pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", (const char *) sResult);
			pFile->PrintIndent("%s %s __attribute__((unused));\n",
								(const char*)sMWord, (const char*)sDummy);
        }
		if (!bSymbols)
		{
			pFile->Print("#else // !PIC\n");
			pFile->Print("#if !defined(PROFILE)\n");
		}
		if (bNPROF)
		{
			WriteReturnVariableDeclaration(pFile, pContext);
			// write result variable
			pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", (const char *) sResult);
			pFile->PrintIndent("%s %s = 0;\n", (const char *)sMWord, (const char*)sDummy);
		}
		if (!bSymbols)
		{
			pFile->Print("#endif // PROFILE\n");
			pFile->Print("#endif // !PIC\n");
		}
		// if we have in either direction some bit-stuffing, we need more dummies
		// declare local exception variable
		WriteExceptionWordDeclaration(pFile, false /* do not init variable*/, pContext);
	    // finished with declaration
	}
	else
	{
	    CL4BECallFunction::WriteVariableDeclaration(pFile, pContext);
	    // we need the dummy for the assembler statements
        if (bAssembler)
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
	// check if we use assembler
    if (!(((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext) &&
	    ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(0, pContext)))
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
	// check if we use assembler
    if (!(((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext) &&
	    ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(0, pContext)))
	{
	    CL4BECallFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
    }
}

/* \brief write the unmarshalling code for the short IPC
 * \param pFile the file to write to
 * \param nStartOffset the offset where to start marshalling
 * \param bUseConstOffset true if nStartOffset can be used
 * \param pContext the context of the write operation
 *
 * If we have a short IPC in both direction, we can skip the unmarshalling.
 * In case we use the assembler code and its a short IPC, all the returned
 * parameters land in the variables directly, but we still have to extract
 * the exception from its variable.
 */
void CL4X0aBECallFunction::WriteUnmarshalling(CBEFile * pFile,  int nStartOffset,
                            bool & bUseConstOffset,  CBEContext * pContext)
{
	// check if we use assembler
    if (((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext) &&
	    ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(0, pContext))
	{
		WriteEnvExceptionFromWord(pFile, pContext);
		return;
	}

	// check flexpages
// 	int nFlexSize = 0, nReturnPos = 0;
//     if (((CL4BEMsgBufferType*) m_pMsgBuffer)->HasReceiveFlexpages())
//     {
//         // we have to always check if this was a flexpage IPC
//         String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
//         pFile->PrintIndent("if (l4_ipc_fpage_received(%s))\n", (const char*)sResult);
// 		pFile->PrintIndent("{\n");
// 		pFile->IncIndent();
// 	    // if we receive flexpages, the first will arrive just before the
// 		// return variable
// 		CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
//         // get flexpages
// 		nFlexSize = pMarshaller->Unmarshal(pFile, this, TYPE_FLEXPAGE, 0, nStartOffset, bUseConstOffset, pContext);
// 		// get return variable
// 		nReturnPos = nStartOffset+nFlexSize;
// 		nFlexSize += WriteUnmarshalReturn(pFile, nReturnPos, bUseConstOffset, pContext);
// 		// get rest of variables
// 		pMarshaller->Unmarshal(pFile, this, -TYPE_FLEXPAGE, 0/*all*/, nStartOffset+nFlexSize, bUseConstOffset, pContext);
// 		pFile->DecIndent();
// 		pFile->PrintIndent("}\n");
//         pFile->PrintIndent("else\n");
// 		pFile->PrintIndent("{\n");
//         // since we should always get receive flexpages we expect, this is the error case
//         // therefore we do not add this size to the offset, since we cannot trust it
//         pFile->IncIndent();
// 		// unmarshal exception first
// 		WriteUnmarshalException(pFile, nStartOffset, bUseConstOffset, pContext);
// 		// test for exception and return
// 		WriteExceptionCheck(pFile, pContext);
// 		// unmarshal return
// 		// even though there are no flexpages send, the marshalling didn't know if the flexpages
// 		// are valid. Though it reserved space for them. Therefore we have to apply the opcode
// 		// path here as well. The return code might return the reason for the invalid flexpages.
//         WriteFlexpageReturnPatch(pFile, nReturnPos, bUseConstOffset, pContext);
// 		// return
// 		// because this means error and we want to skip the rest of the unmarshalling
// 		WriteReturn(pFile, pContext);
//         pFile->DecIndent();
// 		pFile->PrintIndent("}\n");
// 		// add the size of the flexpage unmarshalling to offset
// 		nStartOffset += nFlexSize;
//     }
//     else
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
	// check if we use assembler
    if (((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext) &&
	    ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(0, pContext))
    {
		if (!pContext->IsOptionSet(PROGRAM_NO_SEND_CANCELED_CHECK))
		{
			// sometimes it's possible to abort a call of a client.
			// but the client wants his call made, so we try until
			// the call completes
			pFile->PrintIndent("do\n");
			pFile->PrintIndent("{\n");
			pFile->IncIndent();
		}
	    // skip send dope init
	    WriteIPC(pFile, pContext);
		if (!pContext->IsOptionSet(PROGRAM_NO_SEND_CANCELED_CHECK))
		{
			// now check if call has been canceled
			String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
			pFile->DecIndent();
			pFile->PrintIndent("} while ((L4_IPC_ERROR(%s) == L4_IPC_SEABORTED) || (L4_IPC_ERROR(%s) == L4_IPC_SECANCELED));\n",
								(const char*)sResult, (const char*)sResult);
		}
		WriteIPCErrorCheck(pFile, pContext);
    }
	else
        CL4BECallFunction::WriteInvocation(pFile, pContext);
}

