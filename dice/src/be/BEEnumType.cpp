/**
 *    \file    dice/src/be/BEEnumType.cpp
 *    \brief   contains the implementation of the class CBEEnumType
 *
 *    \date    Tue Jul 23 2002
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

#include "be/BEEnumType.h"
#include "be/BEFile.h"
#include "be/BETypedef.h"
#include "be/BEDeclarator.h"

#include "fe/FEEnumType.h"
#include "fe/FETaggedEnumType.h"
#include "fe/FEIdentifier.h"

CBEEnumType::CBEEnumType()
{
    m_vMembers.clear();
}

/** destroys the enum type */
CBEEnumType::~CBEEnumType()
{
    m_vMembers.clear();
}

/** \brief adds a member to the enum
 *  \param sMember the member to add
 */
void CBEEnumType::AddMember(string sMember)
{
    m_vMembers.push_back(sMember);
}

/** \brief removes a specific member from the enum list
 *  \param sMember the member to remove
 *
 * Since we add and remove string, we can only remove a string, when strings in the list are equal
 * to the given string. We will also remove _ALL_ occurences of the string.
 */
void CBEEnumType::RemoveMember(string sMember)
{
    vector<string>::iterator iter;
    for (iter = m_vMembers.begin(); iter != m_vMembers.end(); iter++)
    {
        if (*iter == sMember)
            m_vMembers.erase(iter);
    }
}

/** \brief accesses a member at the given position
 *  \param nIndex the position in the array (zero based)
 *  \return the requested string
 */
string CBEEnumType::GetMemberAt(unsigned int nIndex)
{
    if (nIndex >= m_vMembers.size())
        return string();
    return m_vMembers[nIndex];
}

/** \brief retrieves the size of the array
 *  \return the number of elements
 */
int CBEEnumType::GetMemberCount()
{
    return m_vMembers.size();
}

/** \brief creates the enum type
 *  \param pFEType the front-end type to use as reference
 *  \param pContext the context of the create operation
 *  \return true if successful
 */
bool CBEEnumType::CreateBackEnd(CFETypeSpec *pFEType, CBEContext *pContext)
{
    if (!CBEType::CreateBackEnd(pFEType, pContext))
        return false;

    // extract members
    CFEEnumType *pFEEnumType = (CFEEnumType*)pFEType;
    vector<CFEIdentifier*>::iterator iterI =  pFEEnumType->GetFirstMember();
    CFEIdentifier *pFEMember;
    while ((pFEMember = pFEEnumType->GetNextMember(iterI)) != 0)
    {
        AddMember(pFEMember->GetName());
    }
    // check tagged
    if (dynamic_cast<CFETaggedEnumType*>(pFEEnumType))
        m_sTag = ((CFETaggedEnumType*)pFEEnumType)->GetTag();
    // return true
    return true;
}

/** \brief writes the enum type to the target file
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 */
void CBEEnumType::Write(CBEFile * pFile, CBEContext * pContext)
{
    if (!pFile->IsOpen())
        return;

    // open enum
    pFile->Print("%s", m_sName.c_str());    // should be set to "enum"
    if (!m_sTag.empty())
        pFile->Print(" %s", m_sTag.c_str());
    // only print member if we got some
    unsigned int nMax = m_vMembers.size();
    if (nMax > 0)
    {
        pFile->PrintIndent(" { ");
        // print members
        for (unsigned int nCurr = 0; nCurr < nMax; nCurr++)
        {
            *pFile << m_vMembers[nCurr];
            if (nCurr < nMax-1)
                *pFile << ", ";
        }
        // close enum
        pFile->PrintIndent(" }");
    }
}

/** \brief write the initialization of a enum with the 'zero' element
 *  \param pFile the file to write to
 *  \param pContext the context of the write operation
 *
 *  The 'zero' or initial element of an enum is associated with its declarator. Therefore we could
 * only guess. What we can do is to use the first element of the enum.
 */
void CBEEnumType::WriteZeroInit(CBEFile * pFile, CBEContext * pContext)
{
    if (m_vMembers.empty())
    {
        pFile->Print("0");
        return;
    }
    *pFile << m_vMembers[0];
}

/** \brief tests if the enum type has the given tag
 *  \param sTag the tag to test for
 *  \return true if the given tag is the same as the local tag
 */
bool CBEEnumType::HasTag(string sTag)
{
    return (m_sTag == sTag);
}

/** \brief writes a cast of this type
 *  \param pFile the file to write to
 *  \param bPointer true if the cast should produce a pointer
 *  \param pContext the context of the write operation
 *
 * A enum cast is '(enum tag)'.
 */
void CBEEnumType::WriteCast(CBEFile* pFile,  bool bPointer,  CBEContext* pContext)
{
    pFile->Print("(");
    if (m_sTag.empty())
    {
        // no tag -> we need a typedef to save us
        // the alias can be used for the cast
        CBETypedef *pTypedef = GetTypedef();
        assert(pTypedef);
        // get first declarator (without stars)
        vector<CBEDeclarator*>::iterator iterD = pTypedef->GetFirstDeclarator();
        CBEDeclarator *pDecl;
        while ((pDecl = pTypedef->GetNextDeclarator(iterD)) != 0)
        {
            if (pDecl->GetStars() <= (bPointer?1:0))
                break;
        }
        assert(pDecl);
        pFile->Print("%s", pDecl->GetName().c_str());
        if (bPointer && (pDecl->GetStars() == 0))
            pFile->Print("*");
    }
    else
    {
        pFile->Print("%s %s", m_sName.c_str(), m_sTag.c_str());
        if (bPointer)
            pFile->Print("*");
    }
    pFile->Print(")");
}
