/**
 *    \file    dice/src/be/l4/L4BEWaitFunction.cpp
 *    \brief   contains the implementation of the class CL4BEWaitFunction
 *
 *    \date    06/01/2002
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

#include "be/l4/L4BEWaitFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEClassFactory.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEComponent.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEType.h"
#include "be/BEClient.h"
#include "be/BEMarshaller.h"
#include "be/BEOpcodeType.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BESizes.h"
#include "be/l4/L4BEIPC.h"

#include "TypeSpec-Type.h"
#include "Attribute-Type.h"

CL4BEWaitFunction::CL4BEWaitFunction(bool bOpenWait)
: CBEWaitFunction(bOpenWait)
{
}

/** destroys this object */
CL4BEWaitFunction::~CL4BEWaitFunction()
{
}

/** \brief writes the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of this operation
 */
void CL4BEWaitFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // first call base class
    CBEWaitFunction::WriteVariableDeclaration(pFile, pContext);

    // write result variable
    string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", sResult.c_str());
}

/** \todo write opcode check
 */
void CL4BEWaitFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    // set size and send dope
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->WriteInitialization(pFile, TYPE_MSGDOPE_SEND, 0, pContext);

    // invocate
    WriteIPC(pFile, pContext);
    // write opcode check
    WriteOpcodeCheck(pFile, pContext);

    WriteIPCErrorCheck(pFile, pContext);
}

/** \brief writes the IPC error checking code
 *  \param pFile the file to write to
 *  \param pContext the context of the write
 *
 * \todo write the IPC error checking code
 */
void CL4BEWaitFunction::WriteIPCErrorCheck(CBEFile * pFile, CBEContext * pContext)
{
    if (!m_sErrorFunction.empty())
    {
        string sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
        pFile->PrintIndent("/* test for IPC errors */\n");
        pFile->PrintIndent("if (L4_IPC_IS_ERROR(%s))\n", sResult.c_str());
        pFile->IncIndent();
        *pFile << "\t" << m_sErrorFunction << "(" << sResult << ", ";
        WriteCallParameter(pFile, m_pCorbaEnv, pContext);
        *pFile << ");\n";
        pFile->DecIndent();
    }
}

/** \brief writes the unmarshalling code for this function
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start unmarshalling
 *  \param bUseConstOffset true if a constant offset should be used, set it to false if not possible
 *  \param pContext the context of the write operation
 *
 * The receive function unmarshals the opcode. We can print this code by hand. We should use
 * a marshaller anyways.
 */
void CL4BEWaitFunction::WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext)
{
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (pMsgBuffer->GetCount(TYPE_FLEXPAGE, GetReceiveDirection()) > 0)
    {
        nStartOffset += pMarshaller->Unmarshal(pFile, this, TYPE_FLEXPAGE, 0/*all*/, nStartOffset, bUseConstOffset, pContext);
    }

    if (IsComponentSide())
    {
        /* If the noopcode option is set, there is no opcode in the message
         * buffer. Therefore start immediately after the flexpages (if any).
         */
        if (!FindAttribute(ATTR_NOOPCODE))
        {
            // start after opcode
            CBEOpcodeType *pOpcodeType = pContext->GetClassFactory()->GetNewOpcodeType();
            pOpcodeType->SetParent(this);
            if (pOpcodeType->CreateBackEnd(pContext))
                nStartOffset += pOpcodeType->GetSize();
            delete pOpcodeType;
        }
    }
    else
    {
        // first unmarshal return value
        nStartOffset += WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
    }
    // now unmarshal rest
    pMarshaller->Unmarshal(pFile, this, -TYPE_FLEXPAGE, 0/*all*/, nStartOffset, bUseConstOffset, pContext);
    delete pMarshaller;
}

/** \brief writes a patch to find the opcode if flexpage were received
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * This function may receive messages from different function. Because we don't know at
 * compile time, which function sends, we don't know if the message contains a flexpage.
 * If it does the unmarshalled opcode is wrong. Because flexpages have to come first in
 * the message buffer, the opcode cannot be the first parameter. We have to check this
 * condition and get the opcode from behind the flexpages.
 *
 * First we get the number of flexpages of the interface. If it has none, we don't need
 * this extra code. If it has a fixed number of flexpages (either none is sent or one, but
 * if flexpages are sent it is always the same number of flexpages) we can hard code
 * the offset were to find the opcode. If we have different numbers of flexpages (one
 * function may send one, another sends two) we have to use code which can deal with a variable
 * number of flexpages.
 */
