/**
 *  \file    dice/src/fe/FEStructType.cpp
 *  \brief   contains the implementation of the class CFEStructType
 *
 *  \date    01/31/2001
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

#include "FEStructType.h"
#include "FETypedDeclarator.h"
#include "FEInterface.h"
#include "FELibrary.h"
#include "FEFile.h"
#include "File.h"
#include "Compiler.h"
#include "Visitor.h"
#include <iostream>

CFEStructType::CFEStructType(string sTag, 
    vector<CFETypedDeclarator*> * pMembers)
: CFEConstructedType(TYPE_STRUCT),
    m_Members(pMembers, this)
{
    m_sTag = sTag;
    if (!pMembers)
        m_bForwardDeclaration = true;
}

CFEStructType::CFEStructType(CFEStructType & src)
: CFEConstructedType(src),
    m_Members(src.m_Members)
{
    m_sTag = src.m_sTag;
    m_Members.Adopt(this);
}

/** cleans up the struct object (delete all members) */
CFEStructType::~CFEStructType()
{ }

/** copies the struct object
 *  \return a reference to the new struct object
 */
CObject* CFEStructType::Clone()
{ 
    return new CFEStructType(*this);
}

/** tries to find a member by its name
 *  \param sName the name to look for
 *  \return the member if found, 0 if no such member
 */
CFETypedDeclarator *CFEStructType::FindMember(string sName)
{
    if (m_Members.empty())
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

    vector<CFETypedDeclarator*>::iterator iter;
    for (iter = m_Members.begin();
	 iter != m_Members.end();
	 iter++)
    {
	if ((*iter)->m_Declarators.Find(sBase))
	{
	    if ((iUse != string::npos) && (iUse > 0))
	    {
		// if the found typed declarator has a constructed type (struct)
		// search for the second part of the name there
		CFEStructType *pStruct = 
		    dynamic_cast<CFEStructType*>((*iter)->GetType());
		if (pStruct)
		{
		    if (!pStruct->FindMember(sMember))
		    {
			// no nested member with that name found
			return 0;
		    }
		}
	    }
	    // return the found typed declarator
	    return *iter;
	}
    }

    return (CFETypedDeclarator*)0;
}

/** \brief accepts the iterations of the visitor
 *  \param v reference to the visitor
 */
void
CFEStructType::Accept(CVisitor& v)
{
    v.Visit(*this);

    vector<CFETypedDeclarator*>::iterator iter;
    for (iter = m_Members.begin();
	iter != m_Members.end();
	iter++)
    {
	(*iter)->Accept(v);
    }
}
