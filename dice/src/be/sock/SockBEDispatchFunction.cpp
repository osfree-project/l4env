/**
 *    \file    dice/src/be/sock/SockBEDispatchFunction.cpp
 *    \brief    contains the implementation of the class CSockBEDispatchFunction
 *
 *    \date    06/01/2004
 *    \author    Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "be/sock/SockBEDispatchFunction.h"
#include "be/BEMsgBufferType.h"
#include "be/BEDeclarator.h"

CSockBEDispatchFunction::CSockBEDispatchFunction()
 : CBEDispatchFunction()
{
}

CSockBEDispatchFunction::CSockBEDispatchFunction(CSockBEDispatchFunction &src)
 : CBEDispatchFunction(src)
{
}

/** destroys dispatch function object */
CSockBEDispatchFunction::~CSockBEDispatchFunction()
{
}

/** \brief remove references from message buffer
 *  \param pFEInterface the front-end interface to use as reference
 *  \param pContext the context of the create process
 *  \return true on success
 */
bool CSockBEDispatchFunction::AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext)
{
    if (!CBEDispatchFunction::AddMessageBuffer(pFEInterface, pContext))
        return false;
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->GetAlias()->IncStars(-pMsgBuffer->GetAlias()->GetStars());
    return true;
}

/** \brief writes the additional parameters after the "normal" parameters
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *  \param bComma true if a comma should be written first
 */
void CSockBEDispatchFunction::WriteCallAfterParameters(CBEFile* pFile,  CBEContext* pContext,  bool bComma)
{
    m_bCastMsgBufferOnCall = false;
    CBEDispatchFunction::WriteCallAfterParameters(pFile, pContext, bComma);
}
