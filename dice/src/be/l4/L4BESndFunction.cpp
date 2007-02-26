/**
 *	\file	dice/src/be/l4/L4BESndFunction.cpp
 *	\brief	contains the implementation of the class CL4BESndFunction
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

#include "be/l4/L4BESndFunction.h"
#include "be/BEContext.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BEOpcodeType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEMarshaller.h"
#include "be/BEClient.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/l4/L4BESizes.h"

#include "fe/FETypeSpec.h"
#include "fe/FEAttribute.h"

IMPLEMENT_DYNAMIC(CL4BESndFunction);

CL4BESndFunction::CL4BESndFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4BESndFunction, CBESndFunction);
}

/** destructs the send function class */
CL4BESndFunction::~CL4BESndFunction()
{
}

/** \brief writes the variable declaration
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BESndFunction::WriteVariableDeclaration(CBEFile * pFile, CBEContext * pContext)
{
    // first call base class
    CBESndFunction::WriteVariableDeclaration(pFile, pContext);

    // write result variable
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    pFile->PrintIndent("l4_msgdope_t %s = { msgdope: 0 };\n", (const char *) sResult);
}

/** \brief writes the invocation code
 *  \param pFile the file tow rite to
 *  \param pContext the context of the write opeation
 */
void CL4BESndFunction::WriteInvocation(CBEFile * pFile, CBEContext * pContext)
{
    // after marshalling set the message dope
    int nDirection = GetSendDirection();
    ((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteSendDopeInit(pFile, nDirection, pContext);

    WriteIPC(pFile, pContext);
    WriteIPCErrorCheck(pFile, pContext);
}

/** \brief creates this class
 *  \param pFEOperation the resprective front-end operation
 *  \param pContext the context of the creation
 *  \return true if successful
 */
bool CL4BESndFunction::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    if (!CBESndFunction::CreateBackEnd(pFEOperation, pContext))
        return false;

    return true;
}

/** \brief writes the IPC error check
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 * \todo write IPC error checking
 */
void CL4BESndFunction::WriteIPCErrorCheck(CBEFile * pFile, CBEContext * pContext)
{
// should do some error checking here
}

/** \brief writes the marshalling code
 *  \param pFile the file to marshal to
 *  \param nStartOffset the const offset into the message buffer
 *  \param bUseConstOffset true if nStartOffset can be used
 *  \param pContext the context of this write operation
 */
void CL4BESndFunction::WriteMarshalling(CBEFile * pFile, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext)
{
    // if we have send flexpages, marshal them first
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    nStartOffset += pMarshaller->Marshal(pFile, this, TYPE_FLEXPAGE, nStartOffset, bUseConstOffset, pContext);
    // marshal opcode
    nStartOffset += WriteMarshalOpcode(pFile, nStartOffset, bUseConstOffset, pContext);
    // marshal rest
    pMarshaller->Marshal(pFile, this, -TYPE_FLEXPAGE, nStartOffset, bUseConstOffset, pContext);
    delete pMarshaller;
}

/** \brief init message buffer size dope
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BESndFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
    CBESndFunction::WriteVariableInitialization(pFile, pContext);
    ((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteSizeDopeInit(pFile, pContext);
}

/** \brief decides whether two parameters should be exchanged during sort (moving 1st behind 2nd)
 *  \param pPrecessor the 1st parameter
 *  \param pSuccessor the 2nd parameter
 *  \param pContext the context of the sorting
 *  \return true if parameters should be exchanged
 */
bool CL4BESndFunction::DoSortParameters(CBETypedDeclarator * pPrecessor, CBETypedDeclarator * pSuccessor, CBEContext * pContext)
{
    if (!(pPrecessor->GetType()->IsOfType(TYPE_FLEXPAGE)) &&
        pSuccessor->GetType()->IsOfType(TYPE_FLEXPAGE))
        return true;
    // if first is flexpage and second is not, we prohibit an exchange
    // explicetly
    if ( pPrecessor->GetType()->IsOfType(TYPE_FLEXPAGE) &&
        !pSuccessor->GetType()->IsOfType(TYPE_FLEXPAGE))
        return false;
    return CBESndFunction::DoSortParameters(pPrecessor, pSuccessor, pContext);
}

/** \brief write the IPC code
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CL4BESndFunction::WriteIPC(CBEFile *pFile, CBEContext *pContext)
{
    int nDirection = GetSendDirection();
    String sServerID = pContext->GetNameFactory()->GetComponentIDVariable(pContext);
    String sResult = pContext->GetNameFactory()->GetString(STR_RESULT_VAR, pContext);
    String sTimeout;
    if (IsComponentSide())
        sTimeout = pContext->GetNameFactory()->GetTimeoutServerVariable(pContext);
    else
        sTimeout = pContext->GetNameFactory()->GetTimeoutClientVariable(pContext);
    String sMsgBuffer = pContext->GetNameFactory()->GetMessageBufferVariable(pContext);
    String sMWord = pContext->GetNameFactory()->GetTypeName(TYPE_MWORD, true, pContext);

    pFile->PrintIndent("l4_i386_ipc_send(*%s,\n", (const char *) sServerID);
    pFile->IncIndent();
    pFile->PrintIndent("");
    bool bVarBuffer = m_pMsgBuffer->IsVariableSized(nDirection);
    if (((CL4BEMsgBufferType*)m_pMsgBuffer)->HasSendFlexpages())
        pFile->Print("(%s*)((%s)", (const char*)sMWord, (const char*)sMWord);
    if (((CL4BEMsgBufferType*)m_pMsgBuffer)->IsShortIPC(nDirection, pContext))
        pFile->Print("L4_IPC_SHORT_MSG");
    else
    {
        if (bVarBuffer)
            pFile->Print("%s", (const char *) sMsgBuffer);
        else
            pFile->Print("&%s", (const char *) sMsgBuffer);
    }
    if (((CL4BEMsgBufferType*)m_pMsgBuffer)->HasSendFlexpages())
        pFile->Print(")|2)");
    pFile->Print(",\n");

    pFile->PrintIndent("*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[0]))),\n");
    pFile->PrintIndent("*((%s*)(&(", (const char*)sMWord);
    m_pMsgBuffer->WriteMemberAccess(pFile, TYPE_INTEGER, pContext);
    pFile->Print("[4]))),\n");

    pFile->PrintIndent("%s, &%s);\n", (const char *) sTimeout, (const char *) sResult);

    pFile->DecIndent();
}
