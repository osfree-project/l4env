/**
 *    \file    dice/src/fe/FEEnumType.cpp
 *    \brief   contains the implementation of the class CFEEnumType
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

#include "fe/FEEnumType.h"
#include "fe/FETaggedEnumType.h"
#include "fe/FEIdentifier.h"
#include "Compiler.h"
#include "File.h"

CFEEnumType::CFEEnumType(vector<CFEIdentifier*> * pMembers)
: CFEConstructedType(TYPE_ENUM)
{
    m_vMembers.swap(*pMembers);
    vector<CFEIdentifier*>::iterator iter;
    for (iter = m_vMembers.begin(); iter != m_vMembers.end(); iter++)
        (*iter)->SetParent(this);
}

CFEEnumType::CFEEnumType(CFEEnumType & src)
: CFEConstructedType(src)
{
    vector<CFEIdentifier*>::iterator iter = src.m_vMembers.begin();
    for (; iter != src.m_vMembers.end(); iter++)
    {
        CFEIdentifier *pNew = (CFEIdentifier*)((*iter)->Clone());
        m_vMembers.push_back(pNew);
        pNew->SetParent(this);
    }
}

/** clean up the enum type (delete the members) */
CFEEnumType::~CFEEnumType()
{
    while (!m_vMembers.empty())
    {
        delete m_vMembers.back();
        m_vMembers.pop_back();
    }
}

/** retrieves a pointer to the first member
 *    \return an iterator, which points to the first member
 */
vector<CFEIdentifier*>::iterator CFEEnumType::GetFirstMember()
{
    return m_vMembers.begin();
}

/** retrieves the next member
 *    \param iter the iterator, which points to the next member
 *    \return a reference to the next memeber
 */
CFEIdentifier *CFEEnumType::GetNextMember(vector<CFEIdentifier*>::iterator &iter)
{
    if (iter == m_vMembers.end())
        return 0;
    return *iter++;
}

/** copies the object
 *    \return a reference to a new enumeration type object
 */
CObject *CFEEnumType::Clone()
{
    return new CFEEnumType(*this);
}

/** \brief checks consitency
 *  \return false if error occurs, true if everything is fine
 *
 * A enum is consistent if it contains at least one member.
 */
bool CFEEnumType::CheckConsistency()
{
    if (!m_vMembers.empty())
        return true;
    // no members:
    CCompiler::GccError(this, 0, "An enum should contain at least one member.");
    return false;
}

/** serializes this object
 *    \param pFile the file to serialize to/from
 */
void CFEEnumType::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        pFile->PrintIndent("<enum_type>\n");
        pFile->IncIndent();
        SerializeMembers(pFile);
        pFile->DecIndent();
        pFile->PrintIndent("</enum_type>\n");
    }
}

/** serializes the members of the enum
 *  \param pFile the file to write to
 */
void CFEEnumType::SerializeMembers(CFile *pFile)
{
    if (pFile->IsStoring())
    {
        vector<CFEIdentifier*>::iterator iter = m_vMembers.begin();
        for (; iter != m_vMembers.end(); iter++)
        {
            pFile->PrintIndent("<member>\n");
            pFile->IncIndent();
            (*iter)->Serialize(pFile);
            pFile->DecIndent();
            pFile->PrintIndent("</member>\n");
        }
    }
}
