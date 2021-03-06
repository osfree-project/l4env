/**
 *    \file    dice/src/fe/FEConstDeclarator.cpp
 *    \brief   contains the implementation of the class CFEConstDeclarator
 *
 *    \date    01/31/2001
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

#include "FEConstDeclarator.h"
#include "FEUserDefinedType.h"
#include "FETypedDeclarator.h"
#include "FEExpression.h"
#include "FEFile.h"
#include "Compiler.h"
#include "Visitor.h"
#include <iostream>
#include <cassert>

CFEConstDeclarator::CFEConstDeclarator(CFETypeSpec * pConstType,
	std::string sConstName,
	CFEExpression * pConstValue)
: CFEInterfaceComponent(static_cast<CObject*>(0)),
	m_pConstType(pConstType),
	m_sConstName(sConstName),
	m_pConstValue(pConstValue)
{ }

CFEConstDeclarator::CFEConstDeclarator(CFEConstDeclarator* src)
: CFEInterfaceComponent(src)
{
	m_sConstName = src->m_sConstName;
	CLONE_MEM(CFETypeSpec, m_pConstType);
	CLONE_MEM(CFEExpression, m_pConstValue);
}

/** cleans up the constant declarator (frees all members) */
CFEConstDeclarator::~CFEConstDeclarator()
{
	if (m_pConstType)
		delete m_pConstType;
	if (m_pConstValue)
		delete m_pConstValue;
}

/** \brief create a copy of this object
 *  \return reference to clone
 */
CFEConstDeclarator* CFEConstDeclarator::Clone()
{
	return new CFEConstDeclarator(this);
}

/** returns the type of the constant
 *  \return the type of the constant
 */
CFETypeSpec *CFEConstDeclarator::GetType()
{
	return m_pConstType;
}

/** returns the name of the constant
 *  \return the name of the constant
 */
string CFEConstDeclarator::GetName()
{
	return m_sConstName;
}

/** \brief tries to match the internal name with the given argument
 *  \param sName the name to match
 *  \return true if matches, false otherwise
 */
bool CFEConstDeclarator::Match(std::string sName)
{
	return GetName() == sName;
}

/** returns the value (the expression) of the constant
 *  \return the value (the expression) of the constant
 */
CFEExpression *CFEConstDeclarator::GetValue()
{
	return m_pConstValue;
}

/** \brief accepts the iteration of the visitors
 *  \param v reference to the visitor
 */
void CFEConstDeclarator::Accept(CVisitor& v)
{
	v.Visit(*this);
}
