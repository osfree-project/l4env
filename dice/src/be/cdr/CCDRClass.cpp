/**
 *  \file    dice/src/be/cdr/CCDRClass.cpp
 *  \brief   contains the implementation of the class CCDRClass
 *
 *  \date    10/28/2003
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#include "be/BEClassFactory.h"
#include "Compiler.h"
#include "fe/FEOperation.h"
#include "fe/FEInterface.h"
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
 *  \return true if successful
 *
 * A function has to be generated depending on its attributes. If it is a call
 * function, we have to generate different back-end function than for a
 * message passing function.
 *
 * We depend on the fact, that either the [in] or the [out] attribute are
 * specified.  Never both may appear.
 */
void 
CCDRClass::CreateBackEnd(CFEOperation* pFEOperation)
{
    string exc = string(__func__);
    // set source information
    CBEObject::CreateBackEnd(pFEOperation);

    CFunctionGroup *pGroup = new CFunctionGroup(pFEOperation);
    m_FunctionGroups.Add(pGroup);
    CBEClassFactory *pCF = CCompiler::GetClassFactory();

    if (!(pFEOperation->m_Attributes.Find(ATTR_IN)) &&
        !(pFEOperation->m_Attributes.Find(ATTR_OUT)))
    {
        // the call case:
        // for client side: marshal unmarshal
        CBEOperationFunction *pFunction = pCF->GetNewMarshalFunction();
        m_Functions.Add(pFunction);
        pFunction->SetComponentSide(false);
        pGroup->m_Functions.Add(pFunction);
	try
	{
	    pFunction->CreateBackEnd(pFEOperation);
	}
	catch (CBECreateException *e)
        {
	    m_Functions.Remove(pFunction);
            delete pFunction;
	    e->Print();
	    delete e;

	    exc += " failed, because call function could not be created for " +
		pFEOperation->GetName();
	    throw new CBECreateException(exc);
        }

        pFunction = pCF->GetNewUnmarshalFunction();
        m_Functions.Add(pFunction);
        pFunction->SetComponentSide(false);
        pGroup->m_Functions.Add(pFunction);
	try
	{
	    pFunction->CreateBackEnd(pFEOperation);
	}
	catch (CBECreateException *e)
        {
	    m_Functions.Remove(pFunction);
            delete pFunction;
	    e->Print();
	    delete e;

	    exc += " failed, because unmarshal function could not be created" \
		" for " + pFEOperation->GetName();
            throw new CBECreateException(exc);
        }

        // server side
        pFunction = pCF->GetNewMarshalFunction();
        m_Functions.Add(pFunction);
        pFunction->SetComponentSide(true);
        pGroup->m_Functions.Add(pFunction);
	try
	{
	    pFunction->CreateBackEnd(pFEOperation);
	}
	catch (CBECreateException *e)
        {
	    m_Functions.Remove(pFunction);
            delete pFunction;
	    e->Print();
	    delete e;

	    exc += " failed, because call function could not be created for " +
		pFEOperation->GetName();
            throw new CBECreateException(exc);
        }

        pFunction = pCF->GetNewUnmarshalFunction();
        m_Functions.Add(pFunction);
        pFunction->SetComponentSide(true);
        pGroup->m_Functions.Add(pFunction);
	try
	{
	    pFunction->CreateBackEnd(pFEOperation);
	}
	catch (CBECreateException *e)
        {
	    m_Functions.Remove(pFunction);
            delete pFunction;
	    e->Print();
	    delete e;

	    exc += " failed, because unmarshal function could not be created" \
		" for " + pFEOperation->GetName();
            throw new CBECreateException(exc);
        }

        pFunction = pCF->GetNewComponentFunction();
        m_Functions.Add(pFunction);
        pFunction->SetComponentSide(true);
        pGroup->m_Functions.Add(pFunction);
	try
	{
	    pFunction->CreateBackEnd(pFEOperation);
	}
	catch (CBECreateException *e)
        {
	    m_Functions.Remove(pFunction);
            delete pFunction;
	    e->Print();
	    delete e;
	    
	    exc += " failed, because component function could not be created" \
		" for " + pFEOperation->GetName();
            throw new CBECreateException(exc);
        }
    }
    else
    {
        // the MP case
        bool bComponent = (pFEOperation->m_Attributes.Find(ATTR_OUT));
        // sender: marshal
        CBEOperationFunction *pFunction = pCF->GetNewMarshalFunction();
        m_Functions.Add(pFunction);
        pFunction->SetComponentSide(bComponent);
        pGroup->m_Functions.Add(pFunction);
	try
	{
	    pFunction->CreateBackEnd(pFEOperation);
	}
	catch (CBECreateException *e)
        {
	    m_Functions.Remove(pFunction);
            delete pFunction;
	    e->Print();
	    delete e;

	    exc += " failed, because send function could not be created for " +
		pFEOperation->GetName();
            throw new CBECreateException(exc);
        }

        // receiver: unmarshal
        pFunction = pCF->GetNewUnmarshalFunction();
        m_Functions.Add(pFunction);
        pFunction->SetComponentSide(!bComponent);
        pGroup->m_Functions.Add(pFunction);
	try
	{
	    pFunction->CreateBackEnd(pFEOperation);
	}
	catch (CBECreateException *e)
        {
	    m_Functions.Remove(pFunction);
            delete pFunction;
	    e->Print();
	    delete e;

	    exc += " failed, because wait function could not be created for " +
		pFEOperation->GetName();
            throw new CBECreateException(exc);
        }

        // if we send oneway to the server we need a component function
        if (pFEOperation->m_Attributes.Find(ATTR_IN))
        {
            pFunction = pCF->GetNewComponentFunction();
            m_Functions.Add(pFunction);
            pFunction->SetComponentSide(true);
            pGroup->m_Functions.Add(pFunction);
	    try
	    {
		pFunction->CreateBackEnd(pFEOperation);
	    }
	    catch (CBECreateException *e)
            {
		m_Functions.Remove(pFunction);
                delete pFunction;
		e->Print();
		delete e;

		exc += " failed, because component function could not be" \
		    " created for " + pFEOperation->GetName();
                throw new CBECreateException(exc);
            }
        }
    }
}

/** \brief adds the functions for an interface
 *  \param pFEInterface the interface to add the functions for
 *  \return true if successful
 */
void
CCDRClass::AddInterfaceFunctions(CFEInterface* pFEInterface)
{
    CBEClassFactory *pCF = CCompiler::GetClassFactory();
    CBEInterfaceFunction *pFunction = pCF->GetNewDispatchFunction();
    m_Functions.Add(pFunction);
    pFunction->SetComponentSide(true);
    try
    {
	pFunction->CreateBackEnd(pFEInterface);
    }
    catch (CBECreateException *e)
    {
	m_Functions.Remove(pFunction);
        delete pFunction;
	e->Print();
	delete e;

	string exc = string(__func__);
	exc += " failed because dispatch function could not be created";
        throw new CBECreateException(exc);
    }
}

