/**
 *  \file   dice/src/be/BEReplyCodeType.cpp
 *  \brief  contains the implementation of the class CBEReplyCodeType
 *
 *  \date   10/10/2003
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "BEReplyCodeType.h"
#include "be/BEContext.h"
#include "BENameFactory.h"
#include "Compiler.h"
#include "TypeSpec-Type.h"

CBEReplyCodeType::CBEReplyCodeType()
{ }

/** \brief destructor of this instance */
CBEReplyCodeType::~CBEReplyCodeType()
{ }

CBEReplyCodeType::CBEReplyCodeType(CBEReplyCodeType* src)
: CBEType(src)
{ }

/** \brief create a copy of this object
 *  \return reference to clone
 */
CObject* CBEReplyCodeType::Clone()
{
	return new CBEReplyCodeType(this);
}

/** \brief creates the back-end structure for a type class for opcodes
 *  \return true if code generation was successful
 *
 * This implementation sets the basic members, but uses special values,
 * specific for opcodes.
 */
void CBEReplyCodeType::CreateBackEnd()
{
	CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL,
		"CBEReplyCodeType::%s() called\n", __func__);

	m_bUnsigned = false;
	m_nSize = 2;    // bytes
	m_nFEType = TYPE_INTEGER;
	CBENameFactory *pNF = CBENameFactory::Instance();
	m_sName = pNF->GetTypeName(m_nFEType, false, m_nSize);

	CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL,
		"CBEReplyCodeType::%s() returns\n", __func__);
}

