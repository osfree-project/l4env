/**
 *	\file	dice/src/be/BEException.cpp
 *	\brief	contains the implementation of the class CBEException
 *
 *	\date	01/15/2002
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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

#include "be/BEException.h"
#include "be/BEContext.h"

#include "fe/FEIdentifier.h"

IMPLEMENT_DYNAMIC(CBEException);

CBEException::CBEException()
{
    IMPLEMENT_DYNAMIC_BASE(CBEException, CBEObject);
}

CBEException::CBEException(CBEException & src):CBEObject(src)
{
    m_sName = src.m_sName;
    IMPLEMENT_DYNAMIC_BASE(CBEException, CBEObject);
}

/**	\brief destructor of this instance */
CBEException::~CBEException()
{
}

/**	\brief prepares the target code generation for this element
 *	\param pFEException the respective front-end exception
 *	\param pContext the context of the code generation
 *	\return true if code generationwas successful
 */
bool CBEException::CreateBackEnd(CFEIdentifier * pFEException, CBEContext * pContext)
{
    m_sName = pFEException->GetName();
    return true;
}
