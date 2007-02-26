/**
 *	\file	dice/src/be/l4/L4BEReplyAnyWaitAnyFunction.cpp
 *	\brief	contains the implementation of the class CL4BEReplyAnyWaitAnyFunction
 *
 *	\date	Wed Jun 12 2002
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

#include "be/l4/L4BEReplyAnyWaitAnyFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEClassFactory.h"
#include "be/BEContext.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEType.h"
#include "be/BEOperationFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BEIPC.h"
#include "be/l4/L4BESizes.h"
#include "be/BEMarshaller.h"

#include "TypeSpec-Type.h"
#include "fe/FEAttribute.h"

IMPLEMENT_DYNAMIC(CL4BEReplyAnyWaitAnyFunction);

CL4BEReplyAnyWaitAnyFunction::CL4BEReplyAnyWaitAnyFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4BEReplyAnyWaitAnyFunction, CBEReplyAnyWaitAnyFunction);
}

/** destroys this object */
CL4BEReplyAnyWaitAnyFunction::~CL4BEReplyAnyWaitAnyFunction()
{
}

/** \brief invokes the ipc
 *  \param pFile the file to write to
 *  \param pContext the context of this operation
 *
 * This function assumes that the message buffer is marshalled and can be sent right away and that
 * the only thing to be unmarshalled if the opcode.
 *
 * Usually we set the message dopes before actually invoking the IPC, but
 * we don't do it here, because we have no clue which function this is coming
 * from. We trust the calling function to set the dopes correctly. We use the size
 * dope to test for the number of dwords to determine whether to use short IPC.
 */
void CL4BEReplyAnyWaitAnyFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    // invoke
    WriteIPC(pFile, pContext);
	WriteExceptionCheck(pFile, pContext); // resets exception
    WriteIPCErrorCheck(pFile, pContext); // set IPC exception
}

/**	\brief write the error checking code for the IPC
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 */
void CL4BEReplyAnyWaitAnyFunction::WriteIPCErrorCheck(CBEFile * pFile, CBEContext * pContext)
{
	String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
	pFile->PrintIndent("/* test for IPC errors */\n");
	pFile->PrintIndent("if (L4_IPC_IS_ERROR(%s))\n", (const char *) sResult);
	pFile->PrintIndent("{\n");
	pFile->IncIndent();
	// set opcode to zero value
	if (m_pReturnVar)
	    m_pReturnVar->WriteSetZero(pFile, pContext);
    if (!m_sErrorFunction.IsEmpty())
        pFile->PrintIndent("%s(%s);\n", (const char*)m_sErrorFunction, (const char*)sResult);
	// set zero value in msgbuffer
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
	bool bUseConstOffset = true;
	pMarshaller->MarshalValue(pFile, this, pContext->GetSizes()->GetOpcodeSize(), 0, 0, bUseConstOffset, pContext);
	delete pMarshaller;
	// set exception if not set already
	VectorElement *pIter = m_pCorbaEnv->GetFirstDeclarator();
	CBEDeclarator *pDecl = m_pCorbaEnv->GetNextDeclarator(pIter);
	pFile->PrintIndent("if (");
	pDecl->WriteName(pFile, pContext);
	if (pDecl->GetStars() > 0)
	    pFile->Print("->");
	else
	    pFile->Print(".");
	pFile->Print("major == CORBA_NO_EXCEPTION)\n");
	pFile->IncIndent();
	// set exception
	pFile->PrintIndent("CORBA_exception_set(");
	if (pDecl->GetStars() == 0)
	    pFile->Print("&");
	pDecl->WriteName(pFile, pContext);
	pFile->Print(",\n");
	pFile->IncIndent();
	pFile->PrintIndent("CORBA_SYSTEM_EXCEPTION,\n");
	pFile->PrintIndent("CORBA_DICE_INTERNAL_IPC_ERROR,\n");
	pFile->PrintIndent("0);\n");
	pFile->DecIndent();
	pFile->DecIndent();
	// return (skip unmarshalling)
	WriteReturn(pFile, pContext);
	pFile->DecIndent();
	pFile->PrintIndent("}\n");
}

/**	\brief write the checking code for opcode exceptions
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * reset any previous exceptions. Must be called before IPC Error check
 */
void CL4BEReplyAnyWaitAnyFunction::WriteExceptionCheck(CBEFile * pFile, CBEContext * pContext)
{
	// set exception if not set already
	VectorElement *pIter = m_pCorbaEnv->GetFirstDeclarator();
	CBEDeclarator *pDecl = m_pCorbaEnv->GetNextDeclarator(pIter);
	pFile->PrintIndent("if (");
	pDecl->WriteName(pFile, pContext);
	if (pDecl->GetStars() > 0)
	    pFile->Print("->");
	else
	    pFile->Print(".");
	pFile->Print("major != CORBA_NO_EXCEPTION)\n");
	pFile->IncIndent();
	// set exception
	pFile->PrintIndent("CORBA_exception_set(");
	if (pDecl->GetStars() == 0)
	    pFile->Print("&");
	pDecl->WriteName(pFile, pContext);
	pFile->Print(",\n");
	pFile->IncIndent();
	pFile->PrintIndent("CORBA_NO_EXCEPTION,\n");
	pFile->PrintIndent("CORBA_DICE_EXCEPTION_NONE,\n");
	pFile->PrintIndent("0);\n");
	pFile->DecIndent();
	pFile->DecIndent();
}

