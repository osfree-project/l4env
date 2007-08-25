/**
 *  \file    dice/src/Object.cpp
 *  \brief   contains the implementation of the class CObject
 *
 *  \date    01/31/2001
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

#include "Object.h"

CObject::CObject(CObject * pParent)
    : m_sourceLoc(),
    m_pParent(pParent)
{ }

CObject::CObject(const CObject & src)
    : m_sourceLoc(src.m_sourceLoc),
    m_pParent(src.m_pParent)
{ }

/** cleans up the object */
CObject::~CObject()
{ }

/** \brief creates a copy of this object
 *  \return a copy of this object
 */
CObject *CObject::Clone()
{
    return new CObject(*this);
}

/** \brief accept function for visitors
 */
void CObject::Accept(CVisitor&)
{ }

/** \brief test if an object is parent of this object
 *  \param pParent the alleged parent
 *  \return true if pParent really is my parent
 */
bool CObject::IsParent(CObject * pParent)
{
    if (!pParent)
	return false;
    CObject *pP = m_pParent;
    while (pP && pP != pParent)
	pP = pP->GetParent();
    return pP == pParent;
}

