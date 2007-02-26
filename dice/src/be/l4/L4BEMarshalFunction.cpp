/**
 *	\file	dice/src/be/l4/L4BEMarshalFunction.cpp
 *	\brief	contains the implementation of the class CL4BEMarshalFunction
 *
 *	\date	10/10/2003
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
#include "be/l4/L4BEMarshalFunction.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEDeclarator.h"
#include "be/BEFile.h"
#include "be/BEType.h"
#include "be/BEContext.h"
#include "TypeSpec-Type.h"
#include "fe/FEAttribute.h"

IMPLEMENT_DYNAMIC(CL4BEMarshalFunction);

CL4BEMarshalFunction::CL4BEMarshalFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4BEMarshalFunction, CBEMarshalFunction);
}

CL4BEMarshalFunction::CL4BEMarshalFunction(CL4BEMarshalFunction & src)
: CBEMarshalFunction(src)
{
    IMPLEMENT_DYNAMIC_BASE(CL4BEMarshalFunction, CBEMarshalFunction);
}

/**	\brief destructor of target class */
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
    bool bSendFpages = ((CL4BEMsgBufferType*)m_pMsgBuffer)->HasSendFlexpages();
    if (bSendFpages)
	{
        // if (env.major != CORBA_NO_EXCEPTION)
		//   marshal exception
		// else
		//   marhsal flexpages
		VectorElement *pIter = m_pCorbaEnv->GetFirstDeclarator();
		CBEDeclarator *pDecl = m_pCorbaEnv->GetNextDeclarator(pIter);
		pFile->PrintIndent("if (");
		pDecl->WriteName(pFile, pContext);
		if (pDecl->GetStars() > 0)
		    pFile->Print("->");
		else
		    pFile->Print(".");
		pFile->Print("major != CORBA_NO_EXCEPTION)\n");
        pFile->PrintIndent("{\n");
		pFile->IncIndent();
		WriteMarshalException(pFile, nStartOffset, bUseConstOffset, pContext);
		// clear exception
		pFile->PrintIndent("CORBA_exception_free(");
		if (pDecl->GetStars() == 0)
		    pFile->Print("&");
		pDecl->WriteName(pFile, pContext);
		pFile->Print(");\n");
		pFile->DecIndent();
		pFile->PrintIndent("}\n");
		pFile->PrintIndent("else\n");
		pFile->PrintIndent("{\n");
		pFile->IncIndent();
		CBEOperationFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
		pFile->DecIndent();
		pFile->PrintIndent("}\n");
	}
	else
		CBEMarshalFunction::WriteMarshalling(pFile, nStartOffset, bUseConstOffset, pContext);
	// set size dope
	// but first zero it
    int nSendDirection = GetSendDirection();
	bool bHasSizeIsParams = (GetParameterCount(ATTR_SIZE_IS, ATTR_REF, nSendDirection) > 0) ||
	    (GetParameterCount(ATTR_LENGTH_IS, ATTR_REF, nSendDirection) > 0);
    ((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteSendDopeInit(pFile, nSendDirection, bHasSizeIsParams, pContext);
	// if we had send flexpages,we have to set the flexpage bit
	if (bSendFpages)
		((CL4BEMsgBufferType*)m_pMsgBuffer)->WriteSendFpageDope(pFile, pContext);
}

/** \brief decides whether two parameters should be exchanged during sort (moving 1st behind 2nd)
 *  \param pPrecessor the 1st parameter
 *  \param pSuccessor the 2nd parameter
 *  \param pContext the context of the sorting
 *  \return true if parameters should be exchanged
 */
bool CL4BEMarshalFunction::DoSortParameters(CBETypedDeclarator * pPrecessor, CBETypedDeclarator * pSuccessor, CBEContext * pContext)
{
    if (!(pPrecessor->GetType()->IsOfType(TYPE_FLEXPAGE)) &&
        pSuccessor->GetType()->IsOfType(TYPE_FLEXPAGE))
        return true;
    // if first is flexpage and second is not, we prohibit an exchange
    // explicetly
    if ( pPrecessor->GetType()->IsOfType(TYPE_FLEXPAGE) &&
        !pSuccessor->GetType()->IsOfType(TYPE_FLEXPAGE))
        return false;
    // if the 1st parameter is the return variable, we cannot exchange it, because
    // we make assumptions about its position in the message buffer
    String sReturn = pContext->GetNameFactory()->GetReturnVariable(pContext);
    if (pPrecessor->FindDeclarator(sReturn))
        return false;
    // if successor is return variable (should not occur) move it forward
    if (pSuccessor->FindDeclarator(sReturn))
        return true;
    // no special case, so use base class' method
    return CBEMarshalFunction::DoSortParameters(pPrecessor, pSuccessor, pContext);
}

/** \brief test if this function has variable sized parameters (needed to specify temp + offset var)
 *  \return true if variable sized parameters are needed
 */
bool CL4BEMarshalFunction::HasVariableSizedParameters()
{
    bool bRet = CBEMarshalFunction::HasVariableSizedParameters();
    bool bFixedNumberOfFlexpages = true;
    CBEClass *pClass = GetClass();
    assert(pClass);
    pClass->GetParameterCount(TYPE_FLEXPAGE, bFixedNumberOfFlexpages);
    // if no flexpages, return
    if (!bFixedNumberOfFlexpages)
        return true;
    return bRet;
}
