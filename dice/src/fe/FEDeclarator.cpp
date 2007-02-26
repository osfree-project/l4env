/**
 *    \file    dice/src/fe/FEDeclarator.cpp
 *  \brief   contains the implementation of the class CFEDeclarator
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

#include "fe/FEDeclarator.h"
#include "File.h"

CFEDeclarator::CFEDeclarator(CFEDeclarator & src)
:CFEIdentifier(src)
{
    m_nType = src.m_nType;
    m_nNumStars = src.m_nNumStars;
    m_nBitfields = src.m_nBitfields;
}

CFEDeclarator::CFEDeclarator(DECL_TYPE nType)
:CFEIdentifier(string())
{
    m_nType = nType;
    m_nNumStars = 0;
    m_nBitfields = 0;
}

CFEDeclarator::CFEDeclarator(DECL_TYPE nType, string sName, int nNumStars, int nBitfields)
:CFEIdentifier(sName)
{
    m_nType = nType;
    m_nNumStars = nNumStars;
    m_nBitfields = nBitfields;
}

/** cleans up the declarator */
CFEDeclarator::~CFEDeclarator()
{
    // nothing to clean up
}

/** returns the number of asterisks
 *  \return the number of asterisks
 */
int CFEDeclarator::GetStars()
{
    return m_nNumStars;
}

/** returns the type of the declarator
 *  \return the type of the declarator
 */
DECL_TYPE CFEDeclarator::GetType()
{
    return m_nType;
}

/** sets the number of stars for this declarator
 *  \param nNumStars the new number of stars
 */
void CFEDeclarator::SetStars(int nNumStars)
{
    m_nNumStars = nNumStars;
}

/** creates a copy of this object
 *  \return a reference to the copy of this object
 */
CObject *CFEDeclarator::Clone()
{
    return new CFEDeclarator(*this);
}

/** \brief returns the bitfields value
 *  \return the bitfields value
 *
 * This functionr eturns the number of bit fields the declarator uses
 */
int CFEDeclarator::GetBitfields()
{
    return m_nBitfields;
}

/** \brief sets the number of bitfields
 *  \param nBitfields the number of bit-fields
 *
 * This function sets the number of bits this declarator uses.
 */
void CFEDeclarator::SetBitfields(int nBitfields)
{
    m_nBitfields = nBitfields;
}

/** \brief changes the type of the declarator
 *  \param nNewType the new type of the declarator
 */
void CFEDeclarator::SetType(DECL_TYPE nNewType)
{
    m_nType = nNewType;
}
