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
#include "be/l4/L4BEReplyFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEClassFactory.h"
#include "be/l4/L4BEIPC.h"

#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEType.h"

#include "TypeSpec-Type.h"
#include "fe/FEAttribute.h"

IMPLEMENT_DYNAMIC(CL4BEReplyFunction);

CL4BEReplyFunction::CL4BEReplyFunction()
 : CBEReplyFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4BEReplyFunction, CBEReplyFunction);
}

CL4BEReplyFunction::CL4BEReplyFunction(CL4BEReplyFunction& src)
: CBEReplyFunction(src)
{
    IMPLEMENT_DYNAMIC_BASE(CL4BEReplyFunction, CBEReplyFunction);
}

/** destroy the object */
CL4BEReplyFunction::~CL4BEReplyFunction()
{
}

/**	\brief writes the invocation of the message transfer
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * In L4 this is a send. Do not set size dope, because the size dope is set by
 * the server (wait-any function).
 */
void CL4BEReplyFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    int nSendDirection = GetSendDirection();
	bool bHasSizeIsParams = (GetParameterCount(ATTR_SIZE_IS, ATTR_REF, nSendDirection) > 0) ||
	    (GetParameterCount(ATTR_LENGTH_IS, ATTR_REF, nSendDirection) > 0);
    // set size and send dopes
    ((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteSendDopeInit(pFile, nSendDirection, bHasSizeIsParams, pContext);

    // invocate
    WriteIPC(pFile, pContext);
    WriteIPCErrorCheck(pFile, pContext);
}


/**	\brief tests if this IPC was successful
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The IPC error check tests the result code of the IPC, whether the reply operation had any errors.
 *
 * \todo: Do we want to block the server, waiting for one client, which might not respond?
 */
void CL4BEReplyFunction::WriteIPCErrorCheck(CBEFile * pFile, CBEContext * pContext)
{
    if (!m_sErrorFunction.IsEmpty())
    {
        String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
        pFile->PrintIndent("/* test for IPC errors */\n");
        pFile->PrintIndent("if (L4_IPC_IS_ERROR(%s))\n", (const char *) sResult);
        pFile->IncIndent();
        pFile->PrintIndent("%s(%s);\n", (const char*)m_sErrorFunction, (const char*)sResult);
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
	((CL4BEIPC*)m_pComm)->WriteSend(pFile, this, pContext);
}

/** \brief decides whether two parameters should be exchanged during sort (moving 1st behind 2nd)
 *  \param pPrecessor the 1st parameter
 *  \param pSuccessor the 2nd parameter
 *  \param pContext the context of the sorting
 *  \return true if parameters should be exchanged
 */
bool CL4BEReplyFunction::DoSortParameters(CBETypedDeclarator * pPrecessor, CBETypedDeclarator * pSuccessor, CBEContext * pContext)
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
    String sReturn = pContext->GetNameFactory()->GetReturnVariable(pContext);
    if (pPrecessor->FindDeclarator(sReturn))
        return false;
    // if successor is return variable (should not occur) move it forward
    if (pSuccessor->FindDeclarator(sReturn))
        return true;
    // nothing special, return base class' decision
    return CBEReplyFunction::DoSortParameters(pPrecessor, pSuccessor, pContext);
}

/**	\brief writes the variable declarations of this function
 *	\param pFile the file to write to
 *	\param pContext the context of the write operation
 *
 * The variable declarations of the reply and receive function only contains so-called helper variables.
 * This is the result variable and marshalling helpers (offset or similar).
 */
void CL4BEReplyFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // first call base class
    CBEReplyFunction::WriteVariableDeclaration(pFile, pContext);

    // write result variable
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0};\n", (const char *) sResult);
}

/** \brief init message buffer size dope
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BEReplyFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    CBEReplyFunction::WriteVariableInitialization(pFile, pContext);
    ((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteSizeDopeInit(pFile, pContext);
}
