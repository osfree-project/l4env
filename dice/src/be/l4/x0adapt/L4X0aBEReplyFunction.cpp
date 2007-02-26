/**
 *	\file	dice/src/be/l4/x0adapt/L4X0aBEReplyFunction.cpp
 *	\brief	contains the implementation of the class CL4X0aBEReplyFunction
 *
 *	\date	08/15/2003
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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
#include "be/l4/x0adapt/L4X0aBEReplyFunction.h"
#include "be/l4/L4BEIPC.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "TypeSpec-Type.h"

IMPLEMENT_DYNAMIC(CL4X0aBEReplyFunction);

CL4X0aBEReplyFunction::CL4X0aBEReplyFunction()
 : CL4BEReplyFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0aBEReplyFunction, CL4BEReplyFunction);
}

/** destroys the reply function object */
CL4X0aBEReplyFunction::~CL4X0aBEReplyFunction()
{
}

/** \brief write the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0aBEReplyFunction::WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext)
{
	CL4BEReplyFunction::WriteVariableDeclaration(pFile, pContext);
	// check if we use assembler
	bool bAssembler = ((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext);
    if (bAssembler)
    {
		CBENameFactory *pNF = pContext->GetNameFactory();
		String sDummy = pNF->GetDummyVariable(pContext);
		String sMWord = pNF->GetTypeName(TYPE_MWORD, false, pContext, 0);
		pFile->PrintIndent("%s %s;\n", (const char*)sMWord, (const char*)sDummy);
    }
}

/** \brief writes the unmarshalling code
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start unmarshalling in the message buffer
 *  \param bUseConstOffset true if nStartOffset should be used
 *  \param pContext the context of the unmarshalling
 */
void CL4X0aBEReplyFunction::WriteUnmarshalling(CBEFile* pFile,  int nStartOffset,  bool& bUseConstOffset,  CBEContext* pContext)
{
	WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
}
