/**
 *    \file    dice/src/be/l4/v2/L4V2BESndFunction.cpp
 *    \brief   contains the implementation of the class CL4V2BESndFunction
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
#include "be/l4/v2/L4V2BESndFunction.h"
#include "be/l4/L4BEIPC.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEMarshaller.h"
#include "be/BEContext.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"



CL4V2BESndFunction::CL4V2BESndFunction()
 : CL4BESndFunction()
{

}

/** destroys the send function object */
CL4V2BESndFunction::~CL4V2BESndFunction()
{
}

/** \brief writes the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4V2BESndFunction::WriteVariableDeclaration(CBEFile * pFile,  CBEContext * pContext)
{
    // check if we use assembler
    bool bAssembler = m_pComm->CheckProperty(this, COMM_PROP_USE_ASM, pContext);
    if (bAssembler)
    {
        CBENameFactory *pNF = pContext->GetNameFactory();
        string sMWord = pNF->GetTypeName(TYPE_MWORD, true, pContext);
        string sDummy = pNF->GetDummyVariable(pContext);
        *pFile << "\t" << sMWord << " " << sDummy << ";\n";
    }
    CL4BESndFunction::WriteVariableDeclaration(pFile, pContext);
}

/** \brief writes the parameter marshalling
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start marshalling
 *  \param bUseConstOffset true if nStartOffset should be used
 *  \param pContext the context of the write method
 *
 * For V2 we need to marshal flexpages before the opcode. They always
 * have to come first.
 */
void CL4V2BESndFunction::WriteMarshalling(CBEFile* pFile,  int nStartOffset,  bool& bUseConstOffset,  CBEContext* pContext)
{
    VERBOSE("CL4V2BESndFunction::WriteMarshalling(%s) called\n",
        GetName().c_str());
    // check if we use assembler
    bool bAssembler = m_pComm->CheckProperty(this, COMM_PROP_USE_ASM, pContext);
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (!(bAssembler &&
      pMsgBuffer->CheckProperty(MSGBUF_PROP_SHORT_IPC, GetSendDirection(), pContext)))
    {
        // if we have send flexpages, marshal them first
        CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
        nStartOffset += pMarshaller->Marshal(pFile, this, TYPE_FLEXPAGE, 0/*all*/, nStartOffset, bUseConstOffset, pContext);
        // marshal opcode
        if (!FindAttribute(ATTR_NOOPCODE))
            nStartOffset += WriteMarshalOpcode(pFile, nStartOffset, bUseConstOffset, pContext);
        // marshal rest
        pMarshaller->Marshal(pFile, this, -TYPE_FLEXPAGE, 0/*all*/, nStartOffset, bUseConstOffset, pContext);
        delete pMarshaller;
    }
    VERBOSE("CL4V2BESndFunction::WriteMarshalling(%s) finished\n",
        GetName().c_str());
}
