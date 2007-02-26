/**
 *    \file    dice/src/fe/FETaggedUnionType.h
 *    \brief   contains the declaration of the class CFETaggedUnionType
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
#ifndef __DICE_FE_FETAGGEDUNIONTYPE_H__
#define __DICE_FE_FETAGGEDUNIONTYPE_H__

#include "fe/FEUnionType.h"

/**    \class CFETaggedUnionType
 *    \ingroup frontend
 *    \brief represents a tagged union type
 */
class CFETaggedUnionType : public CFEUnionType
{

// standard constructor/destructor
public:
    /** constructs a tagged union type
     *    \param sTag the tag of the union
     *    \param pUnionTypeHeader the union type header for this union
     */
    CFETaggedUnionType(string sTag, CFEUnionType *pUnionTypeHeader);
    /** constructs a tagged union type
     *  \param sTag the tag of the union
     */
    CFETaggedUnionType(string sTag);
    virtual ~CFETaggedUnionType();

protected:
    /**    \brief copy constructor
     *    \param src the source to copy from
     */
    CFETaggedUnionType(CFETaggedUnionType &src);
    virtual void SerializeMembers(CFile *pFile);

// Operations
public:
    virtual string GetTag();
    virtual CObject* Clone();
    virtual bool CheckConsistency();

// attribute
protected:
    /**    \var string m_sTag
     *    \brief the union's tag
     */
    string m_sTag;
};

#endif /* __DICE_FE_FETAGGEDUNIONTYPE_H__ */
