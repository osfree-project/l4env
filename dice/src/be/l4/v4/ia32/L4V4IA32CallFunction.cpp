/**
 *    \file    dice/src/be/l4/v4/ia32/L4V4IA32CallFunction.cpp
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

#include "be/l4/v4/ia32/L4V4IA32CallFunction.h"
#include "be/BEContext.h"
#include "be/l4/v4/L4V4BENameFactory.h"

CL4V4IA32CallFunction::CL4V4IA32CallFunction()
 : CL4V4BECallFunction()
{
}

/** destroys object of this class */
CL4V4IA32CallFunction::~CL4V4IA32CallFunction()
{
}

/** \brief writes the IA32 specific variable declarations for msgtag
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4V4IA32CallFunction::WriteMsgTagDeclaration(CBEFile* pFile,
    CBEContext* pContext)
{
    if (pContext->IsOptionSet(PROGRAM_FORCE_C_BINDINGS))
        CL4V4BECallFunction::WriteMsgTagDeclaration(pFile, pContext);
    else
    {
        string sMsgTag =
            pContext->GetNameFactory()->GetString(STR_MSGTAG_VARIABLE,
                pContext, 0);
        *pFile << "\tregister L4_MsgTag_t " << sMsgTag << " asm(\"esi\");\n";
    }
}
