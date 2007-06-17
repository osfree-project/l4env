/**
 *  \file    dice/src/fe/FEEnumDeclarator.cpp
 *  \brief   contains the implementation of the class CFEEnumDeclarator
 *
 *  \date    06/08/2001
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

#include "fe/FEEnumDeclarator.h"
#include "fe/FEExpression.h"

CFEEnumDeclarator::CFEEnumDeclarator(CFEEnumDeclarator & src)
:CFEDeclarator(src)
{
    m_pInitialValue = (CFEExpression *) (src.m_pInitialValue->Clone());
}

CFEEnumDeclarator::CFEEnumDeclarator()
:CFEDeclarator(DECL_ENUM, string())
{
    m_pInitialValue = 0;
}

CFEEnumDeclarator::CFEEnumDeclarator(string sName, CFEExpression * pInitialValue)
:CFEDeclarator(DECL_ENUM, sName)
{
    m_pInitialValue = pInitialValue;
    if (m_pInitialValue)
    m_pInitialValue->SetParent(this);
}

/** cleans up the declarator object */
CFEEnumDeclarator::~CFEEnumDeclarator()
{
    if (m_pInitialValue)
    delete m_pInitialValue;
}

/** \brief creates a copy of this object
 *  \return a reference to a new object
 */
CObject *CFEEnumDeclarator::Clone()
{
    return new CFEEnumDeclarator(*this);
}

/** \brief retrieves a reference to the initial value of the enum
 *  \return a reference to the initial value of the enum
 */
CFEExpression* CFEEnumDeclarator::GetInitialValue()
{
    return m_pInitialValue;
}
