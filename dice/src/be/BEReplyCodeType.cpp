/**
 *	\file	dice/src/be/BEReplyCodeType.cpp
 *	\brief	contains the implementation of the class CBEReplyCodeType
 *
 *	\date	10/10/2003
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

#include "BEReplyCodeType.h"
#include "be/BEContext.h"

#include "TypeSpec-Type.h"

IMPLEMENT_DYNAMIC(CBEReplyCodeType);

CBEReplyCodeType::CBEReplyCodeType()
{
    IMPLEMENT_DYNAMIC_BASE(CBEReplyCodeType, CBEType);
}

CBEReplyCodeType::CBEReplyCodeType(CBEReplyCodeType & src):CBEType(src)
{
    IMPLEMENT_DYNAMIC_BASE(CBEReplyCodeType, CBEType);
}

/**	\brief destructor of this instance */
CBEReplyCodeType::~CBEReplyCodeType()
{

}

/**	\brief creates the back-end structure for a type class for opcodes
 *	\param pContext the context of the code generation
 *	\return true if code generation was successful
 *
 * This implementation sets the basic members, but uses special values, specific for opcodes.
 */
bool CBEReplyCodeType::CreateBackEnd(CBEContext * pContext)
{
    m_bUnsigned = false;
    m_nSize = 2;    // bytes
    m_nFEType = TYPE_INTEGER;
    m_sName = pContext->GetNameFactory()->GetTypeName(m_nFEType, false, pContext, m_nSize);
    return true;
}

/**	\brief generates an exact copy of this class
 *	\return a reference to the new object
 */
CObject *CBEReplyCodeType::Clone()
{
    return new CBEReplyCodeType(*this);
}
