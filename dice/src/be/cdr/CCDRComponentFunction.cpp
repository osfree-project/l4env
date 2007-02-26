/**
 *	\file	dice/src/be/cdr/CCDRComponentFunction.cpp
 *	\brief	contains the implementation of the class CCDRComponentFunction
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

#include "be/cdr/CCDRComponentFunction.h"
#include "be/BEContext.h"
#include "be/BERoot.h"
#include "be/BETypedDeclarator.h"

#include "fe/FEOperation.h"

IMPLEMENT_DYNAMIC(CCDRComponentFunction);

CCDRComponentFunction::CCDRComponentFunction()
 : CBEComponentFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CCDRComponentFunction, CBEComponentFunction);
}

/** destroys this object */
CCDRComponentFunction::~CCDRComponentFunction()
{
}

/**	\brief creates the call function
 *	\param pFEOperation the front-end operation used as reference
 *	\param pContext the context of the write operation
 *	\return true if successful
 *
 * This implementation only sets the name of the function. And it stores a reference to
 * the client side function in case this implementation is tested.
 */
bool CCDRComponentFunction::CreateBackEnd(CFEOperation* pFEOperation,  CBEContext* pContext)
{
    pContext->SetFunctionType(FUNCTION_TEMPLATE);
	// set target file name
	SetTargetFileName(pFEOperation, pContext);
    // get own name
    m_sName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);

    if (!CBEOperationFunction::CreateBackEnd(pFEOperation, pContext))
        return false;

    CBERoot *pRoot = GetRoot();
    assert(pRoot);

    // check the attribute
	pContext->SetFunctionType(FUNCTION_MARSHAL);
    String sFunctionName = pContext->GetNameFactory()->GetFunctionName(pFEOperation, pContext);
    m_pFunction = pRoot->FindFunction(sFunctionName);
    if (!m_pFunction)
    {
        VERBOSE("CBEComponentFunction::CreateBackEnd failed because component's function (%s) could not be found\n",
                (const char*)sFunctionName);
        return false;
    }

    // the return value "belongs" to the client function (needed to determine global test variable's name)
    m_pReturnVar->SetParent(m_pFunction);

    return true;
}
