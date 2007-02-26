/* Copyright (C) 2001-2003 by
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

#include "be/l4/x0adapt/L4X0aBESndFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BEIPC.h"
#include "be/BEFile.h"
#include "be/BEContext.h"
#include "be/BEMarshaller.h"

#include "TypeSpec-Type.h"

IMPLEMENT_DYNAMIC(CL4X0aBESndFunction);

CL4X0aBESndFunction::CL4X0aBESndFunction()
 : CL4BESndFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4X0aBESndFunction, CL4BESndFunction);
}


CL4X0aBESndFunction::~CL4X0aBESndFunction()
{
}

/** \brief writes the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4X0aBESndFunction::WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext)
{
	// check if we use assembler
	bool bAssembler = ((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext);
    if (bAssembler)
	{
		CBENameFactory *pNF = pContext->GetNameFactory();
		String sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
		String sDummy = pNF->GetDummyVariable(pContext);
		pFile->PrintIndent("%s %s;\n", (const char*)sMWord, (const char*)sDummy);
	}
	CL4BESndFunction::WriteVariableDeclaration(pFile, pContext);
}

/** \brief marshals the parameters of this function
 *  \param pFile the file to marshal to
 *  \param nStartOffset the start offset in the message buffer
 *  \param bUseConstOffset true if nStartOffset can be used
 *  \param pContext the context of the write operation
 */
void CL4X0aBESndFunction::WriteMarshalling(CBEFile* pFile,  int nStartOffset,  bool& bUseConstOffset,  CBEContext* pContext)
{
	// check if we use assembler
	bool bAssembler = ((CL4BEIPC*)m_pComm)->UseAssembler(this, pContext);
    if (!(bAssembler &&
	    (((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(GetSendDirection(), pContext)) ))
	{
		CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
		// marshal opcode
		nStartOffset += WriteMarshalOpcode(pFile, nStartOffset, bUseConstOffset, pContext);
		// if we have send flexpages, marshal them now
		nStartOffset += pMarshaller->Marshal(pFile, this, TYPE_FLEXPAGE, 0/*all*/, nStartOffset, bUseConstOffset, pContext);
		// marshal rest
		pMarshaller->Marshal(pFile, this, -TYPE_FLEXPAGE, 0/*all*/, nStartOffset, bUseConstOffset, pContext);
		delete pMarshaller;
	}
}
