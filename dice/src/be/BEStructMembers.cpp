/**
 * \file    dice/src/be/BEStructMembers.cpp
 * \brief   contains the implementation of the class CBEStructMembers
 *
 * \date    09/05/2007
 * \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
 */
/*
 * Copyright (C) 2007
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

#include "BEStructMembers.h"
#include "BETypedDeclarator.h"
#include "Object.h"
#include "Compiler.h"
#include <string>

CStructMembers::CStructMembers(vector<CBETypedDeclarator*> *src, CObject *pParent)
: CSearchableCollection<CBETypedDeclarator, std::string>(src, pParent)
{ }

CStructMembers::CStructMembers(CStructMembers& src)
: CSearchableCollection<CBETypedDeclarator, std::string>(src)
{ }

CStructMembers::~CStructMembers()
{ }

/** \brief adds a new member to the struct members collection
 *  \param pMember reference to the new member
 */
void CStructMembers::Add(CBETypedDeclarator *pMember)
{
	if (!pMember)
		return;

	if (pMember->m_Declarators.First() &&
		Find(pMember->m_Declarators.First()->GetName()))
		return; // member already exists
	CSearchableCollection<CBETypedDeclarator, std::string>::Add(pMember);
}

/** \brief moves a member in the struct
 *  \param sName the name of the member
 *  \param nPos the new position in the struct
 *
 * If nPos is -1 this means the end of the struct.
 * If nPos is 0 this means the begin of the struct.
 * nPos is regarded to be zero-based.
 * Otherwise the member is extracted from the struct
 * and inserted before the old member with the number nPos.
 * If the number nPos is larger than the struct size,
 * an error is returned.
 */
void CStructMembers::Move(std::string sName, int nPos)
{
	CBETypedDeclarator *pMember = Find(sName);
	if (!pMember)
		return;

	Remove(pMember);
	// check if nPos was too big
	if (nPos == -1 || (unsigned)nPos > size())
	{
		Add(pMember);
		return;
	}

	// insert member
	vector<CBETypedDeclarator*>::insert(begin() + nPos, pMember);
}

/** \brief moves a member in the struct
 *  \param sName the name of the member to move
 *  \param sBeforeHere the name of the member to move before
 */
void CStructMembers::Move(std::string sName, std::string sBeforeHere)
{
	CBETypedDeclarator *pMember = Find(sName);
	if (!pMember)
		return;
	if (sName == sBeforeHere)
		return;

	Remove(pMember);
	// get position of BeforeHere
	iterator i = std::find_if(begin(), end(), std::bind2nd(std::ptr_fun(Match), sBeforeHere));
	// check if BeforeHere was member of this struct
	if (i == end())
	{
		Add(pMember);
		return;
	}
	// move member
	vector<CBETypedDeclarator*>::insert(i, pMember);
}

/** \brief moves a member in the struct
 *  \param sName the name of the member to move
 *  \param sAfterHere the name of the member to move after
 */
void CStructMembers::MoveAfter(std::string sName, std::string sAfterHere)
{
	CBETypedDeclarator *pMember = Find(sName);
	if (!pMember)
		return;
	if (sName == sAfterHere)
		return;

	Remove(pMember);
	// get position of AfterHere
	iterator i = std::find_if(begin(), end(), std::bind2nd(std::ptr_fun(Match), sAfterHere));
	// check if AfterHere was member of this struct
	if (i == end())
	{
		Add(pMember);
		return;
	}
	// move member
	vector<CBETypedDeclarator*>::insert(i+1, pMember);
}

