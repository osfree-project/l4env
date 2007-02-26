/**
 *    \file    dice/src/be/l4/x0adapt/L4X0aBEWaitAnyFunction.cpp
 *    \brief   contains the implementation of the class CL4X0aBEWaitAnyFunction
 *
 *    \date    06/01/2002
 *    \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "L4X0aBEWaitAnyFunction.h"
#include "be/l4/L4BEIPC.h"
#include "be/BEFile.h"
#include "be/BEContext.h"

#include "TypeSpec-Type.h"

CL4X0aBEWaitAnyFunction::CL4X0aBEWaitAnyFunction(bool bOpenWait, bool bReply)
 : CL4BEWaitAnyFunction(bOpenWait, bReply)
{
}

CL4X0aBEWaitAnyFunction::CL4X0aBEWaitAnyFunction(CL4X0aBEWaitAnyFunction &src)
: CL4BEWaitAnyFunction(src)
{
}

/** destroys the object */
CL4X0aBEWaitAnyFunction::~CL4X0aBEWaitAnyFunction()
{
}

/** \brief writes the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void
CL4X0aBEWaitAnyFunction::WriteVariableDeclaration(CBEFile * pFile,
    CBEContext * pContext)
{
    // check if we use assembler
    bool bAssembler = m_pComm->CheckProperty(this, COMM_PROP_USE_ASM, pContext);
    if (bAssembler)
    {
        CBENameFactory *pNF = pContext->GetNameFactory();
        string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
        string sDummy = pNF->GetDummyVariable(pContext);
        *pFile << "#if defined(__PIC__)\n";
        *pFile << "\t" << sMWord << " " << sDummy << ";\n";
        *pFile << "#else\n" << "#if !defined(PROFILE)\n";
        *pFile << "\t" << sMWord << " " << sDummy <<
            " __attribute__ ((unused));\n";
        *pFile << "#endif\n" << "#endif\n";
    }
    CL4BEWaitAnyFunction::WriteVariableDeclaration(pFile, pContext);
}

/** \brief writes the unmarshalling code
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start unmarshalling in the \
 *    message buffer
 *  \param bUseConstOffset true if nStartOffset should be used
 *  \param pContext the context of the write
 */
void
CL4X0aBEWaitAnyFunction::WriteUnmarshalling(CBEFile* pFile,
    int nStartOffset,
    bool& bUseConstOffset,
    CBEContext* pContext)
{
    if (m_bReply)
        WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
    else
        CL4BEWaitAnyFunction::WriteUnmarshalling(pFile, nStartOffset,
            bUseConstOffset, pContext);
}
