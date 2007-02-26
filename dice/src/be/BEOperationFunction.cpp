/**
 *	\file	dice/src/be/BEOperationFunction.cpp
 *	\brief	contains the implementation of the class CBEOperationFunction
 *
 *	\date	01/14/2002
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

#include "be/BEOperationFunction.h"
#include "be/BEContext.h"
#include "be/BEAttribute.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BEException.h"
#include "be/BERoot.h"
#include "be/BEDeclarator.h"
#include "be/BEClass.h"
#include "be/BEOpcodeType.h"
#include "be/BEMarshaller.h"

#include "fe/FEOperation.h"
#include "fe/FETypedDeclarator.h"
#include "fe/FEAttribute.h"
#include "fe/FEInterface.h"
#include "fe/FEStringAttribute.h"

IMPLEMENT_DYNAMIC(CBEOperationFunction);

CBEOperationFunction::CBEOperationFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CBEOperationFunction, CBEFunction);
}

CBEOperationFunction::CBEOperationFunction(CBEOperationFunction & src):CBEFunction(src)
{
    IMPLEMENT_DYNAMIC_BASE(CBEOperationFunction, CBEFunction);
}

/**	\brief destructor of target class */
CBEOperationFunction::~CBEOperationFunction()
{

}

/**	\brief prepares this class for further deployment using the front-end operation
 *	\param pFEOperation the respective front.end operation
 *	\param pContext the context of the code generation
 *	\return true if the code generation was succesful
 *
 * This implementation adds the attributes, types, parameters, exception, etc. given by the front-end function to
 * this instance of the back-end function.
 */
bool CBEOperationFunction::CreateBackEnd(CFEOperation * pFEOperation, CBEContext * pContext)
{
    // basic init
    CBEFunction::CreateBackEnd(pContext);
    // add attributes
    if (!AddAttributes(pFEOperation, pContext))
    {
        VERBOSE("%s failed because attributes could not be added\n", __PRETTY_FUNCTION__);
        return false;
    }
    // add return type
    String sReturn = pContext->GetNameFactory()->GetReturnVariable(pContext);
    if (!SetReturnVar(pFEOperation->GetReturnType(), sReturn, pContext))
    {
        VERBOSE("CBEOperationFunction::CreateBE failed because return var could not be set\n");
        return false;
    }
    // add parameters
    if (!AddParameters(pFEOperation, pContext))
    {
        VERBOSE("%s failed because parameters could not be added\n", __PRETTY_FUNCTION__);
        return false;
    }
    // add exceptions
    if (!AddExceptions(pFEOperation, pContext))
    {
        VERBOSE("%s failed because exceptions could not be added\n", __PRETTY_FUNCTION__);
        return false;
    }
    // set opcode name
    m_sOpcodeConstName = pContext->GetNameFactory()->GetOpcodeConst(pFEOperation, pContext);
    // set parent
    CBERoot *pRoot = GetRoot();
    assert(pRoot);
    assert(pFEOperation->GetParentInterface());
    m_pClass = pRoot->FindClass(pFEOperation->GetParentInterface()->GetName());
    assert(m_pClass);
    // would like to test for class == parent, but this is not the case for
    // switch case: parent = srv-loop function

    // check if interface has error function and add its name if available
    if (pFEOperation->GetParentInterface())
    {
        if (pFEOperation->GetParentInterface()->FindAttribute(ATTR_ERROR_FUNCTION))
        {
            CFEStringAttribute *pErrorFunc = (CFEStringAttribute*)(pFEOperation->GetParentInterface()->FindAttribute(ATTR_ERROR_FUNCTION));
            m_sErrorFunction = pErrorFunc->GetString();
        }
    }
    
    return true;
}

/**	\brief adds the parameters of a front-end function to this class
 *	\param pFEOperation the front-end function
 *	\param pContext the context of the code generation
 *	\return true if successful
 */
bool CBEOperationFunction::AddParameters(CFEOperation * pFEOperation, CBEContext * pContext)
{
    VectorElement *pIter = pFEOperation->GetFirstParameter();
    CFETypedDeclarator *pFEParameter;
    while ((pFEParameter = pFEOperation->GetNextParameter(pIter)) != 0)
    {
        if (!AddParameter(pFEParameter, pContext))
            return false;
    }
    return true;
}

/**	\brief adds a single parameter to this class
 *	\param pFEParameter the parameter to add
 *	\param pContext the context of the operation
 *	\return true if successful
 */
