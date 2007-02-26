/**
 *  \file    dice/src/be/l4/L4BEUnmarshalFunction.cpp
 *  \brief   contains the implementation of the class CL4BEUnmarshalFunction
 *
 *  \date    02/20/2002
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

#include "be/l4/L4BEUnmarshalFunction.h"
#include "be/l4/L4BENameFactory.h"
#include "be/BEContext.h"
#include "be/BEFile.h"
#include "be/BETypedDeclarator.h"
#include "be/BEType.h"
#include "be/BEMarshaller.h"

#include "TypeSpec-Type.h"
#include "Attribute-Type.h"

CL4BEUnmarshalFunction::CL4BEUnmarshalFunction()
{
}

CL4BEUnmarshalFunction::CL4BEUnmarshalFunction(CL4BEUnmarshalFunction & src)
:CBEUnmarshalFunction(src)
{
}

/** \brief destructor of target class */
CL4BEUnmarshalFunction::~CL4BEUnmarshalFunction()
{

}

/** \brief test if parameter needs additional reference
 *  \param pDeclarator the declarator to test
 *  \param bCall true if this test is invoked for a call to this function
 *  \return true if the given parameter needs an additional reference
 *
 * All [ref] attributes need an additional reference, because they are
 * pointers, which will be set by the unmarshal function.
 */
bool 
CL4BEUnmarshalFunction::HasAdditionalReference(CBEDeclarator * pDeclarator, 
    bool bCall)
{
    if (CBEUnmarshalFunction::HasAdditionalReference(pDeclarator, 
	    bCall))
        return true;
    // find parameter
    CBETypedDeclarator *pParameter = GetParameter(pDeclarator, bCall);
    if (!pParameter)
        return false;
    // find attribute
    if (pParameter->m_Attributes.Find(ATTR_REF))
        return true;
    return false;
}

/** \brief test if this function has variable sized parameters (needed to \
 *         specify temp + offset var)
 *  \return true if variable sized parameters are needed
 */
bool
CL4BEUnmarshalFunction::HasVariableSizedParameters(int nDirection)
{
    bool bRet = CBEUnmarshalFunction::HasVariableSizedParameters(nDirection);
    // if we have indirect strings to marshal then we need the offset vars
    if (GetParameterCount(ATTR_REF, ATTR_NONE, nDirection))
        return true;
    return bRet;
}

