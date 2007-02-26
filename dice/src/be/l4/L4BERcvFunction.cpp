/**
 *	\file	dice/src/be/l4/L4BERcvFunction.cpp
 *	\brief	contains the implementation of the class CL4BERcvFunction
 *
 *	\date	Sat Jun 1 2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

#include "be/l4/L4BERcvFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BEComponent.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEType.h"
#include "be/BEClient.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BESizes.h"

#include "fe/FETypeSpec.h"
#include "fe/FEAttribute.h"

IMPLEMENT_DYNAMIC(CL4BERcvFunction);

CL4BERcvFunction::CL4BERcvFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4BERcvFunction, CBERcvFunction);
}

/** destroys this object */
CL4BERcvFunction::~CL4BERcvFunction()
{
}

/** \brief writes the variable declaration for this function
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BERcvFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // first call base class
    CBERcvFunction::WriteVariableDeclaration(pFile, pContext);

    // write result variable
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0};\n", (const char *) sResult);
}

/** \brief creates this receive function
 *  \param pFEOperation the reference front-end function
 *  \param pContext the context of the creation
 *  \return true if successfule
 */
bool CL4BERcvFunction::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    if (!CBERcvFunction::CreateBackEnd(pFEOperation, pContext))
        return false;

    return true;
}

/** \brief write the invocation call
 *  \param pFile the file to write to
 *  \param pContext the context of this operation
 *
 * \todo write opcode check
 */
void CL4BERcvFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    // init size and send dope
    ((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteSendDopeInit(pFile, pContext); // does not send anything

    // invocate
    WriteIPC(pFile, pContext);
    // write opcode check
    WriteOpcodeCheck(pFile, pContext);
    WriteIPCErrorCheck(pFile, pContext);
}

/** \brief write the IPC error checking code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * \todo write IPC error checking code
 */
void CL4BERcvFunction::WriteIPCErrorCheck(CBEFile * pFile, CBEContext * pContext)
{
    if (!m_sErrorFunction.IsEmpty())
    {
        String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
        pFile->PrintIndent("/* test for IPC errors */\n");
        pFile->PrintIndent("if (L4_IPC_IS_ERROR(%s))\n", (const char *) sResult);
        pFile->IncIndent();
        pFile->PrintIndent("%s(%s);\n", (const char*)m_sErrorFunction, (const char*)sResult);
        pFile->DecIndent();
    }
}

/** \brief init message buffer size dope
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BERcvFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    CBERcvFunction::WriteVariableInitialization(pFile, pContext);
    ((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteSizeDopeInit(pFile, pContext);
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
void CL4BERcvFunction::WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext)
{
    if (((CL4BEMsgBufferType*) m_pMsgBuffer)->HasReceiveFlexpages())
    {
        // we have to always check if this was a flexpage IPC
        String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
        pFile->PrintIndent("if (l4_ipc_fpage_received(%s))\n", (const char*)sResult);
        WriteFlexpageOpcodePatch(pFile, pContext);  // does indent itself
        pFile->PrintIndent("else\n");
        pFile->IncIndent();
        WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
        pFile->DecIndent();
    }
    else
    {
        WriteUnmarshalReturn(pFile, nStartOffset, bUseConstOffset, pContext);
    }
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
void CL4BERcvFunction::WriteFlexpageOpcodePatch(CBEFile *pFile, CBEContext *pContext)
{
    if (!(((CL4BEMsgBufferType*)m_pMsgBuffer)->HasReceiveFlexpages()))
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
        String sTempVar = pContext->GetNameFactory()->GetTempOffsetVariable(pContext);
        // init temp var
        pFile->PrintIndent("%s = 0;\n", (const char*)sTempVar);
        pFile->PrintIndent("while ((");
        m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
        pFile->Print("[%s] != 0) && (", (const char*)sTempVar);
        m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
        pFile->Print("[%s+4] != 0)) %s += 8;\n", (const char*)sTempVar, (const char*)sTempVar);
        // now sTempVar points to the delimiter flexpage
        // we have to add another 8 bytes to find the opcode, because UnmarshalReturn does only use
        // temp-var
        pFile->PrintIndent("%s += 8;\n", (const char*)sTempVar);
        // now unmarshal opcode
        bool bUseConstOffset = false;
        WriteUnmarshalReturn(pFile, 0, bUseConstOffset, pContext);
        pFile->DecIndent();
        pFile->PrintIndent("}\n");
    }
}

/** \brief decides whether two parameters should be exchanged during sort (moving 1st behind 2nd)
 *  \param pPrecessor the 1st parameter
 *  \param pSuccessor the 2nd parameter
 *  \param pContext the context of the sorting
 *  \return true if parameters should be exchanged
 */
bool CL4BERcvFunction::DoSortParameters(CBETypedDeclarator * pPrecessor, CBETypedDeclarator * pSuccessor, CBEContext * pContext)
{
    if (!(pPrecessor->GetType()->IsOfType(TYPE_FLEXPAGE)) &&
        pSuccessor->GetType()->IsOfType(TYPE_FLEXPAGE))
        return true;
    // if first is flexpage and second is not, we prohibit an exchange
    // explicetly
    if ( pPrecessor->GetType()->IsOfType(TYPE_FLEXPAGE) &&
        !pSuccessor->GetType()->IsOfType(TYPE_FLEXPAGE))
        return false;
    return CBERcvFunction::DoSortParameters(pPrecessor, pSuccessor, pContext);
}

/** \brief writes the ipc code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BERcvFunction::WriteIPC(CBEFile *pFile, CBEContext *pContext)
{
    String sServerID = pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    String sTimeout;
    if (IsComponentSide())
        sTimeout = pContext->GetNameFactory()->GetTimeoutServerVariable(pContext);
    else
        sTimeout = pContext->GetNameFactory()->GetTimeoutClientVariable(pContext);

    pFile->PrintIndent("l4_i386_ipc_receive(*%s,\n", (const char *) sServerID);
    pFile->IncIndent();

    String sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);

    int nDirection = GetReceiveDirection();
    bool bVarBuffer = m_pMsgBuffer->IsVariableSized(nDirection);
    if (((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(nDirection, pContext))
        pFile->PrintIndent("L4_IPC_SHORT_MSG,\n");
    else
    {
        if (bVarBuffer)
            pFile->PrintIndent("%s,\n", (const char *) sMsgBuffer);
        else
            pFile->PrintIndent("&%s,\n", (const char *) sMsgBuffer);
    }

    String sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);
    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0])),\n");
    pFile->PrintIndent("(%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4])),\n");

    pFile->PrintIndent("%s, &%s);\n", (const char *) sTimeout, (const char *) sResult);
    pFile->DecIndent();
}
