/**
 *	\file	dice/src/be/l4/L4BECallFunction.cpp
 *	\brief	contains the implementation of the class CL4BECallFunction
 *
 *	\date	02/07/2002
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

#include "be/l4/L4BECallFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEClassFactory.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEOpcodeType.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEMarshaller.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BESizes.h"
#include "be/l4/L4BEIPC.h"

#include "TypeSpec-Type.h"
#include "fe/FEAttribute.h"

IMPLEMENT_DYNAMIC(CL4BECallFunction);

CL4BECallFunction::CL4BECallFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4BECallFunction, CBECallFunction);
}

CL4BECallFunction::CL4BECallFunction(CL4BECallFunction & src):CBECallFunction(src)
{
    IMPLEMENT_DYNAMIC_BASE(CL4BECallFunction, CBECallFunction);
}

/**	\brief destructor of target class */
CL4BECallFunction::~CL4BECallFunction()
{

}

/**	\brief writes the variable declarations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The L4 implementation also includes the result variable for the IPC
 */
void CL4BECallFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // first call base class
    CBECallFunction::WriteVariableDeclaration(pFile, pContext);

    // write result variable
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", (const char *) sResult);
}

/**	\brief writes the invocation of the message transfer
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * Because this is the call function, we can use the IPC call of L4.
 */
void CL4BECallFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    int nSendDirection = GetSendDirection();
	bool bHasSizeIsParams = (GetParameterCount(ATTR_SIZE_IS, ATTR_REF, nSendDirection) > 0) ||
	    (GetParameterCount(ATTR_LENGTH_IS, ATTR_REF, nSendDirection) > 0) ||
		(GetParameterCount(ATTR_STRING, ATTR_REF, nSendDirection) > 0);
    // set dopes
    ((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteSendDopeInit(pFile, nSendDirection, bHasSizeIsParams, pContext);
    // invocate
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
	// check for errors
    WriteIPCErrorCheck(pFile, pContext);
}

/** \brief writes the marshaling of the message
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start with marshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param pContext the context of the write operation
 */
void CL4BECallFunction::WriteMarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
	// if we have send flexpages marshal them first
	CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
	nStartOffset += pMarshaller->Marshal(pFile, this, TYPE_FLEXPAGE, 0/*all*/, nStartOffset, bUseConstOffset, pContext);
	// marshal opcode
	nStartOffset += WriteMarshalOpcode(pFile, nStartOffset, bUseConstOffset, pContext);
	// marshal rest
	pMarshaller->Marshal(pFile, this, -TYPE_FLEXPAGE, 0/*all*/, nStartOffset, bUseConstOffset, pContext);
	delete pMarshaller;
}

/**	\brief write the error checking code for the IPC
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * If there was an IPC error, we write this into the environment.
 * This can be done by checking if there was an error, then sets the major value
 * to CORBA_SYSTEM_EXCEPTION and then sets the ipc_error value to
 * L4_IPC_ERROR(result).
 */
void CL4BECallFunction::WriteIPCErrorCheck(CBEFile * pFile, CBEContext * pContext)
{
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    VectorElement *pIter = m_pCorbaEnv->GetFirstDeclarator();
    CBEDeclarator *pDecl = m_pCorbaEnv->GetNextDeclarator(pIter);

    pFile->PrintIndent("if (L4_IPC_IS_ERROR(%s))\n", (const char*)sResult);
    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    // env.major = CORBA_SYSTEM_EXCEPTION;
	// env.repos_id = DICE_IPC_ERROR;
    pFile->PrintIndent("CORBA_exception_set(");
    if (pDecl->GetStars() == 0)
	    pFile->Print("&");
    pDecl->WriteName(pFile, pContext);
    pFile->Print(",\n");
	pFile->IncIndent();
	pFile->PrintIndent("CORBA_SYSTEM_EXCEPTION,\n");
	pFile->PrintIndent("CORBA_DICE_EXCEPTION_IPC_ERROR,\n");
	pFile->PrintIndent("0);\n");
	pFile->DecIndent();
    // env.ipc_error = L4_IPC_ERROR(result);
    pFile->PrintIndent("");
    pDecl->WriteName(pFile, pContext);
    if (pDecl->GetStars())
        pFile->Print("->");
    else
        pFile->Print(".");
    pFile->Print("ipc_error = L4_IPC_ERROR(%s);\n", (const char*)sResult);
	// return
	WriteReturn(pFile, pContext);
    // close }
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
}