void CL4BEWaitFunction::WriteFlexpageOpcodePatch(CBEFile *pFile, CBEContext *pContext)
{
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    if (pMsgBuffer->GetCount(TYPE_FLEXPAGE, GetReceiveDirection()) == 0)
        return;
    bool bFixedNumberOfFlexpages = true;
    int nNumberOfFlexpages = m_pClass->GetParameterCount(TYPE_FLEXPAGE, bFixedNumberOfFlexpages);
    // if fixed number  (should be true for only one flexpage as well)
    if (bFixedNumberOfFlexpages)
    {
        // the fixed offset (where to find the opcode) is:
        // offset = 8*nMaxNumberOfFlexpages + 8
        bool bUseConstOffset = true;
        pFile->IncIndent();
        WriteUnmarshalReturn(pFile, 8*nNumberOfFlexpages+8, bUseConstOffset, pContext);
        pFile->DecIndent();
    }
    else
    {
        // the variable offset can be determined by searching for the delimiter flexpage
        // which is two zero dwords
        pFile->PrintIndent("{\n");
        pFile->IncIndent();
        // search for delimiter flexpage
        string sTempVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        // init temp var
        pFile->PrintIndent("%s = 0;\n", sTempVar.c_str());
        pFile->PrintIndent("while ((");
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
        pFile->Print("[%s] != 0) && (", sTempVar.c_str());
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, DIRECTION_OUT, pContext);
        pFile->Print("[%s+4] != 0)) %s += 8;\n", sTempVar.c_str(), sTempVar.c_str());
        // now sTempVar points to the delimiter flexpage
        // we have to add another 8 bytes to find the opcode, because UnmarshalReturn does only use
        // temp-var
        pFile->PrintIndent("%s += 8;\n", sTempVar.c_str());
        // now unmarshal opcode
        bool bUseConstOffset = false;
        WriteUnmarshalReturn(pFile, 0, bUseConstOffset, pContext);
        pFile->DecIndent();
        pFile->PrintIndent("}\n");
    }
}

/** \brief decides whether two parameters should be exchanged during sort
 *  \param pPrecessor the 1st parameter
 *  \param pSuccessor the 2nd parameter
 *  \param pContext the context of the sorting
 *  \return true if parameters 1st is smaller than 2nd
 */
bool
CL4BEWaitFunction::DoExchangeParameters(CBETypedDeclarator * pPrecessor,
    CBETypedDeclarator * pSuccessor,
    CBEContext *pContext)
{
    if (!(pPrecessor->GetType()->IsOfType(TYPE_FLEXPAGE)) &&
        pSuccessor->GetType()->IsOfType(TYPE_FLEXPAGE))
        return true;
    // if first is flexpage and second is not, we prohibit an exchange
    // explicetly
    if ( pPrecessor->GetType()->IsOfType(TYPE_FLEXPAGE) &&
        !pSuccessor->GetType()->IsOfType(TYPE_FLEXPAGE))
        return false;
    return CBEWaitFunction::DoExchangeParameters(pPrecessor, pSuccessor, pContext);
}

/** \brief writes the ipc code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BEWaitFunction::WriteIPC(CBEFile *pFile, CBEContext *pContext)
{
    assert(m_pComm);
    if (m_bOpenWait)
        m_pComm->WriteWait(pFile, this, pContext);
    else
        m_pComm->WriteReceive(pFile, this, pContext);
}

/** \brief init message receive flexpage
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BEWaitFunction::WriteVariableInitialization(CBEFile* pFile,  CBEContext* pContext)
{
    CBEWaitFunction::WriteVariableInitialization(pFile, pContext);
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    pMsgBuffer->WriteInitialization(pFile, TYPE_MSGDOPE_SIZE, 0, pContext);
    pMsgBuffer->WriteInitialization(pFile, TYPE_FLEXPAGE, GetReceiveDirection(), pContext);
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \param pContext the context of this calculation
 *  \return the size of the parameters
 *
 * If we recv flexpages, remove the exception size again, since either the
 * flexpage or the exception is sent.
 */
int CL4BEWaitFunction::GetSize(int nDirection, CBEContext *pContext)
{
    // get base class' size
    int nSize = CBEWaitFunction::GetSize(nDirection, pContext);
    if ((nDirection & DIRECTION_OUT) &&
        !FindAttribute(ATTR_NOEXCEPTIONS) &&
        (GetParameterCount(TYPE_FLEXPAGE, DIRECTION_OUT) > 0))
        nSize -= pContext->GetSizes()->GetExceptionSize();
    return nSize;
}

/** \brief calculates the size of the function's fixed-sized parameters
 *  \param nDirection the direction to count
 *  \param pContext the context of this calculation
 *  \return the size of the parameters
 *
 * If we recv flexpages, remove the exception size again, since either the
 * flexpage or the exception is sent.
 */
int CL4BEWaitFunction::GetFixedSize(int nDirection, CBEContext *pContext)
{
    int nSize = CBEWaitFunction::GetFixedSize(nDirection, pContext);
    if ((nDirection & DIRECTION_OUT) &&
        !FindAttribute(ATTR_NOEXCEPTIONS) &&
        (GetParameterCount(TYPE_FLEXPAGE, DIRECTION_OUT) > 0))
        nSize -= pContext->GetSizes()->GetExceptionSize();
    return nSize;
}

/** \brief test if this function has variable sized parameters (needed to specify temp + offset var)
 *  \return true if variable sized parameters are needed
 */
bool CL4BEWaitFunction::HasVariableSizedParameters(int nDirection)
{
    bool bRet = CBEWaitFunction::HasVariableSizedParameters(nDirection);
    // if we have indirect strings to marshal then we need the offset vars
    if (GetParameterCount(ATTR_REF, 0, nDirection))
        return true;
    return bRet;
}
