/**
 *    \file    dice/src/fe/FETaggedUnionType.cpp
 *    \brief   contains the implementation of the class CFETaggedUnionType
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

#include "fe/FETaggedUnionType.h"
#include "fe/FEFile.h"
#include "File.h"

CFETaggedUnionType::CFETaggedUnionType(string sTag, CFEUnionType * pUnionTypeHeader)
: CFEUnionType(*pUnionTypeHeader)
{
    m_nType = TYPE_TAGGED_UNION;
    m_sTag = sTag;
}

CFETaggedUnionType::CFETaggedUnionType(string sTag)
: CFEUnionType(0)
{
    m_nType = TYPE_TAGGED_UNION;
    m_sTag = sTag;
}

CFETaggedUnionType::CFETaggedUnionType(CFETaggedUnionType & src)
: CFEUnionType(src)
{
    m_sTag = src.m_sTag;
}

/** cleans up the tagged union type object */
CFETaggedUnionType::~CFETaggedUnionType()
{

}

/** clones this object
 *    \return a reference to a new tagged union type object
 */
CObject *CFETaggedUnionType::Clone()
{
    return new CFETaggedUnionType(*this);
}

/** delivers the tag of this class
 *    \return a reference to the tag of this union
 */
string CFETaggedUnionType::GetTag()
{
    return m_sTag;
}

/** \brief checks the integrity of an union
 *  \return true if everything is fine
 *
 * A union is consistent if it has a union body and the elements of this body are
 * consistent.
 */
bool CFETaggedUnionType::CheckConsistency()
{
    if (m_bForwardDeclaration)
    {
        // check if this is an alias
        CFEFile *pRoot = dynamic_cast<CFEFile*>(GetRoot());
        if (pRoot->FindTaggedDecl(GetTag()))
            return true;
    }
    return CFEUnionType::CheckConsistency();
}

/** serialize members of this object
 *    \param pFile the file to serialize to/from
 */
void CFETaggedUnionType::SerializeMembers(CFile *pFile)
{
    *pFile << "\t<tag>" << GetTag() << "</tag>";
    CFEUnionType::SerializeMembers(pFile);
}