bool CBEOperationFunction::AddParameter(CFETypedDeclarator * pFEParameter, CBEContext * pContext)
{
    CBETypedDeclarator *pParameter = pContext->GetClassFactory()->GetNewTypedDeclarator();
    CBEFunction::AddParameter(pParameter);
    if (!pParameter->CreateBackEnd(pFEParameter, pContext))
    {
        VERBOSE("%s failed because parameter could not be created\n", __PRETTY_FUNCTION__);
        RemoveParameter(pParameter);
        delete pParameter;
        return false;
    }
    AddSortedParameter(pParameter);
    return true;
}

/**	\brief adds exceptions of a front-end function to this class
 *	\param pFEOperation the front-end function
 *	\param pContext the context of the operation
 *	\return true if successful
 */
bool CBEOperationFunction::AddExceptions(CFEOperation * pFEOperation,
					 CBEContext * pContext)
{
    VectorElement *pIter = pFEOperation->GetFirstRaisesDeclarator();
    CFEIdentifier *pFEException;
    while ((pFEException = pFEOperation->GetNextRaisesDeclarator(pIter)) != 0)
      {
	  if (!AddException(pFEException, pContext))
	      return false;
      }
    return true;
}

/**	\brief adds a single exception to this class
 *	\param pFEException the exception to add
 *	\param pContext the context of this code generation
 *	\return true if successful
 */
bool CBEOperationFunction::AddException(CFEIdentifier * pFEException, CBEContext * pContext)
{
    CBEException *pException = pContext->GetClassFactory()->GetNewException();
    CBEFunction::AddException(pException);
    if (!pException->CreateBackEnd(pFEException, pContext))
    {
        RemoveException(pException);
        delete pException;
        return false;
    }
    return true;
}

/**	\brief adds attributes of a front-end function this this class
 *	\param pFEOperation the front-end operation
 *	\param pContext the context of the code generation
 *	\return true if successful
 */
bool CBEOperationFunction::AddAttributes(CFEOperation * pFEOperation, CBEContext * pContext)
{
    VectorElement *pIter = pFEOperation->GetFirstAttribute();
    CFEAttribute *pFEAttribute;
    while ((pFEAttribute = pFEOperation->GetNextAttribute(pIter)) != 0)
    {
        if (!AddAttribute(pFEAttribute, pContext))
        return false;
    }
    return true;
}

/**	\brief adds a single attribute to this function
 *	\param pFEAttribute the attribute to add
 *	\param pContext the context of the operation
 *	\return true if successful
 */
bool CBEOperationFunction::AddAttribute(CFEAttribute * pFEAttribute, CBEContext * pContext)
{
    CBEAttribute *pAttribute = pContext->GetClassFactory()->GetNewAttribute();
    CBEFunction::AddAttribute(pAttribute);
    if (!pAttribute->CreateBackEnd(pFEAttribute, pContext))
    {
        RemoveAttribute(pAttribute);
        delete pAttribute;
        return false;
    }
    return true;
}

/** \brief returns a reference to the interface belonging to this function
 *  \return a reference to the interface of this function
 *
 * Because we have a hierarchy of libraries, types, consts and interfaces beside the
 * file and function hierarchy, we have to connect them somewhere. This is done using the
 * m_pInterface member. It will be set differently for Operation-function and interface-
 * functions. Anyhow: if a back-end function needs information about its interface it can
 * obtain a reference to this interface using this function
 */
CBEClass* CBEOperationFunction::GetClass()
{
    return m_pClass;
}

/** \brief marshals the opcode at the specified offset
 *  \param pFile the file to write to
 *  \param nStartOffset the offset to start marshalling from
 *  \param bUseConstOffset true if nStartOffset should be used
 *  \param pContext the context of the marshalling
 *  \return the size of the marshalled opcode
 *
 * This function assumes that it is called before the other parameters
 */
int CBEOperationFunction::WriteMarshalOpcode(CBEFile * pFile, int nStartOffset, bool& bUseConstOffset, CBEContext * pContext)
{
    int nSize = 0;
    // opcode type
    CBEOpcodeType *pType = pContext->GetClassFactory()->GetNewOpcodeType();
    pType->SetParent(this);
    if (pType->CreateBackEnd(pContext))
    {
        CBETypedDeclarator *pConst = pContext->GetClassFactory()->GetNewTypedDeclarator();
        pConst->SetParent(this);
        if (pConst->CreateBackEnd(pType, m_sOpcodeConstName, pContext))
        {
            CBEMarshaller *pMarshaller = pContext->GetClassFactory()->GetNewMarshaller(pContext);
            // only if we really did marshal something, set size
            nSize = pMarshaller->Marshal(pFile, pConst, nStartOffset, bUseConstOffset, m_vParameters.GetSize() == 0, pContext);
            delete pMarshaller;
        }
        delete pConst;
    }
    delete pType;
    return nSize;
}
