/**
 *    \file    dice/src/fe/FETaggedStructType.h
 *    \brief   contains the declaration of the class CFETaggedStructType
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
#ifndef __DICE_FE_FETAGGEDSTRUCTTYPE_H__
#define __DICE_FE_FETAGGEDSTRUCTTYPE_H__

#include "fe/FEStructType.h"
#include <string>
using namespace std;

/**    \class CFETaggedStructType
 *    \ingroup frontend
 *    \brief represents a tagged struct type
 *
 * A tagged struct type is a struct type with an additional name
 * set after the 'struct' keyword: "struct &lt;name&gt; { ..."
 */
class CFETaggedStructType : public CFEStructType
{

public:
    /** constructs a taged struct object
     *    \param sTag the tag of the struct
     *    \param pMembers the members of the struct
     */
    CFETaggedStructType(string sTag, vector<CFETypedDeclarator*> *pMembers = 0);
    virtual ~CFETaggedStructType();

protected:
    /** \brief copy constructor
     *    \param src the source to copy from
     */
    CFETaggedStructType(CFETaggedStructType &src);
    virtual void SerializeMembers(CFile *pFile);

// Operations
public:
    virtual CObject* Clone();
    virtual string GetTag();
    virtual bool CheckConsistency();

protected:
    /**    \var string m_sTag
     *    \brief the tag of the struct
     */
    string m_sTag;
};

#endif /* __DICE_FE_FESTRUCTTYPE_H__ */