/**	\brief writes the variable declarations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The variable declarations of the reply-and-wait function only contains so-called helper variables.
 * This is the result variable.
 */
void CL4BEReplyAnyWaitAnyFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
   // first call base class
    CBEReplyAnyWaitAnyFunction::WriteVariableDeclaration(pFile, pContext);

    // write result variable
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", (const char *) sResult);
}

/** \brief writes the unmarshalling code for this function
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start unmarshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param pContext the context of the write operation
 *
 * The reply-and-wait function does only unmarshal the opcode. We can print this code by hand. We should use
 * a marshaller anyways.
 */
void CL4BEReplyAnyWaitAnyFunction::WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext)
{
    if (((CL4BEMsgBufferType*)m_pMsgBuffer)->HasReceiveFlexpages())
    {
        // we have to always check if this was a flexpage IPC
        String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
        pFile->PrintIndent("if (l4_ipc_fpage_received(%s))\n", (const char*)sResult);
        WriteFlexpageOpcodePatch(pFile, pContext);  // does indent itself
        pFile->PrintIndent("else\n");
        pFile->IncIndent();
        WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
        pFile->DecIndent();
    }
    else
    {
        WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
    }
}

/** \brief writes a patch to find the opcode if flexpage were received
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This function may receive messages from different function. Because we don't know at
 * compile time, which function sends, we don't know if the message contains a flexpage.
 * If it does the unmarshalled opcode is wrong. Because flexpages have to come first in
 * the message buffer, the opcode cannot be the first parameter. We have to check this
 * condition and get the opcode from behind the flexpages.
 *
 * First we get the number of flexpages of the interface. If it has none, we don't need
 * this extra code. If it has a fixed number of flexpages (either none is sent or one, but
 * if flexpages are sent it is always the same number of flexpages) we can hard code
 * the offset were to find the opcode. If we have different numbers of flexpages (one
 * function may send one, another sends two) we have to use code which can deal with a variable
 * number of flexpages.
 */
void CL4BEReplyAnyWaitAnyFunction::WriteFlexpageOpcodePatch(CBEFile * pFile, CBEContext * pContext)
{
    if (!(((CL4BEMsgBufferType*)m_pMsgBuffer)->HasReceiveFlexpages()))
        return;
    bool bFixedNumberOfFlexpages = true;
    int nNumberOfFlexpages = m_pClass->GetParameterCount(TYPE_FLEXPAGE, bFixedNumberOfFlexpages);
    // if fixed number  (should be true for only one flexpage as well)
    if (bFixedNumberOfFlexpages)
    {
        // the fixed offset (where to find the opcode) is:
        // offset = 8*nMaxNumberOfFlexpages + 8
        bool bUseConstOffset = true;
        pFile->IncIndent();
        WriteUnmarshalReturn(pFile, 8*nNumberOfFlexpages+8, bUseConstOffset, pContext);
        pFile->DecIndent();
    }
    else
    {
        // the variable offset can be determined by searching for the delimiter flexpage
        // which is two zero dwords
        pFile->PrintIndent("{\n");
        pFile->IncIndent();
        // search for delimiter flexpage
        String sTempVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        // init temp var
        pFile->PrintIndent("%s = 0;\n", (const char*)sTempVar);
        pFile->PrintIndent("while ((");
        m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
        pFile->Print("[%s] != 0) && (", (const char*)sTempVar);
        m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
        pFile->Print("[%s+4] != 0)) %s += 8;\n", (const char*)sTempVar, (const char*)sTempVar);
        // now sTempVar points to the delimiter flexpage
        // we have to add another 8 bytes to find the opcode, because UnmarshalReturn does only use
        // temp-var
        pFile->PrintIndent("%s += 8;\n", (const char*)sTempVar);
        // now unmarshal opcode
        bool bUseConstOffset = false;
        WriteUnmarshalReturn(pFile, 0, bUseConstOffset, pContext);
        pFile->DecIndent();
        pFile->PrintIndent("}\n");
    }
}

/** \brief initializes the variables
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * We do not initialize the receive indirect strings, because we assume that theyhave been
 * initialized by the server loop. After that the buffer is handed to the server function.
 * If they intend to use it after the component function is left, they have to copy it.
 */
void CL4BEReplyAnyWaitAnyFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    // call base class
    CBEReplyAnyWaitAnyFunction::WriteVariableInitialization(pFile, pContext);
    if (m_pMsgBuffer)
    {
        // init receive flexpage
        ((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteReceiveFlexpageInitialization(pFile, pContext);
    }
}

/** \brief writes the ipc code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BEReplyAnyWaitAnyFunction::WriteIPC(CBEFile *pFile, CBEContext *pContext)
{
    if (pContext->IsOptionSet(PROGRAM_TRACE_SERVER))
	{
		String sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, false, pContext, 0);
		String sFunc = pContext->GetTraceServerFunc();
		if (sFunc.IsEmpty())
			sFunc = String("printf");
        pFile->PrintIndent("%s(\"reply (dw0=%%x, dw1=%%x)%s\", ",
		                    (const char*)sFunc,
							(sFunc=="LOG")?"":"\\n");
		pFile->Print("(*((%s*)(&(", (const char*)sMWord);
		m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
		pFile->Print("[0])))), ");
		pFile->Print("(*((%s*)(&(", (const char*)sMWord);
		m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
		pFile->Print("[4]))))");
		pFile->Print(");\n");
		// dwords
		pFile->PrintIndent("%s(\"  words: %%d%s\", ",
		                    (const char*)sFunc,
							(sFunc=="LOG")?"":"\\n");
		m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, pContext);
		pFile->Print(".md.dwords);\n");
		// strings
		pFile->PrintIndent("%s(\"  strings: %%d%s\", ",
		                    (const char*)sFunc,
							(sFunc=="LOG")?"":"\\n");
		m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, pContext);
		pFile->Print(".md.strings);\n");
	    // print if we got an fpage
		pFile->PrintIndent("%s(\"  fpage: %%s%s\", (",
		                    (const char*)sFunc,
							(sFunc=="LOG")?"":"\\n");
		m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SIZE, pContext);
		pFile->Print(".md.fpage_received==1)?\"yes\":\"no\");\n");
	}
	CL4BESizes *pSizes = (CL4BESizes*)pContext->GetSizes();
	int nShortWords = pSizes->GetMaxShortIPCSize(DIRECTION_IN) / pSizes->GetSizeOfType(TYPE_MWORD);
    // to determine if we can send a short IPC we have to test the size dope of the message
    pFile->PrintIndent("if ((");
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, pContext);
    pFile->Print(".md.dwords <= %d) && (", nShortWords);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, pContext);
    pFile->Print(".md.strings == 0))\n");
    pFile->IncIndent();
    // if fpage
    pFile->PrintIndent("if (");
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, pContext);
    pFile->Print(".md.fpage_received == 1)\n");
    pFile->IncIndent();
    // short IPC
	WriteShortFlexpageIPC(pFile, pContext);
    // else (fpage)
    pFile->DecIndent();
    pFile->PrintIndent("else\n");
    pFile->IncIndent();
    // !fpage
    WriteShortIPC(pFile, pContext);
    pFile->DecIndent();
    pFile->DecIndent();
    pFile->PrintIndent("else\n");
    pFile->IncIndent();
    // if fpage
    pFile->PrintIndent("if (");
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, pContext);
    pFile->Print(".md.fpage_received == 1)\n");
    pFile->IncIndent();
    // long IPC
	WriteLongFlexpageIPC(pFile, pContext);
    // else (fpage)
    pFile->DecIndent();
    pFile->PrintIndent("else\n");
    pFile->IncIndent();
    // ! fpage
	WriteLongIPC(pFile, pContext);
    pFile->DecIndent();
    pFile->DecIndent();
}

/** \brief write the ip code with a short msg reply containing a flexpage
 *  \param pFile the file to write to
 *  \param pContext the contet of the write operation
 */
void CL4BEReplyAnyWaitAnyFunction::WriteShortFlexpageIPC(CBEFile *pFile, CBEContext *pContext)
{
    assert(m_pComm);
	((CL4BEIPC*)m_pComm)->WriteReplyAndWait(pFile, this, true, true, pContext);
}

/** \brief write the ipc code with a short msg reply
 *  \param pFile the file to write to
 *  \param pContext the contet of the write operation
 */
void CL4BEReplyAnyWaitAnyFunction::WriteShortIPC(CBEFile *pFile, CBEContext *pContext)
{
    assert(m_pComm);
	((CL4BEIPC*)m_pComm)->WriteReplyAndWait(pFile, this, false, true, pContext);
}

/** \brief write the ipc code with a long msg containing flexpages
 *  \param pFile the file to write to
 *  \param pContext the contet of the write operation
 */
void CL4BEReplyAnyWaitAnyFunction::WriteLongFlexpageIPC(CBEFile *pFile, CBEContext *pContext)
{
    assert(m_pComm);
	((CL4BEIPC*)m_pComm)->WriteReplyAndWait(pFile, this, true, false, pContext);
}

/** \brief write ipc code with a long msg
 *  \param pFile the file to write to
 *  \param pContext the contet of the write operation
 */
void CL4BEReplyAnyWaitAnyFunction::WriteLongIPC(CBEFile *pFile, CBEContext *pContext)
{
    assert(m_pComm);
	((CL4BEIPC*)m_pComm)->WriteReplyAndWait(pFile, this, false, false, pContext);
}
