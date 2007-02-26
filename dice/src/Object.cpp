/**
 *	\file	dice/src/Object.cpp
 *	\brief	contains the implementation of the class CObject
 *
 *	\date	01/31/2001
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

#include "Object.h"
#include "Vector.h"

IMPLEMENT_DYNAMIC(CObject)

CObject::CObject(CObject * pParent)
{
    m_pParent = pParent;
}

CObject::CObject(CObject & src)
{
    m_pParent = src.m_pParent;
}

/** cleans up the object */
CObject::~CObject()
{

}

/**	retrieves a reference to the parent object
 *	\return a reference to the parent object
 */
CObject *CObject::GetParent()
{
    return m_pParent;
}

/** sets the new parent object
 *	\param pParent a reference to the new parent object
 */
void CObject::SetParent(CObject * pParent)
{
    m_pParent = pParent;
}

/**	creates a copy of this object
 *	\return a copy of this object
 */
CObject *CObject::Clone()
{
    return new CObject(*this);
}
