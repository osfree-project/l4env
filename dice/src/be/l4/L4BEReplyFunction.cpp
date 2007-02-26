/**
 *    \file    dice/src/be/l4/L4BEReplyFunction.cpp
 *    \brief   contains the implementation of the class CL4BEReplyFunction
 *
 *    \date    02/07/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004
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
#include "be/l4/L4BEReplyFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEClassFactory.h"
#include "be/l4/L4BEIPC.h"

#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BEDeclarator.h"

#include "TypeSpec-Type.h"
#include "Attribute-Type.h"

CL4BEReplyFunction::CL4BEReplyFunction()
 : CBEReplyFunction()
{
}

CL4BEReplyFunction::CL4BEReplyFunction(CL4BEReplyFunction& src)
: CBEReplyFunction(src)
{
}

/** destroy the object */
CL4BEReplyFunction::~CL4BEReplyFunction()
{
}

/**    \brief writes the invocation of the message transfer
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * In L4 this is a send. Do not set size dope, because the size dope is set by
 * the server (wait-any function).
 */
void CL4BEReplyFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    // set size and send dopes
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->WriteInitialization(pFile, TYPE_MSGDOPE_SEND, GetSendDirection(), pContext);

    // invocate
    WriteIPC(pFile, pContext);
    WriteIPCErrorCheck(pFile, pContext);
}


/**    \brief tests if this IPC was successful
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * The IPC error check tests the result code of the IPC, whether the reply operation had any errors.
 *
 * \todo: Do we want to block the server, waiting for one client, which might not respond?
 */
void CL4BEReplyFunction::WriteIPCErrorCheck(CBEFile * pFile, CBEContext * pContext)
{
    if (!m_sErrorFunction.empty())
    {
        string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
        pFile->PrintIndent("/* test for IPC errors */\n");
        pFile->PrintIndent("if (L4_IPC_IS_ERROR(%s))\n", sResult.c_str());
        pFile->IncIndent();
        *pFile << "\t" << m_sErrorFunction << "(" << sResult << ", ";
        WriteCallParameter(pFile, m_pCorbaEnv, pContext);
        *pFile << ");\n";
        pFile->DecIndent();
    }
}

/** \brief writes the ipc code for this function
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BEReplyFunction::WriteIPC(CBEFile *pFile, CBEContext *pContext)
{
    assert(m_pComm);
    m_pComm->WriteReply(pFile, this, pContext);
}

/** \brief decides whether two parameters should be exchanged during sort
 *  \param pPrecessor the 1st parameter
 *  \param pSuccessor the 2nd parameter
 *    \param pContext the context of the sorting
 *  \return true if parameters 1st is smaller than 2nd
 */
bool
CL4BEReplyFunction::DoExchangeParameters(CBETypedDeclarator * pPrecessor,
    CBETypedDeclarator * pSuccessor,
    CBEContext *pContext)
{
    if (!(pPrecessor->GetType()->IsOfType(TYPE_FLEXPAGE)) &&
        pSuccessor->GetType()->IsOfType(TYPE_FLEXPAGE))
        return true;
    // if first is flexpage and second is not, we prohibit an exchange
    // explicetly
    if ( pPrecessor->GetType()->IsOfType(TYPE_FLEXPAGE) &&
        !pSuccessor->GetType()->IsOfType(TYPE_FLEXPAGE))
        return false;
    // if the 1st parameter is the return variable, we cannot exchange it, because
    // we make assumptions about its position in the message buffer
    if (m_pReturnVar)
    {
        vector<CBEDeclarator*>::iterator iterRet = m_pReturnVar->GetFirstDeclarator();
        CBEDeclarator *pDecl = *iterRet;

        if (pPrecessor->FindDeclarator(pDecl->GetName()))
            return false;
        // if successor is return variable (should not occur) move it forward
        if (pSuccessor->FindDeclarator(pDecl->GetName()))
            return true;
    }
    // nothing special, return base class' decision
    return CBEReplyFunction::DoExchangeParameters(pPrecessor, pSuccessor, pContext);
}

/**    \brief writes the variable declarations of this function
 *    \param pFile the file to write to
 *    \param pContext the context of the write operation
 *
 * The variable declarations of the reply and receive function only contains so-called helper variables.
 * This is the result variable and marshalling helpers (offset or similar).
 */
void CL4BEReplyFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // first call base class
    CBEReplyFunction::WriteVariableDeclaration(pFile, pContext);

    // write result variable
    string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0};\n", sResult.c_str());

    // we might need the offset variables if we transmit [ref] attributes,
    // because strings are found in message buffer by offset calculation
    // if message buffer is at server side.
    if (!HasVariableSizedParameters() && !HasArrayParameters() &&
        FindParameterAttribute(ATTR_REF))
    {
        string sTmpVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        string sOffsetVar = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        *pFile << "\tunsigned " << sTmpVar << " __attribute__ ((unused));\n";
        *pFile << "\tunsigned " << sOffsetVar << " __attribute__ ((unused));\n";
    }
}

/** \brief init message buffer size dope
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BEReplyFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    CBEReplyFunction::WriteVariableInitialization(pFile, pContext);
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->WriteInitialization(pFile, TYPE_MSGDOPE_SIZE, 0, pContext);
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \param pContext the context of this calculation
 *  \return the size of the parameters
 *
 * If we recv flexpages, remove the exception size again, since either the
 * flexpage or the exception is sent.
 */
int CL4BEReplyFunction::GetSize(int nDirection, CBEContext *pContext)
{
    // get base class' size
    int nSize = CBEReplyFunction::GetSize(nDirection, pContext);
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
int CL4BEReplyFunction::GetFixedSize(int nDirection, CBEContext *pContext)
{
    int nSize = CBEReplyFunction::GetFixedSize(nDirection, pContext);
    if ((nDirection & DIRECTION_OUT) &&
        !FindAttribute(ATTR_NOEXCEPTIONS) &&
        (GetParameterCount(TYPE_FLEXPAGE, DIRECTION_OUT) > 0))
        nSize -= pContext->GetSizes()->GetExceptionSize();
    return nSize;
}
