/**
 *  \file   dice/src/be/sock/SockBEMarshalFunction.cpp
 *  \brief  contains the implementation of the class CSockBEMarshalFunction
 *
 *  \date   10/14/2003
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "be/sock/SockBEMarshalFunction.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEMsgBufferType.h"
#include "be/BEDeclarator.h"

CSockBEMarshalFunction::CSockBEMarshalFunction()
{
}

CSockBEMarshalFunction::CSockBEMarshalFunction(CSockBEMarshalFunction & src)
: CBEMarshalFunction(src)
{
}

/** \brief destructor of target class */
CSockBEMarshalFunction::~CSockBEMarshalFunction()
{

}

/** \brief remove references from message buffer
 *  \param pFEInterface the front-end interface to use as reference
 *  \param pContext the context of the create process
 *  \return true on success
 */
bool CSockBEMarshalFunction::AddMessageBuffer(CFEInterface * pFEInterface, CBEContext * pContext)
{
    if (!CBEMarshalFunction::AddMessageBuffer(pFEInterface, pContext))
        return false;
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->GetAlias()->IncStars(-pMsgBuffer->GetAlias()->GetStars());
    return true;
}

/** \brief write the parameters after the "normal" parameters
 *  \param pFile the file to print to
 *  \param pContext the context of the write operation
 *  \param bComma true if a comma should be written before the parameters
 */
void CSockBEMarshalFunction::WriteCallAfterParameters(CBEFile* pFile,  CBEContext* pContext,  bool bComma)
{
    m_bCastMsgBufferOnCall = false;
    CBEMarshalFunction::WriteCallAfterParameters(pFile, pContext, bComma);
}
