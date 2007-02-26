/**
 *    \file    dice/src/be/cdr/CCDRClass.cpp
 *    \brief   contains the implementation of the class CCDRClass
 *
 *    \date    10/28/2003
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

#include "be/cdr/CCDRClass.h"
#include "be/BEContext.h"
#include "be/BEMarshalFunction.h"
#include "be/BEUnmarshalFunction.h"
#include "be/BEComponentFunction.h"
#include "be/BEDispatchFunction.h"

#include "fe/FEOperation.h"
#include "Attribute-Type.h"

CCDRClass::CCDRClass()
 : CBEClass()
{
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
    // call CBEObject's CreateBackEnd method
    if (!CBEObject::CreateBackEnd(pFEOperation))
        return false;

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
            VERBOSE("%s failed, because call function could not be created for %s\n",
                    __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
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
            VERBOSE("%s failed, because unmarshal function could not be created for %s\n",
                    __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
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
            VERBOSE("%s failed, because call function could not be created for %s\n",
                    __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
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
            VERBOSE("%s failed, because unmarshal function could not be created for %s\n",
                    __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
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
            VERBOSE("%s failed, because component function could not be created for %s\n",
                    __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
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
            VERBOSE("%s failed, because send function could not be created for %s\n",
                    __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
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
            VERBOSE("%s failed, because wait function could not be created for %s\n",
                    __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
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
                VERBOSE("%s failed, because component function could not be created for %s\n",
                        __PRETTY_FUNCTION__, pFEOperation->GetName().c_str());
                return false;
            }
        }
    }

    // sort the parameters of the functions
    vector<CBEFunction*>::iterator iter = GetFirstFunction();
    CBEFunction *pFunction;
    while ((pFunction = GetNextFunction(iter)) != 0)
    {
        if (!pFunction->SortParameters(0, pContext))
        {
            VERBOSE("%s failed, because the parameters of function %s could not be sorted\n",
                __PRETTY_FUNCTION__, pFunction->GetName().c_str());
            return false;
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
