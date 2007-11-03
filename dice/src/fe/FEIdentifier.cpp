/**
 *    \file    dice/src/fe/FEIdentifier.cpp
 *  \brief   contains the implementation of the class CFEIdentifier
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

#include "fe/FEIdentifier.h"
#include <string>

CFEIdentifier::CFEIdentifier()
{ }

CFEIdentifier::CFEIdentifier(std::string sName)
{
	m_sName = sName;
}

CFEIdentifier::CFEIdentifier(const char *sName)
{
	m_sName = sName;
}

CFEIdentifier::CFEIdentifier(CFEIdentifier* src)
: CFEBase(src)
{
	m_sName = src->m_sName;
}

/** cleans up the identifier object (frres the name) */
CFEIdentifier::~CFEIdentifier()
{ }

/** \brief create a copy of this object
 *  \return a reference to the clone
 */
CFEIdentifier* CFEIdentifier::Clone()
{
	return new CFEIdentifier(this);
}

/** checks this object for equality with another identifier
 *  \param src the other identifier to compare with
 *  \return true if the two std::string are identical
 * The two std::string are identical if both are 0 or both are the same compared with
 * the strcmp function. They are not the same if only one of them is 0 or if the
 * strings are different.
 */
bool CFEIdentifier::operator ==(CFEIdentifier & src)
{
	return (m_sName == src.m_sName);
}

/** checks this object for equality with a string
 *  \param sName the string to compare with
 *  \return true if the string an this object are equal
 * See above function for definition of equal.
 */
bool CFEIdentifier::operator ==(std::string & sName)
{
	return (m_sName == sName);
}

/** retrieves the character string from the object
 *  \return a reference to the member string
 * If you intend to modify this string, please copy it beforehand.
 */
std::string CFEIdentifier::GetName()
{
	return m_sName;
}

/** return true if the given name matches the internally stored value
 *  \param sName the name to compare against
 *  \return true if names match
 */
bool CFEIdentifier::Match(std::string sName)
{
	return m_sName == sName;
}

/** prefixes the identifier with the string
 *  \param sPrefix the string to prefix
 */
void CFEIdentifier::Prefix(std::string sPrefix)
{
	m_sName = sPrefix + m_sName;
}

/** suffixes the identifier with the string
 *  \param sSuffix the string to suffix
 */
void CFEIdentifier::Suffix(std::string sSuffix)
{
	m_sName += sSuffix;
}

/** \brief exchanges the names of this identifier
 *  \param sNewName the new name
 *  \return the old name
 *
 * This function sounds like none-sense: why replace the name - the identifier wouldn't
 * be the same. Well, because we can Clone all front-end objects and some objects are
 * derived from CFEIdentifier, we might want to keep the attributes from the derived
 * objects, but replace the name when we sometime Clone.
 */
std::string CFEIdentifier::ReplaceName(std::string sNewName)
{
	std::string ret = m_sName;
	m_sName = sNewName;
	return ret;
}
