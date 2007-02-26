/**
 *  \file     dice/src/be/l4/v4/L4V4BEDispatchFunction.cpp
 *  \brief    contains the implementation of the class CL4V4BEDispatchFunction
 *
 *  \date     Tue Jul 6 2004
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
#include "L4V4BEDispatchFunction.h"
#include "be/BEContext.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "Attribute-Type.h"

CL4V4BEDispatchFunction::CL4V4BEDispatchFunction()
 : CL4BEDispatchFunction()
{
}

/** destroys instance of this class */
CL4V4BEDispatchFunction::~CL4V4BEDispatchFunction()
{
}

/** \brief write the L4 specific code when setting the opcode exception in the \
 *    message buffer
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * In V4 we do not set the send dope, that's why we skip L4 specific
 * implementation.
 */
void
CL4V4BEDispatchFunction::WriteSetWrongOpcodeException(CBEFile* pFile,
    CBEContext* pContext)
{
    // first call base class
    CBEDispatchFunction::WriteSetWrongOpcodeException(pFile, pContext);
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
CL4V4BEDispatchFunction::WriteMarshalException(CBEFile* pFile,
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
