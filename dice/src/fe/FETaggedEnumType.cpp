/**
 *	\file	dice/src/fe/FETaggedEnumType.cpp
 *	\brief	contains the implementation of the class CFETaggedEnumType
 *
 *	\date	03/22/2001
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

#include "fe/FETaggedEnumType.h"
#include "fe/FETypedDeclarator.h"

IMPLEMENT_DYNAMIC(CFETaggedEnumType) 

CFETaggedEnumType::CFETaggedEnumType(String sTag, Vector * pMembers)
:CFEEnumType(pMembers)
{
    IMPLEMENT_DYNAMIC_BASE(CFETaggedEnumType, CFEEnumType);

    m_nType = TYPE_TAGGED_ENUM;
    m_sTag = sTag;
}

CFETaggedEnumType::CFETaggedEnumType(CFETaggedEnumType & src)
:CFEEnumType(src)
{
    IMPLEMENT_DYNAMIC_BASE(CFETaggedEnumType, CFEEnumType);
    m_sTag = src.m_sTag;
}

/** cleans up the tagge struct object */
CFETaggedEnumType::~CFETaggedEnumType()
{

}

/** \brief retrieves the tag
 *	\return the tag of the enum
 */
String CFETaggedEnumType::GetTag()
{
    return m_sTag;
}

/** \brief copys the enum object
 *	\return a referenced to a new tagged enum object
 */
CObject *CFETaggedEnumType::Clone()
{
    return new CFETaggedEnumType(*this);
}

/** \brief checks if this type is consistent
 *  \return true if it is
 *
 * Usually an enum should have at least one member. The exception is,
 * if it is child of a typedef (typedef enum &lt;tag&gt; &lt;alias&gt;)
 */
bool CFETaggedEnumType::CheckConsistency()
{
    if (m_pMembers)
    {
        if (m_pMembers->GetSize() > 0)
            return true;
    }
    // no members
    if (GetParent()->IsKindOf(RUNTIME_CLASS(CFETypedDeclarator)))
        return true;
    // no typedef
    CCompiler::GccError(this, 0, "An enum should contain at least one member.");
    return false;
}
