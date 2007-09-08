/**
 *  \file   dice/src/fe/FETypeOfType.cpp
 *  \brief  contains the implementation of the class CFETypeOfType
 *
 *  \date   08/07/2007
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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

#include "fe/FETypeOfType.h"
#include "fe/FEExpression.h"

CFETypeOfType::CFETypeOfType(CFEExpression *pExpression)
  :  CFEConstructedType(TYPE_TYPEOF)
{
    m_pExpression = pExpression;
}

CFETypeOfType::CFETypeOfType(CFETypeOfType* src)
  :  CFEConstructedType(src)
{
	CLONE_MEM(CFEExpression, m_pExpression);
}

/** cleans up the pipe type object */
CFETypeOfType::~CFETypeOfType()
{
	if (m_pExpression)
		delete m_pExpression;
}

/** \brief create a copy of this object
 *  \return reference to clone
 */
CObject* CFETypeOfType::Clone()
{
	return new CFETypeOfType(this);
}
