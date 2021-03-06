/**
 *  \file    dice/src/be/l4/L4BETypedDeclarator.cpp
 *  \brief   contains the implementation of the class CL4BETypedDeclarator
 *
 *  \date    07/17/2002
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

#include "be/l4/L4BETypedDeclarator.h"
#include "TypeSpec-Type.h"
#include "Attribute-Type.h"
#include "be/BEType.h"
#include "be/BEDeclarator.h"
#include "be/BETypedDeclarator.h"
#include "be/BEFile.h"
#include "be/BESizes.h"
#include "Compiler.h"

/** destroys the typed declarator object */
CL4BETypedDeclarator::~CL4BETypedDeclarator()
{}

CL4BETypedDeclarator::CL4BETypedDeclarator()
{ }

CL4BETypedDeclarator::CL4BETypedDeclarator(CL4BETypedDeclarator* src)
: CBETypedDeclarator(src)
{ }

/** \brief create a copy of this object
 *  \return reference to clone
 */
CL4BETypedDeclarator* CL4BETypedDeclarator::Clone()
{
	return new CL4BETypedDeclarator(this);
}

/** \brief check if this typed decl is regarded as variable sized
 *  \return true if it is
 *
 * Even though, only the marshaller class should decide which marshalling
 * strategy it should use for each parameter, this decision is also
 * distributed accross the whole back-end. This function has influence on the
 * decision whether the message buffer is variable sized or fixed sized. Which
 * again influences the way the parameters and especially this parameter is
 * marshalled. Therefore we have to be careful about which policies to
 * distribute across the back-end.
 *
 * This implementation first tests for ref-strings (the [string] attribute is
 * optional and determines the use of strlen). Refstrings - in contrary to
 * "normal" strings - are not marshalled as variable sized arrays, but as
 * fixed sized value-pair of start-address and size.  Therefore they are not
 * variable sized. If it is not a refstring, we ask the base class.
 */
bool CL4BETypedDeclarator::IsVariableSized()
{
	if (m_Attributes.Find(ATTR_REF))
		return false;
	return CBETypedDeclarator::IsVariableSized();
}

/** \brief checks if this parameter is of fixed size
 *  \return true if it is of fixed size
 *
 * This implementation does not count a ref-string as fixed sized parameter,
 * even though it is fixed in size. This is because the ref-strings are not
 * marshalled into the fixed sized elements buffer.
 */
bool CL4BETypedDeclarator::IsFixedSized()
{
	if (m_Attributes.Find(ATTR_REF))
		return false;
	return CBETypedDeclarator::IsFixedSized();
}

/** \brief calculates the max size of the paramater
 *  \param nSize the size to set in bytes
 *  \param sName the name of a specific declarator or empty if all
 *  \return true if value has been assigned
 *
 * If bGuessSize is false, then this is for message buffer size calculation
 * and we have to check for [ref] attribute.
 */
bool CL4BETypedDeclarator::GetMaxSize(int & nSize, std::string sName)
{
	CCompiler::Verbose("CL4BETypedDeclarator::%s(, \"%s\") @ %p called\n", __func__, sName.c_str(), this);

	if (m_Attributes.Find(ATTR_REF))
	{
		CBESizes *pSizes = CCompiler::GetSizes();
		nSize = pSizes->GetSizeOfType(TYPE_REFSTRING);
		return true;
	}
	return CBETypedDeclarator::GetMaxSize(nSize, sName);
}
