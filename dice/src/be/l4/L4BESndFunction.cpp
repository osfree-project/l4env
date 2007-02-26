/**
 *    \file    dice/src/be/l4/L4BESndFunction.cpp
 *    \brief   contains the implementation of the class CL4BESndFunction
 *
 *    \date    06/01/2002
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

#include "be/l4/L4BESndFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEClassFactory.h"
#include "be/BEContext.h"
#include "be/BEOpcodeType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEMarshaller.h"
#include "be/BEClient.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BESizes.h"
#include "be/l4/L4BEIPC.h"

#include "TypeSpec-Type.h"
#include "Attribute-Type.h"

CL4BESndFunction::CL4BESndFunction()
{
}

/** destructs the send function class */
CL4BESndFunction::~CL4BESndFunction()
{
}

/** \brief writes the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BESndFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // first call base class
    CBESndFunction::WriteVariableDeclaration(pFile, pContext);

    // write result variable
    string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", sResult.c_str());
}

/** \brief writes the invocation code
 *  \param pFile the file tow rite to
 *  \param pContext the context of the write opeation
 */
void CL4BESndFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    // after marshalling set the message dope
    pMsgBuffer->WriteInitialization(pFile, TYPE_MSGDOPE_SEND, GetSendDirection(), pContext);
    // invocate
    if (!pContext->IsOptionSet(PROGRAM_NO_SEND_CANCELED_CHECK))
    {
        // sometimes it's possible to abort a call of a client.
        // but the client wants his call made, so we try until
        // the call completes
        *pFile << "\tdo\n";
        *pFile << "\t{\n";
        pFile->IncIndent();
    }
    WriteIPC(pFile, pContext);
    if (!pContext->IsOptionSet(PROGRAM_NO_SEND_CANCELED_CHECK))
    {
        // now check if call has been canceled
        string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
        pFile->DecIndent();
        *pFile << "\t} while ((L4_IPC_ERROR(" << sResult <<
            ") == L4_IPC_SEABORTED) ||\n";
        *pFile << "\t         (L4_IPC_ERROR(" << sResult <<
            ") == L4_IPC_SECANCELED));\n";
    }
    WriteIPCErrorCheck(pFile, pContext);
}

/** \brief writes the IPC error check
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * \todo write IPC error checking
 */
void CL4BESndFunction::WriteIPCErrorCheck(CBEFile * pFile, CBEContext * pContext)
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

/** \brief init message buffer size dope
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BESndFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    CBESndFunction::WriteVariableInitialization(pFile, pContext);
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->WriteInitialization(pFile, TYPE_MSGDOPE_SIZE, 0, pContext);
}

/** \brief decides whether two parameters should be exchanged during sort
 *  \param pPrecessor the 1st parameter
 *  \param pSuccessor the 2nd parameter
 *    \param pContext the context of the sorting
 *  \return true if parameters pPrecessor is smaller than pSuccessor
 */
bool
CL4BESndFunction::DoExchangeParameters(CBETypedDeclarator * pPrecessor,
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
    return CBESndFunction::DoExchangeParameters(pPrecessor, pSuccessor, pContext);
}

/** \brief write the IPC code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BESndFunction::WriteIPC(CBEFile *pFile, CBEContext *pContext)
{
    assert(m_pComm);
    m_pComm->WriteSend(pFile, this, pContext);
}
