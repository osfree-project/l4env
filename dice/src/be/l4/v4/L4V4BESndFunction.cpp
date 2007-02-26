/**
 *    \file    dice/src/be/l4/v4/L4V4BESndFunction.cpp
 *    \brief    contains the implementation of the class CL4V4BESndFunction
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

#include "be/l4/v4/L4V4BESndFunction.h"
#include "be/l4/v4/L4V4BENameFactory.h"
#include "be/BEContext.h"

CL4V4BESndFunction::CL4V4BESndFunction()
 : CL4BESndFunction()
{
}

/** destroy the instance */
CL4V4BESndFunction::~CL4V4BESndFunction()
{
}

/** \brief marshals the opcode at the specified offset
 *  \param pFile the file to write to
 *  \param nStartOffset the offset to start marshalling from
 *  \param bUseConstOffset true if nStartOffset should be used
 *  \param pContext the context of the marshalling
 *  \return the size of the marshalled opcode
 *
 * For V4 the opcode is part of the message tag, which is stored in MR0.
 * It is stored in the left-most 16 (or 48) Bits of MR0. We have to shift
 * the opcode left by 16 Bit and or it with the remainder of the MR0.
 */
int CL4V4BESndFunction::WriteMarshalOpcode(CBEFile* pFile,  int nStartOffset,  bool& bUseConstOffset,  CBEContext* pContext)
{
    string sMsgTag = pContext->GetNameFactory()->GetString(STR_MSGTAG_VARIABLE, pContext, 0);

    // MsgTag = MsgTagAddLabel(MsgTag, label)
    *pFile << "\t" << sMsgTag << " = MsgTagAddLabel(" << sMsgTag << ", " << m_sOpcodeConstName << ");\n";

    // assembler version x86
    // msgtag = (msgtag & 0xffff) | (opcode << 16);

    // nothing really marshalled, because opcode goes into MR0, which is not counted
    return 0;
}
