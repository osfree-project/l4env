/**
 *  \file     dice/src/be/l4/v4/L4V4BEMarshalFunction.cpp
 *  \brief    contains the implementation of the class CL4V4BEMarshalFunction
 *
 *  \date     Mon Jul 5 2004
 *  \author   Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/* Copyright (C) 2001-2004 by
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
#include "L4V4BEMarshalFunction.h"
#include "be/l4/v4/L4V4BENameFactory.h"
#include "be/BEContext.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "Attribute-Type.h"

CL4V4BEMarshalFunction::CL4V4BEMarshalFunction()
 : CL4BEMarshalFunction()
{
}

/** destroys the instance of this class */
CL4V4BEMarshalFunction::~CL4V4BEMarshalFunction()
{
}

/** \brief write the L4 specific marshalling code
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start marshalling in the message buffer
 *  \param bUseConstOffset true if nStartOffset should be used
 *  \param pContext the context of the write operation
 */
void
CL4V4BEMarshalFunction::WriteMarshalling(CBEFile* pFile,
    int nStartOffset,
    bool& bUseConstOffset,
    CBEContext* pContext)
{
    string sMsgBuffer =
        pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    // clear message
    *pFile << "\tL4_MsgClear ( " << sMsgBuffer << " );\n";
    // call base class
    // we skip L4 specific implementation, because it marshals flexpages
    // or exceptions (we don't need this) and it sets the send dope, which
    // is set by convenience functions automatically.
    // we also skip basic backend marshalling, because it marshals exception
    // first, we want this to be done later (into the tag label) and it
    // starts marshalling after the opcode, which is in the tag as well
    CBEOperationFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
    // set exception in msgbuffer
    WriteMarshalException(pFile, nStartOffset, bUseConstOffset, pContext);
}

/** \brief marshals the exception
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start marshalling
 *  \param bUseConstOffset true if nStart can be used
 *  \param pContext the context of the marshalling
 *  \return the number of bytes used to marshal the exception
 *
 * For V4 the exception is stored as label in the tag.
 */
int
CL4V4BEMarshalFunction::WriteMarshalException(CBEFile* pFile,
    int nStartOffset,
    bool& bUseConstOffset,
    CBEContext* pContext)
{
    if (FindAttribute(ATTR_NOEXCEPTIONS))
        return 0;
    if (!m_pExceptionWord)
        return 0;
    // store in message directly
    string sMsgBuffer =
        pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    vector<CBEDeclarator*>::iterator iterExc = m_pExceptionWord->GetFirstDeclarator();
    CBEDeclarator *pD = *iterExc;
    *pFile << "\tL4_Set_MsgLabel ( " << sMsgBuffer << ", " << pD->GetName()
        << " );\n";
    // since exception is in tag it does not use any space
    return 0;
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \param pContext the context of this calculation
 *  \return the size of the parameters
 *
 * For V4 exception is transmitted in tag, so no size for that.
 */
int
CL4V4BEMarshalFunction::GetFixedSize(int nDirection,
    CBEContext* pContext)
{
    int nSize = CBEMarshalFunction::GetFixedSize(nDirection, pContext);
    if (nDirection & DIRECTION_OUT)
        nSize -= pContext->GetSizes()->GetExceptionSize();
    return nSize;
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \param pContext the context of this calculation
 *  \return the size of the parameters
 *
 * For V4 exception is transmitted in tag, so no size for that.
 */
int
CL4V4BEMarshalFunction::GetSize(int nDirection,
    CBEContext *pContext)
{
    int nSize = CBEMarshalFunction::GetSize(nDirection, pContext);
    if (nDirection & DIRECTION_OUT)
        nSize -= pContext->GetSizes()->GetExceptionSize();
    return nSize;
}
