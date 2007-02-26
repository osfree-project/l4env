/**
 *    \file    dice/src/be/sock/SockBECallFunction.cpp
 *    \brief   contains the implementation of the class CSockBECallFunction
 *
 *    \date    11/28/2002
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

#include "be/sock/SockBECallFunction.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEMarshaller.h"
#include "be/BEMsgBufferType.h"
#include "be/sock/BESocket.h"

CSockBECallFunction::CSockBECallFunction()
{
}

CSockBECallFunction::CSockBECallFunction(CSockBECallFunction & src)
: CBECallFunction(src)
{
}

/**    \brief destructor of target class */
CSockBECallFunction::~CSockBECallFunction()
{

}

/** \brief invoke the call to the server
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * Now this is a bit hairy:
 * -# need to open socke (socket call)
 * -# set parametes
 * -# send to socket
 * -# receive from socket
 * -# clode socket
 */
void CSockBECallFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    // create socket
    m_pComm->WriteInitialization(pFile, this, pContext);
    // call
    m_pComm->WriteCall(pFile, this, pContext);
    // close socket
    m_pComm->WriteCleanup(pFile, this, pContext);
}

/** \brief writes varaible declarations
 *  \param pFile the file to write to
 *  \param pContext the context of the write
 *
 * This declares variables, needed for the socket implementation, such
 * as a socket (int).
 */
void CSockBECallFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // write base class' variables
    CBECallFunction::WriteVariableDeclaration(pFile, pContext);
    // write this directly, because we know this runs on Linux
    pFile->PrintIndent("int sd, dice_ret_size;\n");
    // needed for receive
    string sObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    pFile->PrintIndent("socklen_t dice_fromlen = sizeof(*%s);\n", sObj.c_str());
}

/** \brief writes the marshaling of the message
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start with marshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param pContext the context of the write operation
 */
void CSockBECallFunction::WriteMarshalling(CBEFile * pFile, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext)
{
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    // marshal opcode
    nStartOffset += WriteMarshalOpcode(pFile, nStartOffset, bUseConstOffset, pContext);
    // marshal rest
    pMarshaller->Marshal(pFile, this, 0/*all types*/, 0/*all parameters*/, nStartOffset, bUseConstOffset, pContext);
    delete pMarshaller;
}

/** \brief initializes the variables
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CSockBECallFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    CBECallFunction::WriteVariableInitialization(pFile, pContext);
    string sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    string sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
    // msgbuffer is always pointer: either variable sized or char[]
    pFile->PrintIndent("bzero(%s, ", sMsgBuffer.c_str());
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (pMsgBuffer->IsVariableSized())
    {
        pFile->Print("%s);\n", sOffset.c_str());
    }
    else
        pFile->Print("sizeof(%s));\n", sMsgBuffer.c_str());
}
