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
#include "be/l4/L4BEClassFactory.h"
#include "be/l4/L4BEIPC.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "be/BEType.h"
#include "be/BEMarshaller.h"
#include "be/BECommunication.h"

#include "TypeSpec-Type.h"
#include "fe/FEAttribute.h"

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
    CL4BEMsgBufferType *pMsgBuffer = (CL4BEMsgBufferType*)m_pMsgBuffer;
    if (((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext) &&
	    pMsgBuffer->IsShortIPC(0, pContext))
    {
		int nSendDirection = GetSendDirection();
		bool bHasSizeIsParams = (GetParameterCount(ATTR_SIZE_IS, ATTR_REF, nSendDirection) > 0) ||
			(GetParameterCount(ATTR_LENGTH_IS, ATTR_REF, nSendDirection) > 0) ||
			(GetParameterCount(ATTR_STRING, ATTR_REF, nSendDirection) > 0);
	    if (pContext->IsOptionSet(PROGRAM_USE_SYMBOLS))
        {
		    if (!pContext->HasSymbol("__PIC__") &&
			     pContext->HasSymbol("PROFILE"))
            {
    			pMsgBuffer->WriteSendDopeInit(pFile, nSendDirection, bHasSizeIsParams, pContext);
            }
        }
		else
		{
			pFile->Print("#if !defined(__PIC__) && defined(PROFILE)\n");
			pMsgBuffer->WriteSendDopeInit(pFile, nSendDirection, bHasSizeIsParams, pContext);
			pFile->Print("#endif\n");
		}
	    // skip send dope init
		if (!pContext->IsOptionSet(PROGRAM_NO_SEND_CANCELED_CHECK))
		{
			// sometimes it's possible to abort a call of a client.
			// but the client wants his call made, so we try until
			// the call completes
			pFile->PrintIndent("do\n");
			pFile->PrintIndent("{\n");
			pFile->IncIndent();
		}
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

/* \brief write the marshalling code for the short IPC
 * \param pFile the file to write to
 * \param nStartOffset the offset where to start marshalling
 * \param bUseConstOffset true if nStartOffset can be used
 * \param pContext the context of the write operation
 *
 * If we have a short IPC into both direction, we skip the marshalling.
 *
 * The special about V2 is, that we have to marshal flexpage before the
 * opcode. They always have to come first.
 */
void CL4V2BECallFunction::WriteMarshalling(CBEFile * pFile,  int nStartOffset,
                           bool & bUseConstOffset,  CBEContext * pContext)
{
    bool bUseAsmShort = ((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext) &&
	                    ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(0, pContext);
    bool bUseSymbols = pContext->IsOptionSet(PROGRAM_USE_SYMBOLS);
    // for asm short IPC we only need it if !PIC && PROFILE
    if (bUseAsmShort && !bUseSymbols)
	    pFile->Print("#if !defined(__PIC__) && defined(PROFILE)\n");
    if ((bUseAsmShort && bUseSymbols &&
	    !pContext->HasSymbol("__PIC__") && pContext->HasSymbol("PROFILE")) ||
		!bUseAsmShort)
    {
	    CL4BECallFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
    }
    if (bUseAsmShort && !bUseSymbols)
	    pFile->Print("#endif\n");
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
    if (((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext) &&
	    ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(0, pContext))
	{
	    // if short IPC consists of more than two parameters (bit-stuffing), then we
		// have to "unstuff" now
		// if !PIC && PROFILE we use "normal" IPC
		if (pContext->IsOptionSet(PROGRAM_USE_SYMBOLS))
		{
		    if (!pContext->HasSymbol("__PIC__") &&
			     pContext->HasSymbol("PROFILE"))
		    {
        		CL4BECallFunction::WriteUnmarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
		    }
		}
		else
		{
			pFile->Print("#if !defined(__PIC__) && defined(PROFILE)\n");
			CL4BECallFunction::WriteUnmarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
			pFile->Print("#endif\n");
		}
	}
	else
        CL4BECallFunction::WriteUnmarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
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
    bool bUseAssembler = ((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext);
    CL4BEMsgBufferType *pMsgBuf = (CL4BEMsgBufferType*)m_pMsgBuffer;
    bool bShortIPC = pMsgBuf->IsShortIPC(0, pContext);
    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
	    !bShortIPC)
	    pFile->PrintIndent("int _i;\n");
    if (bUseAssembler && bShortIPC)
	{
		// write dummys
		CBENameFactory *pNF = pContext->GetNameFactory();
		String sDummy = pNF->GetDummyVariable(pContext);
		String sMWord = pNF->GetTypeName(TYPE_MWORD, false, pContext, 0);
		String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);

        if (pContext->IsOptionSet(PROGRAM_USE_SYMBOLS))
		{
			if (pContext->HasSymbol("__PIC__"))
			{
				WriteReturnVariableDeclaration(pFile, pContext);
				// write result variable
				pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", (const char *) sResult);
				pFile->PrintIndent("%s %s __attribute__((unused));\n",
									(const char*)sMWord, (const char*)sDummy);
			}
			else
			{
			    if (pContext->HasSymbol("PROFILE"))
            		CL4BECallFunction::WriteVariableDeclaration(pFile, pContext);
			    else
				{
					WriteReturnVariableDeclaration(pFile, pContext);
					// write result variable
					pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", (const char *) sResult);
					pFile->PrintIndent("%s %s = 0;\n", (const char *)sMWord, (const char*)sDummy);
				}
			}
		}
        else
		{
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
	}
	else
	{
	    CL4BECallFunction::WriteVariableDeclaration(pFile, pContext);
	    if (bUseAssembler)
		{
		    // need dummies
			CBENameFactory *pNF = pContext->GetNameFactory();
			String sDummy = pNF->GetDummyVariable(pContext);
			String sMWord = pNF->GetTypeName(TYPE_MWORD, false, pContext, 0);
			if (pContext->IsOptionSet(PROGRAM_USE_SYMBOLS))
			{
			    if (pContext->HasSymbol("__PIC__"))
        	    	pFile->PrintIndent("%s %s __attribute__((unused));\n", (const char *)sMWord, (const char*)sDummy);
			    else if (!pContext->HasSymbol("PROFILE"))
        	    	pFile->PrintIndent("%s %s __attribute__((unused));\n", (const char *)sMWord, (const char*)sDummy);
			}
			else
			{
				pFile->Print("#if defined(__PIC__)\n");
				pFile->PrintIndent("%s %s __attribute__((unused));\n", (const char *)sMWord, (const char*)sDummy);
				pFile->Print("#else // !PIC\n");
				pFile->Print("#if !defined(PROFILE)\n");
				pFile->PrintIndent("%s %s __attribute__((unused));\n", (const char *)sMWord, (const char*)sDummy);
				pFile->Print("#endif // !PROFILE\n");
				pFile->Print("#endif // !PIC\n");
			}
		}
	}
}

/** \brief writest the IPC call
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4V2BECallFunction::WriteIPC(CBEFile * pFile,  CBEContext * pContext)
{
    bool bUseAssembler = ((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext);
    CL4BEMsgBufferType *pMsgBuf = (CL4BEMsgBufferType*)m_pMsgBuffer;
	bool bIsShortIPC = pMsgBuf->IsShortIPC(0, pContext);
	String sFunc = pContext->GetTraceClientFunc();
	if (sFunc.IsEmpty())
		sFunc = String("printf");

	if (pContext->IsOptionSet(PROGRAM_TRACE_CLIENT))
	{
		VectorElement *pIter = m_pCorbaObject->GetFirstDeclarator();
		CBEDeclarator *pObjName = m_pCorbaObject->GetNextDeclarator(pIter);
		String sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, false, pContext, 0);
	    pFile->PrintIndent("%s(\"%s: server %%x.%%x%s\", %s->id.task, %s->id.lthread);\n",
		                    (const char*)sFunc,
		                    (sFunc=="LOG")?"":(const char*)GetName(),
							(sFunc=="LOG")?"":"\\n",
							(const char*)(pObjName->GetName()),
							(const char*)(pObjName->GetName()));
        pFile->PrintIndent("%s(\"%s: with dw0=%%x, dw1=%%x%s\", ",
		                    (const char*)sFunc,
							(sFunc=="LOG")?"":(const char*)GetName(),
							(sFunc=="LOG")?"":"\\n");
		if (bUseAssembler && bIsShortIPC)
		{
        	pFile->Print("%s, ", (const char*)m_sOpcodeConstName);                 /* EDX, 1 */
			CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
			if (!pMarshaller->MarshalToPosition(pFile, this, 1, 4, GetSendDirection(), false, pContext))
				pFile->Print("0");
        }
		else
		{
			pFile->Print("(*((%s*)(&(", (const char*)sMWord);
			m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
			pFile->Print("[0])))), ");
			pFile->Print("(*((%s*)(&(", (const char*)sMWord);
			m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
			pFile->Print("[4]))))");
		}
		pFile->Print(");\n");
    }
    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
	    !bIsShortIPC)
	{
		pFile->PrintIndent("%s(\"%s before call%s\");\n", (const char*)sFunc,
							(const char*)GetName() ,(sFunc=="LOG")?"":"\\n");
		pMsgBuf->WriteDump(pFile, String(), pContext);
	}

	// IPC Code start
	CL4BECallFunction::WriteIPC(pFile, pContext);
	// IPC Code stop

	String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
	if (pContext->IsOptionSet(PROGRAM_TRACE_CLIENT))
	{
	    pFile->PrintIndent("%s(\"%s: return dope %%x (ipc error %%x)%s\", %s.msgdope, L4_IPC_ERROR(%s));\n",
		                    (const char*)sFunc, (sFunc=="LOG")?"":(const char*)GetName(),
							(sFunc=="LOG")?"":"\\n", (const char*)sResult,
							(const char*)sResult);
    }
    if (pContext->IsOptionSet(PROGRAM_TRACE_MSGBUF) &&
	    !bIsShortIPC)
	{
		pFile->PrintIndent("%s(\"%s after call%s\");\n", (const char*)sFunc,
							(const char*)GetName(), (sFunc=="LOG")?"":"\\n");
		pMsgBuf->WriteDump(pFile, sResult, pContext);
	}
}

/** \brief writes the variable initialization
 *  \param pFile the file to write to
 *  \param pContext the context of teh write operation
 *
 * If we use the asm short IPC code we do not init any variables (Its done during declaration).
 */
void CL4V2BECallFunction::WriteVariableInitialization(CBEFile * pFile,  CBEContext * pContext)
{
    if (((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext) &&
	    ((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(0, pContext))
	{
	    if (pContext->IsOptionSet(PROGRAM_USE_SYMBOLS))
		{
		    if (!pContext->HasSymbol("__PIC__") &&
			     pContext->HasSymbol("PROFILE"))
		    {
        	    CL4BECallFunction::WriteVariableInitialization(pFile, pContext);
		    }
		}
		else
		{
			pFile->Print("#if !defined(__PIC__) && defined(PROFILE)\n");
			CL4BECallFunction::WriteVariableInitialization(pFile, pContext);
			pFile->Print("#endif // !PIC && PROFILE\n");
		}
	}
	else
	    CL4BECallFunction::WriteVariableInitialization(pFile, pContext);
}

