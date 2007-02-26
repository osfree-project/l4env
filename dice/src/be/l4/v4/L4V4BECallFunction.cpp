/**
 *    \file    dice/src/be/l4/v4/L4V4BECallFunction.cpp
 *    \brief    contains the implementation of the class CL4V4BECallFunction
 *
 *    \date    01/08/2004
 *    \author    Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
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

#include "be/l4/v4/L4V4BECallFunction.h"
#include "be/l4/v4/L4V4BENameFactory.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"

CL4V4BECallFunction::CL4V4BECallFunction()
 : CL4BECallFunction()
{
}

/** destroy the object of this class */
CL4V4BECallFunction::~CL4V4BECallFunction()
{
}

/** \brief writes the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * For V4 we only declare mr0 and the message buffer.
 */
void CL4V4BECallFunction::WriteVariableDeclaration(CBEFile* pFile,
    CBEContext* pContext)
{
    // first call base class (skip L4 class)
    CBECallFunction::WriteVariableDeclaration(pFile, pContext);
    // declare msgtag
    WriteMsgTagDeclaration(pFile, pContext);
}

/** \brief writes the declaration of the message tag
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4V4BECallFunction::WriteMsgTagDeclaration(CBEFile *pFile,
    CBEContext* pContext)
{
    string sMsgTag = pContext->GetNameFactory()->GetString(STR_MSGTAG_VARIABLE,
                        pContext, 0);
    *pFile << "\t// create empty msgtag\n";
    *pFile << "\tL4_MsgTag_t " << sMsgTag << " = L4_MsgTag ();\n";
}

/** \brief writes the marshaling of the message
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start with marshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to \
 *    false if not possible
 *  \param pContext the context of the write operation
 *
 * In V4 we marshal everything except the opcode, which goes into the message
 * tag label (this is also done in the default call-function class, so simply
 * skip L4 specific implementation.)
 */
void
CL4V4BECallFunction::WriteMarshalling(CBEFile * pFile,
    int nStartOffset,
    bool& bUseConstOffset,
    CBEContext * pContext)
{
    CBECallFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
}

/** \brief writes the invocation of the message transfer
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * Because this is the call function, we can use the IPC call of L4.
 */
void
CL4V4BECallFunction::WriteInvocation(CBEFile * pFile,
    CBEContext * pContext)
{
    string sMsgBuffer =
        pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    string sMsgTag = pContext->GetNameFactory()->GetString(STR_MSGTAG_VARIABLE,
                        pContext, 0);
    // load msgtag into message buffer
    *pFile << "\tL4_Set_MsgLabel ( &" << sMsgBuffer << ", " <<
        m_sOpcodeConstName << " );\n";
    // load the message into the UTCB
    *pFile << "\tL4_MsgLoad ( &" << sMsgBuffer << " );\n";
    // invocate
    WriteIPC(pFile, pContext);
    // store message
    *pFile << "\tL4_MsgStore ( " << sMsgTag << ", &" << sMsgBuffer << " );\n";
    // check for errors
    WriteIPCErrorCheck(pFile, pContext);
}

/** \brief write the error checking code for the IPC
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * If there was an IPC error, we write this into the environment.
 * This can be done by checking if there was an error, then sets the major value
 * to CORBA_SYSTEM_EXCEPTION and then sets the ipc_error value to
 * L4_IPC_ERROR(result).
 */
void
CL4V4BECallFunction::WriteIPCErrorCheck(CBEFile * pFile,
    CBEContext * pContext)
{
    string sResult = pContext->GetNameFactory()->GetString(STR_MSGTAG_VARIABLE,
                        pContext, 0);
    vector<CBEDeclarator*>::iterator iterCE = m_pCorbaEnv->GetFirstDeclarator();
    CBEDeclarator *pDecl = *iterCE;

    *pFile << "\tif (L4_IpcFailed (" << sResult << "))\n" <<
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
    *pFile << "_p.ipc_error = L4_ErrorCode();\n";
    // return
    WriteReturn(pFile, pContext);
    // close }
    pFile->DecIndent();
    *pFile << "\t}\n";
}

/** \brief unmarshals the exception
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start unmarshalling
 *  \param bUseConstOffset true if nStart can be used
 *  \param pContext the context of the unmarshalling
 *  \return the number of bytes used to unmarshal the exception
 *
 * In L4 the exception is in the tag's label, so get it from there. Therefore
 * it has no size and the other parameters can be unmarshalled directly.
 */
int
CL4V4BECallFunction::WriteUnmarshalException(CBEFile* pFile,
    int nStartOffset,
    bool& bUseConstOffset,
    CBEContext* pContext)
{
    if (FindAttribute(ATTR_NOEXCEPTIONS))
        return 0;
    if (!m_pExceptionWord)
        return 0;
    // get from msgtag
    string sMsgTag = pContext->GetNameFactory()->GetString(STR_MSGTAG_VARIABLE,
                        pContext, 0);
    vector<CBEDeclarator*>::iterator iterExc = m_pExceptionWord->GetFirstDeclarator();
    CBEDeclarator *pD = *iterExc;
    *pFile << "\t" << pD->GetName() << " = L4_Label ( " << sMsgTag << " );\n";
    // extract the exception from the word
    WriteEnvExceptionFromWord(pFile, pContext);
    return 0;
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \param pContext the context of this calculation
 *  \return the size of the parameters
 *
 * In V4 we do not have the exception nor the opcode in the msgbuf directly,
 * but in the tag's label.
 */
int CL4V4BECallFunction::GetSize(int nDirection, CBEContext *pContext)
{
    // get base class' size
    int nSize = CBECallFunction::GetSize(nDirection, pContext);
    if (nDirection & DIRECTION_OUT)
        nSize -= pContext->GetSizes()->GetExceptionSize();
    if (nDirection & DIRECTION_IN)
        nSize -= pContext->GetSizes()->GetOpcodeSize();
    return nSize;
}

/** \brief calculates the size of the function's fixed-sized parameters
 *  \param nDirection the direction to count
 *  \param pContext the context of this calculation
 *  \return the size of the parameters
 *
 * In V4 we do not have the exception nor the opcode in the msgbuf directly,
 * but in the tag's label.
 */
int CL4V4BECallFunction::GetFixedSize(int nDirection, CBEContext *pContext)
{
    int nSize = CBECallFunction::GetFixedSize(nDirection, pContext);
    if (nDirection & DIRECTION_OUT)
        nSize -= pContext->GetSizes()->GetExceptionSize();
    if (nDirection & DIRECTION_IN)
        nSize -= pContext->GetSizes()->GetOpcodeSize();
    return nSize;
}
