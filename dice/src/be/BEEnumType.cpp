/**
 *  \file    dice/src/be/BEEnumType.cpp
 *  \brief   contains the implementation of the class CBEEnumType
 *
 *  \date    Tue Jul 23 2002
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

#include "BEEnumType.h"
#include "BEFile.h"
#include "BETypedef.h"
#include "BEDeclarator.h"
#include "Compiler.h"
#include "fe/FEEnumType.h"
#include "fe/FEIdentifier.h"
#include <cassert>

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
 * Since we add and remove string, we can only remove a string, when strings
 * in the list are equal to the given string. We will also remove _ALL_
 * occurences of the string.
 */
void CBEEnumType::RemoveMember(string sMember)
{
    vector<string>::iterator iter = std::find(m_vMembers.begin(),
	m_vMembers.end(), sMember);
    if (iter != m_vMembers.end())
	m_vMembers.erase(iter);
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
 *  \return true if successful
 */
void
CBEEnumType::CreateBackEnd(CFETypeSpec *pFEType)
{
    CCompiler::VerboseI(PROGRAM_VERBOSE_NORMAL, "CBEEnumType::%s(fe) called\n", 
	__func__);
    
    CBEType::CreateBackEnd(pFEType);

    // extract members
    CFEEnumType *pFEEnumType = (CFEEnumType*)pFEType;
    vector<CFEIdentifier*>::iterator iterI;
    for (iterI =  pFEEnumType->m_Members.begin();
	 iterI != pFEEnumType->m_Members.end();
	 iterI++)
    {
        AddMember((*iterI)->GetName());
    }
    // check tagged
    m_sTag = pFEEnumType->GetTag();

    CCompiler::VerboseD(PROGRAM_VERBOSE_NORMAL, "CBEEnumType::%s(fe) returns\n",
	__func__);
}

/** \brief writes the enum type to the target file
 *  \param pFile the file to write to
 */
void CBEEnumType::Write(CBEFile& pFile)
{
    if (!pFile.is_open())
        return;

    // open enum
    pFile << m_sName;
    if (!m_sTag.empty())
	pFile << " " << m_sTag;
    // only print member if we got some
    unsigned int nMax = m_vMembers.size();
    if (nMax > 0)
    {
	pFile << "\t { ";
        // print members
        for (unsigned int nCurr = 0; nCurr < nMax; nCurr++)
        {
            pFile << m_vMembers[nCurr];
            if (nCurr < nMax-1)
                pFile << ", ";
        }
        // close enum
	pFile << "\t } ";
    }
}

/** \brief write the initialization of a enum with the 'zero' element
 *  \param pFile the file to write to
 *
 *  The 'zero' or initial element of an enum is associated with its declarator. Therefore we could
 * only guess. What we can do is to use the first element of the enum.
 */
void CBEEnumType::WriteZeroInit(CBEFile& pFile)
{
    if (m_vMembers.empty())
    {
	pFile << "0";
        return;
    }
    pFile << m_vMembers[0];
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
 *
 * A enum cast is '(enum tag)'.
 */
void CBEEnumType::WriteCast(CBEFile& pFile,  bool bPointer)
{
    pFile << "(";
    if (m_sTag.empty())
    {
        // no tag -> we need a typedef to save us
        // the alias can be used for the cast
        CBETypedef *pTypedef = GetTypedef();
        assert(pTypedef);
        // get first declarator (without stars)
        vector<CBEDeclarator*>::iterator iterD;
	for (iterD = pTypedef->m_Declarators.begin();
	     iterD != pTypedef->m_Declarators.end();
	     iterD++)
        {
            if ((*iterD)->GetStars() <= (bPointer?1:0))
                break;
        }
        assert(iterD != pTypedef->m_Declarators.end());
	pFile << (*iterD)->GetName();
        if (bPointer && ((*iterD)->GetStars() == 0))
	    pFile << "*";
    }
    else
    {
	pFile << m_sName << " " << m_sTag;
        if (bPointer)
	    pFile << "*";
    }
    pFile << ")";
}
