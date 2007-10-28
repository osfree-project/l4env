/**
 *  \file    dice/src/be/l4/L4BEUnmarshalFunction.cpp
 *  \brief   contains the implementation of the class CL4BEUnmarshalFunction
 *
 *  \date    02/20/2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2001-2007
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
#include "fe/FETypedDeclarator.h"

#include "TypeSpec-Type.h"
#include "Attribute-Type.h"
#include <cassert>

CL4BEUnmarshalFunction::CL4BEUnmarshalFunction()
{ }

/** \brief destructor of target class */
CL4BEUnmarshalFunction::~CL4BEUnmarshalFunction()
{ }

/** \brief test if this function has variable sized parameters (needed to \
 *         specify temp + offset var)
 *  \return true if variable sized parameters are needed
 */
bool CL4BEUnmarshalFunction::HasVariableSizedParameters(DIRECTION_TYPE nDirection)
{
	bool bRet = CBEUnmarshalFunction::HasVariableSizedParameters(nDirection);
	// if we have indirect strings to marshal then we need the offset vars
	if (HasParameterWithAttributes(ATTR_REF, (nDirection == DIRECTION_IN) ? ATTR_IN : ATTR_OUT))
		return true;
	return bRet;
}

/** \brief adds a single parameter to this function
 *  \param pFEParameter the parameter to add
 */
void CL4BEUnmarshalFunction::AddParameter(CFETypedDeclarator * pFEParameter)
{
	CBEUnmarshalFunction::AddParameter(pFEParameter);
	// retrieve the parameter
	CBETypedDeclarator* pParameter = m_Parameters.Find(pFEParameter->m_Declarators.First()->GetName());
	// base class can have decided to skip parameter
	if (!pParameter)
		return;
	// find attribute
	CBEDeclarator *pDeclarator = pParameter->m_Declarators.First();
	if (pParameter->m_Attributes.Find(ATTR_REF) && (pDeclarator->GetStars() == 0))
		pDeclarator->IncStars(1);
}
