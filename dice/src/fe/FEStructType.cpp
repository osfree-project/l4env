/**
 *    \file    dice/src/fe/FEStructType.cpp
 *    \brief   contains the implementation of the class CFEStructType
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

#include "fe/FEStructType.h"
#include "fe/FETaggedStructType.h"
#include "File.h"

CFEStructType::CFEStructType(vector<CFETypedDeclarator*> * pMembers)
: CFEConstructedType(TYPE_STRUCT)
{
    if (pMembers)
        m_vMembers.swap(*pMembers);
    else
        m_bForwardDeclaration = true;
    vector<CFETypedDeclarator*>::iterator iter;
    for (iter = m_vMembers.begin(); iter != m_vMembers.end(); iter++)
    {
        (*iter)->SetParent(this);
    }
}

CFEStructType::CFEStructType(CFEStructType & src)
: CFEConstructedType(src)
{
    vector<CFETypedDeclarator*>::iterator iter = src.m_vMembers.begin();
    for (; iter != src.m_vMembers.end(); iter++)
    {
        CFETypedDeclarator *pNew = (CFETypedDeclarator*)((*iter)->Clone());
        m_vMembers.push_back(pNew);
        pNew->SetParent(this);
    }
}

/** cleans up the struct object (delete all members) */
CFEStructType::~CFEStructType()
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
vector<CFETypedDeclarator*>::iterator CFEStructType::GetFirstMember()
{
    return m_vMembers.begin();
}

/** retrieves the next member of the struct
 *    \param iter the iterator, which points to the next member
 *    \return the next member object
 */
CFETypedDeclarator *CFEStructType::GetNextMember(vector<CFETypedDeclarator*>::iterator &iter)
{
    if (iter == m_vMembers.end())
        return 0;
    return *iter++;
}

/** copies the struct object
 *    \return a reference to the new struct object
 */
CObject *CFEStructType::Clone()
{
    return new CFEStructType(*this);
}

/** tries to find a member by its name
 *    \param sName the name to look for
 *    \return the member if found, 0 if no such member
 */
CFETypedDeclarator *CFEStructType::FindMember(string sName)
{
    if (m_vMembers.empty())
        return 0;
    if (sName.empty())
        return 0;

    // check for a structural seperator ("." or "->")
    string::size_type iDot = sName.find('.');
    string::size_type iPtr = sName.find("->");
    string::size_type iUse;
    if ((iDot == string::npos) && (iPtr == string::npos))
        iUse = string::npos;
    else if ((iDot == string::npos) && (iPtr != string::npos))
        iUse = iPtr;
    else if ((iDot != string::npos) && (iPtr == string::npos))
        iUse = iDot;
    else
        iUse = (iDot < iPtr) ? iDot : iPtr;
    string sBase,
    sMember;
    if ((iUse != string::npos) && (iUse > 0))
    {
        sBase = sName.substr(0, iUse);
        if (iUse == iDot)
            sMember = sName.substr(sName.length() - (iDot + 1));
        else
            sMember = sName.substr(sName.length() - (iDot + 2));
    }
    else
        sBase = sName;

    vector<CFETypedDeclarator*>::iterator iter = GetFirstMember();
    CFETypedDeclarator *pTD;
    while ((pTD = GetNextMember(iter)) != 0)
    {
        if (pTD->FindDeclarator(sBase))
        {
            if ((iUse != string::npos) && (iUse > 0))
            {
                // if the found typed declarator has a constructed type (struct)
                // search for the second part of the name there
                if (dynamic_cast<CFEStructType*>(pTD->GetType()))
                {
                    if (!(((CFEStructType *)(pTD->GetType()))->FindMember(sMember)))
                    {
                        // no nested member with that name found
                        return 0;
                    }
                }
            }
            // return the found typed declarator
            return pTD;
        }
    }

    return 0;
}

/** \brief checks consitency
 *  \return false if error occurs, true if everything is fine
 *
 * A struct is consistent if all members are consistent.
 */
bool CFEStructType::CheckConsistency()
{
    vector<CFETypedDeclarator*>::iterator iter = GetFirstMember();
    CFETypedDeclarator *pMember;
    while ((pMember = GetNextMember(iter)) != 0)
    {
        if (!(pMember->CheckConsistency()))
            return false;
    }
    return true;
}

/** serialize this object
 *    \param pFile the file to serialize to/from
 */
void CFEStructType::Serialize(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        pFile->PrintIndent("<struct_type>\n");
        pFile->IncIndent();
        SerializeMembers(pFile);
        pFile->DecIndent();
        pFile->PrintIndent("</struct_type>\n");
    }
}

/** serialize this object
 *    \param pFile the file to serialize to/from
 */
void CFEStructType::SerializeMembers(CFile * pFile)
{
    if (pFile->IsStoring())
    {
        vector<CFETypedDeclarator*>::iterator iter = GetFirstMember();
        CFEBase *pElement;
        while ((pElement = GetNextMember(iter)) != 0)
        {
            pFile->PrintIndent("<member>\n");
            pFile->IncIndent();
            pElement->Serialize(pFile);
            pFile->DecIndent();
            pFile->PrintIndent("</member>\n");
        }
    }
}

/**    \brief test if this struct has members
 *    \return true if struct has members
 */
bool CFEStructType::HasMembers()
{
    return !m_vMembers.empty();
}
