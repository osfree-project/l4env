/**
 *	\file	dice/src/be/sock/SockBEReplyWaitFunction.cpp
 *	\brief	contains the implementation of the class CSockBEReplyWaitFunction
 *
 *	\date	11/28/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

#include "be/sock/SockBEReplyWaitFunction.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEMsgBufferType.h"
#include "be/BEType.h"
#include "be/BEDeclarator.h"

IMPLEMENT_DYNAMIC(CSockBEReplyWaitFunction);

CSockBEReplyWaitFunction::CSockBEReplyWaitFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CSockBEReplyWaitFunction, CBEReplyWaitFunction);
}

CSockBEReplyWaitFunction::CSockBEReplyWaitFunction(CSockBEReplyWaitFunction & src)
: CBEReplyWaitFunction(src)
{
    IMPLEMENT_DYNAMIC_BASE(CSockBEReplyWaitFunction, CBEReplyWaitFunction);
}

/**	\brief destructor of target class */
CSockBEReplyWaitFunction::~CSockBEReplyWaitFunction()
{

}

/** \brief writes the invocation call
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * Socket is already open
 *
 * \todo receive from any sender
 */
void CSockBEReplyWaitFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    // get CORBA_Object and msg buffer
    String sCorbaObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    String sCorbaEnv = pContext->GetNameFactory()->GetCorbaEnvironmentVariable(pContext);
    String sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    // get size of reply
    if (m_pMsgBuffer->IsVariableSized(GetSendDirection()))
    {
        String sOffset = pContext->GetNameFactory()->GetOffsetVariable(pContext);
        // send reply
        pFile->PrintIndent("dice_ret_size = sendto(%s->cur_socket, %s, %s, 0, (struct sockaddr*)%s, dice_fromlen);\n",
            (const char*)sCorbaEnv, (const char*)sMsgBuffer, (const char*)sOffset, (const char*)sCorbaObj);
        pFile->PrintIndent("if (dice_ret_size < %s)\n", (const char*)sOffset);
    }
    else
    {
        int nSize = m_pMsgBuffer->GetFixedCount(GetSendDirection());
        // send reply
        pFile->PrintIndent("dice_ret_size = sendto(%s->cur_socket, %s, %d, 0, (struct sockaddr*)%s, dice_fromlen);\n",
            (const char*)sCorbaEnv, (const char*)sMsgBuffer, nSize, (const char*)sCorbaObj);
        pFile->PrintIndent("if (dice_ret_size < %d)\n", nSize);
    }
    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    pFile->PrintIndent("perror(\"reply\");\n");
    WriteReturn(pFile, pContext);
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
    // reset msg buffer to zeros
    pFile->PrintIndent("bzero(%s, sizeof", (const char*)sMsgBuffer);
    m_pMsgBuffer->GetType()->WriteCast(pFile, false, pContext);
    pFile->Print(");\n");
    // want to receive from any client again
    pFile->PrintIndent("%s->sin_addr.s_addr = INADDR_ANY;\n",
        (const char*)sCorbaObj);
    // wait for new request
    pFile->PrintIndent("dice_ret_size = recvfrom(%s->cur_socket, %s, sizeof",
        (const char*)sCorbaEnv, (const char*)sMsgBuffer);
    m_pMsgBuffer->GetType()->WriteCast(pFile, false, pContext);
    pFile->Print(", 0, (struct sockaddr*)%s, &dice_fromlen);\n", (const char*)sCorbaObj);
    pFile->PrintIndent("if (dice_ret_size < 0)\n");
    pFile->PrintIndent("{\n");
    pFile->IncIndent();
    pFile->PrintIndent("perror(\"wait\");\n");
    WriteReturn(pFile, pContext);
    pFile->DecIndent();
    pFile->PrintIndent("}\n");
}

/** \brief writes additional variable declarations
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CSockBEReplyWaitFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    CBEReplyWaitFunction::WriteVariableDeclaration(pFile, pContext);
    pFile->PrintIndent("int dice_ret_size;\n");
    String sObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    pFile->PrintIndent("socklen_t dice_fromlen = sizeof(*%s);\n", (const char*)sObj);
}

/** \brief remove references from message buffer
 *  \param pFEInterface the front-end interface to use as reference
 *  \param pContext the context of the create process
 *  \return true on success
 */
bool CSockBEReplyWaitFunction::AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext)
{
    if (!CBEReplyWaitFunction::AddMessageBuffer(pFEInterface, pContext))
        return false;
    m_pMsgBuffer->GetAlias()->IncStars(-m_pMsgBuffer->GetAlias()->GetStars());
    return true;
}
