/**
 *	\file	dice/src/be/l4/L4BEUnmarshalFunction.cpp
 *	\brief	contains the implementation of the class CL4BEUnmarshalFunction
 *
 *	\date	02/20/2002
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

#include "be/l4/L4BEUnmarshalFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/l4/L4BEMsgBufferType.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BETypedDeclarator.h"
#include "be/BEType.h"
#include "be/BEMarshaller.h"
#include "be/BEOpcodeType.h"

#include "TypeSpec-Type.h"
#include "fe/FEAttribute.h"

IMPLEMENT_DYNAMIC(CL4BEUnmarshalFunction);

CL4BEUnmarshalFunction::CL4BEUnmarshalFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CL4BEUnmarshalFunction, CBEUnmarshalFunction);
}

CL4BEUnmarshalFunction::CL4BEUnmarshalFunction(CL4BEUnmarshalFunction & src)
:CBEUnmarshalFunction(src)
{
    IMPLEMENT_DYNAMIC_BASE(CL4BEUnmarshalFunction, CBEUnmarshalFunction);
}

/**	\brief destructor of target class */
CL4BEUnmarshalFunction::~CL4BEUnmarshalFunction()
{

}

/** \brief initializie the parameters
 *  \param pFile the file to write to
 *  \param pContext the context of the write process
 *
 * The unmarshalling function has to init parameters, which need dynamically allocated memory.
 * Such parameters are, for instance, variable size arrays and strings.
 */
void CL4BEUnmarshalFunction::WriteVariableInitialization(CBEFile * pFile, CBEContext * pContext)
{
}

/** \brief writes the unmarshalling code
 *  \param pFile the file to write the code to
 *  \param nStartOffset the offset in the message buffer where to start
 *  \param bUseConstOffset true if nStartOffset should be used
 *  \param pContext the context of the write operation
 *
 * We first have to unmarshal possible flexpages, then skip the opcode size and do eventually the rest.
 */
void CL4BEUnmarshalFunction::WriteUnmarshalling(CBEFile * pFile, int nStartOffset, bool & bUseConstOffset, CBEContext * pContext)
{
    CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
    if (((CL4BEMsgBufferType*)m_pMsgBuffer)->HasReceiveFlexpages())
    {
        nStartOffset += pMarshaller->Unmarshal(pFile, this, TYPE_FLEXPAGE, 0/*all*/, nStartOffset, bUseConstOffset, pContext);
    }

    if (IsComponentSide())
    {
        // start after opcode
        CBEOpcodeType *pOpcodeType = pContext->GetClassFactory()->GetNewOpcodeType();
        pOpcodeType->SetParent(this);
        if (pOpcodeType->CreateBackEnd(pContext))
            nStartOffset += pOpcodeType->GetSize();
        delete pOpcodeType;
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

/** \brief decides whether two parameters should be exchanged during sort (moving 1st behind 2nd)
 *  \param pPrecessor the 1st parameter
 *  \param pSuccessor the 2nd parameter
 *  \param pContext the context of the sorting
 *  \return true if parameters should be exchanged
 */
bool CL4BEUnmarshalFunction::DoSortParameters(CBETypedDeclarator * pPrecessor, CBETypedDeclarator * pSuccessor, CBEContext * pContext)
{
    if (!(pPrecessor->GetType()->IsOfType(TYPE_FLEXPAGE)) &&
        pSuccessor->GetType()->IsOfType(TYPE_FLEXPAGE))
        return true;
    // if first is flexpage and second is not, we prohibit an exchange
    // explicetly
    if ( pPrecessor->GetType()->IsOfType(TYPE_FLEXPAGE) &&
        !pSuccessor->GetType()->IsOfType(TYPE_FLEXPAGE))
        return false;
    return CBEUnmarshalFunction::DoSortParameters(pPrecessor, pSuccessor, pContext);
}

/** \brief test if parameter needs additional reference
 *  \param pDeclarator the declarator to test
 *  \param pContext the context of the test
 *  \param bCall true if this test is invoked for a call to this function
 *  \return true if the given parameter needs an additional reference
 *
 * All [ref] attributes need an additional reference, because they are pointers, which
 * will be set by the unmarshal function.
 */
bool CL4BEUnmarshalFunction::HasAdditionalReference(CBEDeclarator * pDeclarator, CBEContext * pContext, bool bCall)
{
    if (CBEUnmarshalFunction::HasAdditionalReference(pDeclarator, pContext, bCall))
        return true;
    // find parameter
    CBETypedDeclarator *pParameter = GetParameter(pDeclarator, bCall);
    if (!pParameter)
        return false;
    // find attribute
    if (pParameter->FindAttribute(ATTR_REF))
        return true;
    return false;
}
