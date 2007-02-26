/**
 *  \file   dice/src/be/l4/v2/L4V2BEMarshaller.cpp
 *  \brief  contains the implementation of the class CL4V2BEMarshaller
 *
 *  \date   05/20/2003
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

#include "be/l4/v2/L4V2BEMarshaller.h"
#include "be/BEFunction.h"
#include "be/BEType.h"



CL4V2BEMarshaller::CL4V2BEMarshaller()
 : CL4BEMarshaller()
{
}

/** destructs the L4 marshaller object */
CL4V2BEMarshaller::~CL4V2BEMarshaller()
{
}

/** \brief marshals a complete parameter
 *  \param pFile the file to marshal to
 *  \param pParameter the parameter to marshal
 *  \param nStartOffset the starting offset into the message buffer
 *  \param bUseConstOffset true if nStartOffset can be used to index the message buffer
 *  \param bLastParameter true if this is the last parameter
 *  \param pContext the context of this marshalling
 *
 * We first call the base class implementation and then check whether total number of flexpages has been reached.
 * If it has, then all flexpages are marshalled and we write the flexpage seperator.
 */
int CL4V2BEMarshaller::Marshal(CBEFile* pFile,  CBETypedDeclarator* pParameter,  int nStartOffset,  bool& bUseConstOffset,  bool bLastParameter,  CBEContext* pContext)
{
    int nSize = CL4BEMarshaller::Marshal(pFile, pParameter, nStartOffset, bUseConstOffset, bLastParameter, pContext);
    // check if flexpage limit is reached
    // in this implementation we have to check for current-1, because the base class
    // incremented current after checking it. To avoid future false positives, we
    // also increment current
    if ((m_nCurrentFlexpages-1) == m_nTotalFlexpages)
    {
        // if we have a return value, we already unmarshalled it.
        // add its size here, so the offset is correct.
        if (m_pFunction &&
            !m_pFunction->IsComponentSide() &&
            m_pFunction->GetReturnType() &&
            !m_pFunction->GetReturnType()->IsVoid() &&
            !m_bMarshal)
        {
            nSize += m_pFunction->GetReturnType()->GetSize();
        }
        // increment current
        m_nCurrentFlexpages++;
    }
    return nSize;
}

