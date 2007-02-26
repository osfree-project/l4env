/**
 *    \file    dice/src/be/l4/L4BECallFunction.cpp
 *    \brief   contains the implementation of the class CL4BECallFunction
 *
 *    \date    02/07/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2004
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
#include "be/l4/L4BEMsgBufferType.h"
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
#include "Attribute-Type.h"

CL4BECallFunction::CL4BECallFunction()
{
}

CL4BECallFunction::CL4BECallFunction(CL4BECallFunction & src):CBECallFunction(src)
{
}

/**    \brief destructor of target class */
CL4BECallFunction::~CL4BECallFunction()
{

}

/**    \brief writes the variable declarations of this function
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * The L4 implementation also includes the result variable for the IPC
 */
void CL4BECallFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // first call base class
    CBECallFunction::WriteVariableDeclaration(pFile, pContext);

    // write result variable
    string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    *pFile << "\tl4_msgdope_t " << sResult << " = { msgdope: 0 };\n";
}

/**    \brief writes the invocation of the message transfer
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * Because this is the call function, we can use the IPC call of L4.
 */
void CL4BECallFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    VERBOSE("CL4BECallFunction::WriteInvocation(%s) called\n",
        GetName().c_str());

    // set dopes
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->WriteInitialization(pFile, TYPE_MSGDOPE_SEND, GetSendDirection(), pContext);
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
        string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
        pFile->DecIndent();
        pFile->PrintIndent("} while ((L4_IPC_ERROR(%s) == L4_IPC_SEABORTED) || (L4_IPC_ERROR(%s) == L4_IPC_SECANCELED));\n",
                            sResult.c_str(), sResult.c_str());
    }
    // check for errors
    WriteIPCErrorCheck(pFile, pContext);

    VERBOSE("CL4BECallFunction::WriteInvocation(%s) finished\n",
        GetName().c_str());
}

/** \brief writes the marshaling of the message
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start with marshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param pContext the context of the write operation
 */
void CL4BECallFunction::WriteMarshalling(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    VERBOSE("CL4BECallFunction::WriteMarshalling(%s) called\n",
        GetName().c_str());

    // if we have send flexpages marshal them first
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    nStartOffset += pMarshaller->Marshal(pFile, this, TYPE_FLEXPAGE, 0/*all*/, nStartOffset, bUseConstOffset, pContext);
    // marshal opcode
    if (!FindAttribute(ATTR_NOOPCODE))
        nStartOffset += WriteMarshalOpcode(pFile, nStartOffset, bUseConstOffset, pContext);
    // marshal rest
    pMarshaller->Marshal(pFile, this, -TYPE_FLEXPAGE, 0/*all*/, nStartOffset, bUseConstOffset, pContext);
    delete pMarshaller;

    VERBOSE("CL4BECallFunction::WriteMarshalling(%s) finished\n",
        GetName().c_str());
}

/**    \brief write the error checking code for the IPC
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * If there was an IPC error, we write this into the environment.
 * This can be done by checking if there was an error, then sets the major value
 * to CORBA_SYSTEM_EXCEPTION and then sets the ipc_error value to
 * L4_IPC_ERROR(result).
 */
void CL4BECallFunction::WriteIPCErrorCheck(CBEFile * pFile, CBEContext * pContext)
{
    string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    vector<CBEDeclarator*>::iterator iterCE = m_pCorbaEnv->GetFirstDeclarator();
    CBEDeclarator *pDecl = *iterCE;

    *pFile << "\tif (L4_IPC_IS_ERROR(" << sResult << "))\n" <<
              "\t{\n";
    pFile->IncIndent();
    // env.major = CORBA_SYSTEM_EXCEPTION;
    // env.repos_id = DICE_IPC_ERROR;
    *pFile << "\tCORBA_exception_set(";
    if (pDecl->GetStars() == 0)
        *pFile << "&";
    pDecl->WriteName(pFile, pContext);
    *pFile << ",\n";
    pFile->IncIndent();
    *pFile << "\tCORBA_SYSTEM_EXCEPTION,\n" <<
              "\tCORBA_DICE_EXCEPTION_IPC_ERROR,\n" <<
              "\t0);\n";
    pFile->DecIndent();
    // env.ipc_error = L4_IPC_ERROR(result);
    *pFile << "\t";
    pDecl->WriteName(pFile, pContext);
    if (pDecl->GetStars())
        *pFile << "->";
    else
        *pFile << ".";
    *pFile << "_p.ipc_error = L4_IPC_ERROR(" << sResult << ");\n";
    // return
    WriteReturn(pFile, pContext);
    // close }
    pFile->DecIndent();
    *pFile << "\t}\n";
}