/** \brief initializes message size dopes
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BECallFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    CBECallFunction::WriteVariableInitialization(pFile, pContext);
	if (pContext->IsOptionSet(PROGRAM_ZERO_MSGBUF))
		((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteSetZero(pFile, pContext);
    ((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteSizeDopeInit(pFile, pContext);
    ((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteReceiveIndirectStringInitialization(pFile, pContext);
}

/** \brief write L4 specific unmarshalling code
 *  \param pFile the file to write to
 *  \param nStartOffset the offset in the message buffer where to start
 *  \param bUseConstOffset true if nStartOffset can be used
 *  \param pContext the context of the write operation
 *
 * We have to check for any received flexpages and fix the offset for any return values.
 */
void CL4BECallFunction::WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext)
{
	// check flexpages
	int nFlexSize = 0;
    if (((CL4BEMsgBufferType*) m_pMsgBuffer)->HasReceiveFlexpages())
    {
        // we have to always check if this was a flexpage IPC
        String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
        pFile->PrintIndent("if (l4_ipc_fpage_received(%s))\n", (const char*)sResult);
        nFlexSize = WriteFlexpageReturnPatch(pFile, nStartOffset, bUseConstOffset, pContext);
        pFile->PrintIndent("else\n");
		pFile->PrintIndent("{\n");
        // since we should always get receive flexpages we expect, this is the error case
        // therefore we do not add this size to the offset, since we cannot trust it
        pFile->IncIndent();
		// unmarshal exception first
		WriteUnmarshalException(pFile, nStartOffset, bUseConstOffset, pContext);
		// test for exception and return
		WriteExceptionCheck(pFile, pContext);
		// unmarshal return
		//WriteUnmarshalReturn(pFile, nStartOffset+nExceptionSize, bUseConstOffset, pContext);
		// even though there are no flexpages send, the marshalling didn't know if the flexpages
		// are valid. Though it reserved space for them. Therefore we have to apply the opcode
		// path here as well. The return code might return the reason for the invalid flexpages.
		pFile->DecIndent();
        nFlexSize = WriteFlexpageReturnPatch(pFile, nStartOffset, bUseConstOffset, pContext);
		pFile->IncIndent();
		// return
		// because this means error and we want to skip the rest of the unmarshalling
		WriteReturn(pFile, pContext);
        pFile->DecIndent();
		pFile->PrintIndent("}\n");
    }
    else
    {
		// unmarshal exception first
		nStartOffset += WriteUnmarshalException(pFile, nStartOffset, bUseConstOffset, pContext);
		// test for exception and return
		WriteExceptionCheck(pFile, pContext);
	    // unmarshal return
        nStartOffset += WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
    }
    // unmarshal rest (skip CBECallFunction)
	// unmarshals flexpages if there are any...
    CBEOperationFunction::WriteUnmarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
    // add the size of the flexpage unmarshalling to offset
    nStartOffset += nFlexSize;
}

/** \brief writes the patch code for the return variable
 *  \param pFile the file to write to
 *  \param nStartOffset the offset in the message buffer where to start
 *  \param bUseConstOffset true if nStartOffset can be used
 *  \param pContext the context of the write operation
 */
