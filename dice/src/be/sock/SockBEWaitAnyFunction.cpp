/**
 *    \file    dice/src/be/sock/SockBEWaitAnyFunction.cpp
 *    \brief   contains the implementation of the class CSockBEWaitAnyFunction
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

#include "be/sock/SockBEWaitAnyFunction.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEMsgBufferType.h"
#include "be/BEType.h"
#include "be/BEDeclarator.h"
#include "be/sock/BESocket.h"

CSockBEWaitAnyFunction::CSockBEWaitAnyFunction(bool bOpenWait, bool bReply)
: CBEWaitAnyFunction(bOpenWait, bReply)
{
}

CSockBEWaitAnyFunction::CSockBEWaitAnyFunction(CSockBEWaitAnyFunction & src)
: CBEWaitAnyFunction(src)
{
}

/**    \brief destructor of target class */
CSockBEWaitAnyFunction::~CSockBEWaitAnyFunction()
{

}

/** \brief writes the message invocation
 *  \param pFile the file to write to
 *  \param pContext the context of the write
 *
 * The socket has to be open already.
 */
void
CSockBEWaitAnyFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    // wait for new request
    assert(m_pComm);
    if (m_bReply)
        m_pComm->WriteReplyAndWait(pFile, this, pContext);
    else
        m_pComm->WriteWait(pFile, this, pContext);
}

/** \brief writes additional variable declarations
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CSockBEWaitAnyFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    CBEWaitAnyFunction::WriteVariableDeclaration(pFile, pContext);
    pFile->PrintIndent("int dice_ret_size;\n");
    string sObj = pContext->GetNameFactory()->GetCorbaObjectVariable(pContext);
    pFile->PrintIndent("socklen_t dice_fromlen = sizeof(*%s);\n", sObj.c_str());
}

/** \brief remove references from message buffer
 *  \param pFEInterface the front-end interface to use as reference
 *  \param pContext the context of the create process
 *  \return true on success
 */
bool CSockBEWaitAnyFunction::AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext)
{
    if (!CBEWaitAnyFunction::AddMessageBuffer(pFEInterface, pContext))
        return false;
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->GetAlias()->IncStars(-pMsgBuffer->GetAlias()->GetStars());
    return true;
}