/** \brief initializes message size dopes
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BECallFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    CBECallFunction::WriteVariableInitialization(pFile, pContext);
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (pContext->IsOptionSet(PROGRAM_ZERO_MSGBUF))
        pMsgBuffer->WriteSetZero(pFile, pContext);
    pMsgBuffer->WriteInitialization(pFile, TYPE_MSGDOPE_SIZE, 0, pContext);
    pMsgBuffer->WriteInitialization(pFile, TYPE_REFSTRING, GetReceiveDirection(), pContext);
    pMsgBuffer->WriteInitialization(pFile, TYPE_FLEXPAGE, GetReceiveDirection(), pContext);
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
    VERBOSE("CL4BECallFunction::WriteUnmarshalling(%s) called\n", GetName().c_str());

    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    // check flexpages
    int nFlexSize = 0;
    if (pMsgBuffer->GetCount(TYPE_FLEXPAGE, GetReceiveDirection()) > 0)
    {
        // we have to always check if this was a flexpage IPC
        string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
        pFile->PrintIndent("if (l4_ipc_fpage_received(%s))\n", sResult.c_str());
        nFlexSize = WriteFlexpageReturnPatch(pFile, nStartOffset, bUseConstOffset, pContext);
        pFile->PrintIndent("else\n");
        pFile->PrintIndent("{\n");
        // since we should always get receive flexpages we expect, this is the error case
        // therefore we do not add this size to the offset, since we cannot trust it
        pFile->IncIndent();
        // unmarshal exception first
        WriteUnmarshalException(pFile, nStartOffset, bUseConstOffset, pContext);
        // test for exception and return
        WriteExceptionCheck(pFile, pContext); // resets exception (in env)
        // even though there are no flexpages send, the marshalling didn't know if the flexpages
        // are valid. Thus it reserved space for them. Therefore we have to apply the opcode
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
        WriteExceptionCheck(pFile, pContext); // resets exception (in env)
        // unmarshal return
        nStartOffset += WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
    }
    // unmarshal rest (skip CBECallFunction)
    // unmarshals flexpages if there are any...
    CBEOperationFunction::WriteUnmarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
    // add the size of the flexpage unmarshalling to offset
    nStartOffset += nFlexSize;

    VERBOSE("CL4BECallFunction::WriteUnmarshalling(%s) finished\n", GetName().c_str());
}

/** \brief writes the patch code for the return variable
 *  \param pFile the file to write to
 *  \param nStartOffset the offset in the message buffer where to start
 *  \param bUseConstOffset true if nStartOffset can be used
 *  \param pContext the context of the write operation
 */
int CL4BECallFunction::WriteFlexpageReturnPatch(CBEFile *pFile, int nStartOffset, bool & bUseConstOffset, CBEContext *pContext)
{
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    // if we have no flexpages do directly unmarshal return variable
    if (pMsgBuffer->GetCount(TYPE_FLEXPAGE, GetReceiveDirection()) == 0)
        return 0;
    // now check for flexpages
    bool bFixedNumberOfFlexpages = true;
    int nNumberOfFlexpages = m_pClass->GetParameterCount(TYPE_FLEXPAGE, bFixedNumberOfFlexpages);
    int nFlexpageSize = pContext->GetSizes()->GetSizeOfType(TYPE_FLEXPAGE);
    int nReturnSize = 0;
    // if fixed number  (should be true for only one flexpage as well)
    if (bFixedNumberOfFlexpages)
    {
        // the fixed offset (where to find the return value) is:
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
    string sTempVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
    // init temp var
    pFile->PrintIndent("%s = 0;\n", sTempVar.c_str());
    pFile->PrintIndent("while ((");
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[%s] != 0) && (", sTempVar.c_str());
    pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
    pFile->Print("[%s+4] != 0)) %s += %d;\n", sTempVar.c_str(), sTempVar.c_str(), nFlexpageSize);
    // now sTempVar points to the delimiter flexpage
    // we have to add another 8 bytes to find the return value,
    // because UnmarshalReturn does only use temp-var
    pFile->PrintIndent("%s += %d;\n", sTempVar.c_str(), nFlexpageSize);
    // now unmarshal return value
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
bool
CL4BECallFunction::DoExchangeParameters(CBETypedDeclarator * pPrecessor,
    CBETypedDeclarator * pSuccessor,
    CBEContext *pContext)
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
    // we have to
    // move the indirect strings to the end. (Successor is not indirect
    // string, but parameter is, then we pass)
    if (  pPrecessor->IsString() && pPrecessor->FindAttribute(ATTR_REF) &&
        !(pSuccessor->IsString() && pSuccessor->FindAttribute(ATTR_REF)))
        return true;
    // if precessor is not string, but successor is we explicetly deny exchange
    // otherwise base class might decide on type sizes
    if (!(pPrecessor->IsString() && pPrecessor->FindAttribute(ATTR_REF)) &&
            pSuccessor->IsString() && pSuccessor->FindAttribute(ATTR_REF))
        return false;
    // if none of our special rules fits, go back for original ones
    return CBECallFunction::DoExchangeParameters(pPrecessor, pSuccessor, pContext);
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
    m_pComm->WriteCall(pFile, this, pContext);
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \param pContext the context of this calculation
 *  \return the size of the parameters
 *
 * If we recv flexpages, remove the exception size again, since either the
 * flexpage or the exception is sent.
 */
int CL4BECallFunction::GetSize(int nDirection, CBEContext *pContext)
{
    // get base class' size
    int nSize = CBECallFunction::GetSize(nDirection, pContext);
    if ((nDirection & DIRECTION_OUT) &&
        !FindAttribute(ATTR_NOEXCEPTIONS) &&
        (GetParameterCount(TYPE_FLEXPAGE, DIRECTION_OUT) > 0))
        nSize -= pContext->GetSizes()->GetExceptionSize();
    return nSize;
}

/** \brief calculates the size of the function's fixed-sized parameters
 *  \param nDirection the direction to count
 *  \param pContext the context of this calculation
 *  \return the size of the parameters
 *
 * If we recv flexpages, remove the exception size again, since either the
 * flexpage or the exception is sent.
 */
int CL4BECallFunction::GetFixedSize(int nDirection, CBEContext *pContext)
{
    int nSize = CBECallFunction::GetFixedSize(nDirection, pContext);
    if ((nDirection & DIRECTION_OUT) &&
        !FindAttribute(ATTR_NOEXCEPTIONS) &&
        (GetParameterCount(TYPE_FLEXPAGE, DIRECTION_OUT) > 0))
        nSize -= pContext->GetSizes()->GetExceptionSize();
    return nSize;
}
