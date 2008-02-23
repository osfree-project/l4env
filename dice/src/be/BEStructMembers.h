/**
 *  \file    dice/src/be/BEStructMembers.h
 *  \brief   contains the declaration of the class CBEStructMembers
 *
 *  \date    09/05/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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

/** preprocessing symbol to check header file */
#ifndef __DICE_BESTRUCTMEMBERS_H__
#define __DICE_BESTRUCTMEMBERS_H__

#include "template.h"

class CBETypedDeclarator;

/** \class CStructMembers
 *  \ingroup backend
 *  \brief a special collection class for struct members
 */
class CStructMembers : public CSearchableCollection<CBETypedDeclarator, std::string>
{
public:
    /** \brief constructs struct members collection
     *  \param src the source vector with the members
     *  \param pParent the parent of the members
     */
    CStructMembers(vector<CBETypedDeclarator*> *src, CObject *pParent);
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CStructMembers(CStructMembers& src);
    /** destroy the collection */
    ~CStructMembers();

    void Add(CBETypedDeclarator *pMember);
    void Move(std::string sName, int nPos);
    void Move(std::string sName, std::string sBeforeHere);
	void MoveAfter(std::string sName, std::string sAfterHere);
};

#endif // !__DICE_BESTRUCTTYPE_H__
