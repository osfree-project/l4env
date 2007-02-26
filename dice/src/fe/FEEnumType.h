/**
 *    \file    dice/src/fe/FEEnumType.h
 *    \brief   contains the declaration of the class CFEEnumType
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

/** preprocessing symbol to check header file */
#ifndef __DICE_FE_FEENUMTYPE_H__
#define __DICE_FE_FEENUMTYPE_H__

#include "fe/FEConstructedType.h"
#include <vector>
using namespace std;

class CFEIdentifier;

/**    \class CFEEnumType
 *    \ingroup frontend
 *    \brief represents the enumeration type
 *
 * This class is used to represent an enumeration type in the IDL.
 */
class CFEEnumType : public CFEConstructedType
{
// standard constructor/destructor
public:
    /** constructs an enum type
     *    \param pMembers the members of the enumeration
     */
    CFEEnumType(vector<CFEIdentifier*> *pMembers);
    virtual ~CFEEnumType();

protected:
    /**    \brief copy constructor
     *    \param src the source to copy from
     */
    CFEEnumType(CFEEnumType &src);
    virtual void SerializeMembers(CFile *pFile);

// Operations
public:
    virtual void Serialize(CFile *pFile);
    virtual bool CheckConsistency();
    virtual CObject* Clone();
    virtual CFEIdentifier* GetNextMember(vector<CFEIdentifier*>::iterator &iter);
    virtual vector<CFEIdentifier*>::iterator GetFirstMember();

// atributes
protected:
    /**    \var vector<CFEIdentifier*> m_vMembers
     *    \brief contains the members (the enumeration names)
     */
    vector<CFEIdentifier*> m_vMembers;
};

#endif /* __DICE_FE_FEENUMTYPE_H__ */

