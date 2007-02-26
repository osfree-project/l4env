/**
 *	\file	dice/src/be/cdr/CCDRClass.cpp
 *	\brief	contains the implementation of the class CCDRClass
 *
 *	\date	10/28/2003
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

#include "be/cdr/CCDRClass.h"
#include "be/BEContext.h"
#include "be/BEMarshalFunction.h"
#include "be/BEUnmarshalFunction.h"
#include "be/BEComponentFunction.h"
#include "be/BEDispatchFunction.h"

#include "fe/FEOperation.h"
#include "fe/FEAttribute.h"

IMPLEMENT_DYNAMIC(CCDRClass);

CCDRClass::CCDRClass()
 : CBEClass()
{
    IMPLEMENT_DYNAMIC_BASE(CCDRClass, CBEClass);
}

/** \brief destroys this class */
CCDRClass::~CCDRClass()
{
}

/** \brief internal function to create the back-end functions
 *  \param pFEOperation the respective front-end function
 *  \param pContext the context of the create process
 *  \return true if successful
 *
 * A function has to be generated depending on its attributes. If it is a call function,
 * we have to generate different back-end function than for a message passing function.
 *
 * We depend on the fact, that either the [in] or the [out] attribute are specified.
 * Never both may appear.
 */
bool CCDRClass::CreateBackEnd(CFEOperation* pFEOperation,  CBEContext* pContext)
{
    CFunctionGroup *pGroup = new CFunctionGroup(pFEOperation);
    AddFunctionGroup(pGroup);

    if (!(pFEOperation->FindAttribute(ATTR_IN)) &&
        !(pFEOperation->FindAttribute(ATTR_OUT)))
    {
        // the call case:
        // for client side: marshal unmarshal
        CBEOperationFunction *pFunction = pContext->GetClassFactory()->GetNewMarshalFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(false);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("CBEClass::CreateBackEnd failed, because call function could not be created for %s\n",
                    (const char*)pFEOperation->GetName());
            return false;
        }

		pFunction = pContext->GetClassFactory()->GetNewUnmarshalFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(false);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("CBEClass::CreateBackEnd failed, because unmarshal function could not be created for %s\n",
                    (const char*)pFEOperation->GetName());
            return false;
        }

		// server side
        pFunction = pContext->GetClassFactory()->GetNewMarshalFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(true);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("CBEClass::CreateBackEnd failed, because call function could not be created for %s\n",
                    (const char*)pFEOperation->GetName());
            return false;
        }

		pFunction = pContext->GetClassFactory()->GetNewUnmarshalFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(true);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("CBEClass::CreateBackEnd failed, because unmarshal function could not be created for %s\n",
                    (const char*)pFEOperation->GetName());
            return false;
        }

		pFunction = pContext->GetClassFactory()->GetNewComponentFunction();
		AddFunction(pFunction);
		pFunction->SetComponentSide(true);
		pGroup->AddFunction(pFunction);
		if (!pFunction->CreateBackEnd(pFEOperation, pContext))
		{
			RemoveFunction(pFunction);
			delete pFunction;
			VERBOSE("CBEClass::CreateBackEnd failed, because component function could not be created for %s\n",
					(const char*)pFEOperation->GetName());
			return false;
		}
    }
    else
    {
        // the MP case
        bool bComponent = (pFEOperation->FindAttribute(ATTR_OUT));
        // sender: marshal
        CBEOperationFunction *pFunction = pContext->GetClassFactory()->GetNewMarshalFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(bComponent);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("CBEClass::CreateBackEnd failed, because send function could not be created for %s\n",
                    (const char*)pFEOperation->GetName());
            return false;
        }

        // receiver: unmarshal
        pFunction = pContext->GetClassFactory()->GetNewUnmarshalFunction();
        AddFunction(pFunction);
        pFunction->SetComponentSide(!bComponent);
        pGroup->AddFunction(pFunction);
        if (!pFunction->CreateBackEnd(pFEOperation, pContext))
        {
            RemoveFunction(pFunction);
            delete pFunction;
            VERBOSE("CBEClass::CreateBackEnd failed, because wait function could not be created for %s\n",
                    (const char*)pFEOperation->GetName());
            return false;
        }

		// if we send oneway to the server we need a component function
		if (pFEOperation->FindAttribute(ATTR_IN))
		{
			pFunction = pContext->GetClassFactory()->GetNewComponentFunction();
			AddFunction(pFunction);
			pFunction->SetComponentSide(true);
			pGroup->AddFunction(pFunction);
			if (!pFunction->CreateBackEnd(pFEOperation, pContext))
			{
				RemoveFunction(pFunction);
				delete pFunction;
				VERBOSE("CBEClass::CreateBackEnd failed, because component function could not be created for %s\n",
						(const char*)pFEOperation->GetName());
				return false;
			}
		}
    }
    return true;
}

/** \brief adds the functions for an interface
 *  \param pFEInterface the interface to add the functions for
 *  \param pContext the context of this operation
 *  \return true if successful
 */
bool CCDRClass::AddInterfaceFunctions(CFEInterface* pFEInterface,  CBEContext* pContext)
{
    CBEInterfaceFunction *pFunction = pContext->GetClassFactory()->GetNewDispatchFunction();
	AddFunction(pFunction);
	pFunction->SetComponentSide(true);
	if (!pFunction->CreateBackEnd(pFEInterface, pContext))
	{
	    RemoveFunction(pFunction);
		VERBOSE("CBEClass::CreateBackEnd failed because dispatch function could not be created\n");
        delete pFunction;
        return false;
    }
    return true;
}
