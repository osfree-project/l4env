/**
 *	\file	dice/src/be/BEInterfaceFunction.cpp
 *	\brief	contains the implementation of the class CBEInterfaceFunction
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

#include "be/BEInterfaceFunction.h"
#include "be/BEContext.h"
#include "be/BEType.h"
#include "be/BETypedDeclarator.h"
#include "be/BERoot.h"

#include "fe/FEInterface.h"
#include "TypeSpec-Type.h"
#include "fe/FEStringAttribute.h"

IMPLEMENT_DYNAMIC(CBEInterfaceFunction);

CBEInterfaceFunction::CBEInterfaceFunction()
{
    IMPLEMENT_DYNAMIC_BASE(CBEInterfaceFunction, CBEFunction);
}

CBEInterfaceFunction::CBEInterfaceFunction(CBEInterfaceFunction & src):CBEFunction(src)
{
    IMPLEMENT_DYNAMIC_BASE(CBEInterfaceFunction, CBEFunction);
}

/**	\brief destructor of target class */
CBEInterfaceFunction::~CBEInterfaceFunction()
{
}

/**	\brief creates the back-end function for the interface
 *	\param pFEInterface the respective front-end interface
 *	\param pContext the context of the code generation
 *	\return true if successful
 *
 * Create empty return variable.
 */
bool CBEInterfaceFunction::CreateBackEnd(CFEInterface * pFEInterface, CBEContext * pContext)
{
    if (!pFEInterface)
        return false;

    // basic init
    CBEFunction::CreateBackEnd(pContext);
    // search for our interface
    CBERoot *pRoot = GetRoot();
    assert(pRoot);
    m_pClass = pRoot->FindClass(pFEInterface->GetName());
    assert(m_pClass);
    // should be parent
    assert(m_pClass == m_pParent);
    // set return type
    if (!SetReturnVar(false, 0, TYPE_VOID, String(), pContext))
    {
        VERBOSE("CBEInterfaceFunction::CreateBE failed because return var could not be set\n");
        return false;
    }

    // check if interface has error function and add its name if available
    if (pFEInterface->FindAttribute(ATTR_ERROR_FUNCTION))
    {
        CFEStringAttribute *pErrorFunc = (CFEStringAttribute*)(pFEInterface->FindAttribute(ATTR_ERROR_FUNCTION));
        m_sErrorFunction = pErrorFunc->GetString();
    }

    return true;
}