int CL4BECallFunction::WriteFlexpageReturnPatch(CBEFile *pFile, int nStartOffset, bool & bUseConstOffset, CBEContext *pContext)
{
    // if we have no flexpages do directly unmarshal return variable
    if (!(((CL4BEMsgBufferType*)m_pMsgBuffer)->HasReceiveFlexpages()))
        return 0;
    // now check for flexpages
    bool bFixedNumberOfFlexpages = true;
    int nNumberOfFlexpages = m_pClass->GetParameterCount(TYPE_FLEXPAGE, bFixedNumberOfFlexpages);
    int nFlexpageSize = pContext->GetSizes()->GetSizeOfType(TYPE_FLEXPAGE);
    int nReturnSize = 0;
    // if fixed number  (should be true for only one flexpage as well)
    if (bFixedNumberOfFlexpages)
    {
        // the fixed offset (where to find the opcode) is:
        // offset = 8*nMaxNumberOfFlexpages + 8
        pFile->IncIndent();
        nReturnSize = WriteUnmarshalReturn(pFile, nStartOffset + nFlexpageSize*(nNumberOfFlexpages+1), bUseConstOffset, pContext);
        pFile->DecIndent();
        return nReturnSize;
    }

    // this is the real patch

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
    pFile->Print("[%s+4] != 0)) %s += %d;\n", (const char*)sTempVar, (const char*)sTempVar, nFlexpageSize);
    // now sTempVar points to the delimiter flexpage
    // we have to add another 8 bytes to find the opcode, because UnmarshalReturn does only use
    // temp-var
    pFile->PrintIndent("%s += %d;\n", (const char*)sTempVar, nFlexpageSize);
    // now unmarshal opcode
    bUseConstOffset = false;
    nReturnSize = WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
    return nReturnSize;
}

/** \brief decides whether two parameters should be exchanged during sort (moving 1st behind 2nd)
 *  \param pPrecessor the 1st parameter
 *  \param pSuccessor the 2nd parameter
 *  \param pContext the context of the sorting
 *  \return true if parameters should be exchanged
 */
bool CL4BECallFunction::DoSortParameters(CBETypedDeclarator * pPrecessor, CBETypedDeclarator * pSuccessor, CBEContext * pContext)
{
    // check if the parameter is not a flexpage and the successor is one
    // we do not test for TYPE_RCV_FLEXPAGE, because this type is reserved
    // for member of message buffer which declares receive window
    if(!pPrecessor->GetType()->IsOfType(TYPE_FLEXPAGE) &&
        pSuccessor->GetType()->IsOfType(TYPE_FLEXPAGE))
        return true;
    // if first is flexpage and second is not, we prohibit an exchange
    // explicetly
    if ( pPrecessor->GetType()->IsOfType(TYPE_FLEXPAGE) &&
        !pSuccessor->GetType()->IsOfType(TYPE_FLEXPAGE))
        return false;
    // if we have a variable sized message buffer, we have to
    // move the indirect strings to the end. (Successor is not indirect
    // string, but parameter is, then we pass)
    if (GetMessageBuffer()->IsVariableSized())
    {
        if (  pPrecessor->IsString() && pPrecessor->FindAttribute(ATTR_REF) &&
            !(pSuccessor->IsString() && pSuccessor->FindAttribute(ATTR_REF)))
            return true;
        // if precessor is not string, but successor is we explicetly deny exchange
        // otherwise base class might decide on type sizes
        if (!(pPrecessor->IsString() && pPrecessor->FindAttribute(ATTR_REF)) &&
              pSuccessor->IsString() && pSuccessor->FindAttribute(ATTR_REF))
            return false;
    }
    // if none of our special rules fits, go back for original ones
    return CBECallFunction::DoSortParameters(pPrecessor, pSuccessor, pContext);
}

/** \brief writes the ipc call
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This implementation writes the L4 V2 IPC code. In Dresden we use a X0-adaption
 * C-binding to invoke IPC calls with the L4 V2 C calling interface. Therefore
 * can we use this IPC call here.
 */
void CL4BECallFunction::WriteIPC(CBEFile* pFile, CBEContext* pContext)
{
    assert(m_pComm);
	((CL4BEIPC*)m_pComm)->WriteCall(pFile, this, pContext);
}
