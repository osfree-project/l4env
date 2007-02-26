/**
 *	\file	dice/src/be/BEEnumType.cpp
 *	\brief	contains the implementation of the class CBEEnumType
 *
 *	\date	Tue Jul 23 2002
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

#include "be/BEEnumType.h"
#include "be/BEFile.h"
#include "fe/FEEnumType.h"
#include "fe/FETaggedEnumType.h"

IMPLEMENT_DYNAMIC(CBEEnumType);

CBEEnumType::CBEEnumType()
{
    IMPLEMENT_DYNAMIC_BASE(CBEEnumType, CBEType);
    m_pMembers = 0;
}

/** destroys the enum type */
CBEEnumType::~CBEEnumType()
{
    for (int i=0; i<m_nMemberCount; i++)
        delete m_pMembers[i];
    free(m_pMembers);
}

/** \brief adds a member to the enum
 *  \param sMember the member to add
 */
void CBEEnumType::AddMember(String sMember)
{
    m_pMembers = (String**)realloc(m_pMembers, (m_nMemberCount+1)*sizeof(String*));
    m_pMembers[m_nMemberCount] = new String(sMember);
    m_nMemberCount++;
}

/** \brief removes a specific member from the enum list
 *  \param sMember the member to remove
 *
 * Since we add and remove string, we can only remove a string, when strings in the list are equal
 * to the given string. We will also remove _ALL_ occurences of the string.
 */
void CBEEnumType::RemoveMember(String sMember)
{
    for (int i=0; i<m_nMemberCount; i++)
    {
        if (m_pMembers[i])
        {
            // is current member the same
            if (GetMemberAt(i) == sMember)
            {
                // delete current member
                delete m_pMembers[i];
                // move successors down
                for (int j=i; j<m_nMemberCount-1; j++)
                    m_pMembers[j] = m_pMembers[j+1];
                // decrease size of array and
                // decrement member count (we are one less now)
                m_pMembers = (String**)realloc(m_pMembers, (--m_nMemberCount)*sizeof(String*));
                // decrement i, because it will be incremented after this run and we
                // want to test the "new" member at position 'i' as well
                i--;
            }
        }
    }
}

/** \brief accesses a member at the given position
 *  \param nIndex the position in the array (zero based)
 *  \return the requested string
 */
String CBEEnumType::GetMemberAt(int nIndex)
{
    if ((nIndex < 0) || (nIndex >= m_nMemberCount))
        return String();
    if (!(m_pMembers[nIndex]))
        return String();
    return *(m_pMembers[nIndex]);
}

/** \brief retrieves the size of the array
 *  \return the number of elements
 */
int CBEEnumType::GetMemberCount()
{
    return m_nMemberCount;
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
    VectorElement *pIter =  pFEEnumType->GetFirstMember();
    CFEIdentifier *pFEMember;
    while ((pFEMember = pFEEnumType->GetNextMember(pIter)) != 0)
    {
        AddMember(pFEMember->GetName());
    }
    // check tagged
    if (pFEEnumType->IsKindOf(RUNTIME_CLASS(CFETaggedEnumType)))
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
    pFile->Print("%s", (const char *) m_sName);	// should be set to "enum"
    if (!m_sTag.IsEmpty())
        pFile->Print(" %s", (const char *) m_sTag);
    pFile->PrintIndent(" { ");
    // print members
    int nMax = GetMemberCount();
    for (int nCurr = 0; nCurr < nMax; nCurr++)
    {
        pFile->Print("%s", (const char*)GetMemberAt(nCurr));
        if (nCurr < nMax-1)
            pFile->Print(", ");
    }
    // close enum
    pFile->PrintIndent(" }");
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
    if ((m_nMemberCount <= 0) || (!m_pMembers))
    {
        pFile->Print("0");
        return;
    }
    int nCurr = 0;
    while (!(m_pMembers[nCurr]) && (nCurr < m_nMemberCount)) nCurr++;
    if (nCurr == m_nMemberCount)
    {
        pFile->Print("0");
        return;
    }
    pFile->Print("%s", (const char*)*(m_pMembers[nCurr]));
}

/** \brief tests if the enum type has the given tag
 *  \param sTag the tag to test for
 *  \return true if the given tag is the same as the local tag
 */
bool CBEEnumType::HasTag(String sTag)
{
    return (m_sTag == sTag);
}
