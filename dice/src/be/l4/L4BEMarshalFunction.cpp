/**
 *    \file    dice/src/be/l4/L4BEMarshalFunction.cpp
 *    \brief   contains the implementation of the class CL4BEMarshalFunction
 *
 *    \date    10/10/2003
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
#include "be/l4/L4BEMarshalFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEFile.h"
#include "be/BEUserDefinedType.h"
#include "be/BEContext.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"

CL4BEMarshalFunction::CL4BEMarshalFunction()
{
}

CL4BEMarshalFunction::CL4BEMarshalFunction(CL4BEMarshalFunction & src)
: CBEMarshalFunction(src)
{
}

/**    \brief destructor of target class */
CL4BEMarshalFunction::~CL4BEMarshalFunction()
{

}

/** \brief write the L4 specific unmarshalling code
 *  \param pFile the file to write to
 *  \param nStartOffset the offset where to start marshalling in the message buffer
 *  \param bUseConstOffset true if nStartOffset should be used
 *  \param pContext the context of the write operation
 *
 * If we send flexpages we have to do special handling:
 * if (exception)
 *   marshal exception
 * else
 *   marshal normal flexpages
 *
 * Without flexpages:
 * marshal exception
 * marshal rest
 */
void CL4BEMarshalFunction::WriteMarshalling(CBEFile* pFile,  int nStartOffset,  bool& bUseConstOffset,  CBEContext* pContext)
{
    CBEMsgBufferType *pMsgBuffer = GetMessageBuffer();
    assert(pMsgBuffer);
    bool bSendFpages = pMsgBuffer->GetCount(TYPE_FLEXPAGE, GetSendDirection()) > 0;
    if (bSendFpages)
    {
        // if (env.major == CORBA_NO_EXCEPTION)
        //   marshal flexpages
        // else
        //   marhsal exception
        string sFreeFunc;
        if (((CBEUserDefinedType*)m_pCorbaEnv->GetType())->GetName() ==
            "CORBA_Server_Environment")
            sFreeFunc = "CORBA_server_exception_free";
        else
            sFreeFunc = "CORBA_exception_free";
        vector<CBEDeclarator*>::iterator iterCE = m_pCorbaEnv->GetFirstDeclarator();
        CBEDeclarator *pDecl = *iterCE;
        pFile->PrintIndent("if (");
        pDecl->WriteName(pFile, pContext);
        if (pDecl->GetStars() > 0)
            pFile->Print("->");
        else
            pFile->Print(".");
        pFile->Print("major == CORBA_NO_EXCEPTION)\n");
        pFile->PrintIndent("{\n");
        pFile->IncIndent();
        pFile->IncIndent();
        CBEOperationFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
        pFile->DecIndent();
        pFile->DecIndent();
        pFile->PrintIndent("}\n");
        pFile->PrintIndent("else\n");
        pFile->PrintIndent("{\n");
        pFile->IncIndent();
        WriteMarshalException(pFile, nStartOffset, bUseConstOffset, pContext);
        // clear exception
        *pFile << "\t" << sFreeFunc << "(";
        if (pDecl->GetStars() == 0)
            pFile->Print("&");
        pDecl->WriteName(pFile, pContext);
        pFile->Print(");\n");
        pFile->DecIndent();
        pFile->PrintIndent("}\n");
    }
    else
        CBEMarshalFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
    // set size dope
    pMsgBuffer->WriteInitialization(pFile, TYPE_MSGDOPE_SEND, GetSendDirection(), pContext);
    // if we had send flexpages,we have to set the flexpage bit
    if (bSendFpages)
    {
        pFile->PrintIndent("");
        pMsgBuffer->WriteMemberAccess(pFile, TYPE_MSGDOPE_SEND, DIRECTION_IN, pContext);
        pFile->Print(".md.fpage_received = 1;\n");
    }
}

/** \brief decides whether two parameters should be exchanged during sort
 *  \param pPrecessor the 1st parameter
 *  \param pSuccessor the 2nd parameter
 *    \param pContext the context of the sorting
 *  \return true if parameters 1st is smaller than 2nd
 */
bool
CL4BEMarshalFunction::DoExchangeParameters(CBETypedDeclarator * pPrecessor,
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
    if (m_pReturnVar)
    {
        vector<CBEDeclarator*>::iterator iterRet = m_pReturnVar->GetFirstDeclarator();
        CBEDeclarator *pDecl = *iterRet;
        // if the 1st parameter is the return variable, we cannot exchange it, because
        // we make assumptions about its position in the message buffer
        if (pPrecessor->FindDeclarator(pDecl->GetName()))
            return false;
        // if successor is return variable (should not occur) move it forward
        if (pSuccessor->FindDeclarator(pDecl->GetName()))
            return true;
    }
    // no special case, so use base class' method
    return CBEMarshalFunction::DoExchangeParameters(pPrecessor, pSuccessor, pContext);
}

/** \brief test if this function has variable sized parameters (needed to specify temp + offset var)
 *  \return true if variable sized parameters are needed
 */
bool CL4BEMarshalFunction::HasVariableSizedParameters(int nDirection)
{
    bool bRet = CBEMarshalFunction::HasVariableSizedParameters(nDirection);
    bool bFixedNumberOfFlexpages = true;
    CBEClass *pClass = GetClass();
    assert(pClass);
    pClass->GetParameterCount(TYPE_FLEXPAGE, bFixedNumberOfFlexpages);
    // if no flexpages, return
    if (!bFixedNumberOfFlexpages)
        return true;
    // if we have indirect strings to marshal then we need the offset vars
    if (GetParameterCount(ATTR_REF, 0, nDirection))
        return true;
    return bRet;
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \param pContext the context of this calculation
 *  \return the size of the parameters
 *
 * If we send flexpages, remove the exception size again, since either the
 * flexpage or the exception is sent.
 */
int CL4BEMarshalFunction::GetFixedSize(int nDirection,  CBEContext* pContext)
{
    int nSize = CBEMarshalFunction::GetFixedSize(nDirection, pContext);
    if ((nDirection & DIRECTION_OUT) &&
        !FindAttribute(ATTR_NOEXCEPTIONS) &&
        (GetParameterCount(TYPE_FLEXPAGE, DIRECTION_OUT) > 0))
        nSize -= pContext->GetSizes()->GetExceptionSize();
    return nSize;
}

/** \brief calculates the size of the function's parameters
 *  \param nDirection the direction to count
 *  \param pContext the context of this calculation
 *  \return the size of the parameters
 *
 * If we send flexpages, remove the exception size again, since either the
 * flexpage or the exception is sent.
 */
int CL4BEMarshalFunction::GetSize(int nDirection, CBEContext *pContext)
{
    int nSize = CBEMarshalFunction::GetSize(nDirection, pContext);
    if ((nDirection & DIRECTION_OUT) &&
        !FindAttribute(ATTR_NOEXCEPTIONS) &&
        (GetParameterCount(TYPE_FLEXPAGE, DIRECTION_OUT) > 0))
        nSize -= pContext->GetSizes()->GetExceptionSize();
    return nSize;
}
