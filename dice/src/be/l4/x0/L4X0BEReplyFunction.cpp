/**
 *    \file    dice/src/be/l4/x0/L4X0BEReplyFunction.cpp
 *    \brief   contains the implementation of the class CL4X0BEReplyFunction
 *
 *    \date    08/15/2003
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
#include "be/l4/x0/L4X0BEReplyFunction.h"
#include "be/l4/L4BEIPC.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "TypeSpec-Type.h"

CL4X0BEReplyFunction::CL4X0BEReplyFunction()
 : CL4BEReplyFunction()
{
}

/** destroys the reply function object */
CL4X0BEReplyFunction::~CL4X0BEReplyFunction()
{
}

/** \brief write the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0BEReplyFunction::WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext)
{
    CL4BEReplyFunction::WriteVariableDeclaration(pFile, pContext);
    // check if we use assembler
    bool bAssembler = m_pComm->CheckProperty(this, COMM_PROP_USE_ASM, pContext);
    if (bAssembler)
    {
        CBENameFactory *pNF = pContext->GetNameFactory();
        string sDummy = pNF->GetDummyVariable(pContext);
        string sMWord = pNF->GetTypeName(TYPE_MWORD, false, pContext, 0);
        *pFile << "\t" << sMWord << " " << sDummy << ";\n";
    }
}

/** \brief writes the unmarshalling code
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start unmarshalling in the message buffer
 *  \param bUseConstOffset true if nStartOffset should be used
 *  \param pContext the context of the unmarshalling
 */
void CL4X0BEReplyFunction::WriteUnmarshalling(CBEFile* pFile,  int nStartOffset,  bool& bUseConstOffset,  CBEContext* pContext)
{
    WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
}
