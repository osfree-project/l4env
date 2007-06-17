/**
 *  \file    dice/src/fe/FEUnionType.h
 *  \brief   contains the declaration of the class CFEUnionType
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

/** preprocessing symbol to check header file */
#ifndef __DICE_FE_FEUNIONTYPE_H__
#define __DICE_FE_FEUNIONTYPE_H__

#include "fe/FEConstructedType.h"
#include "fe/FEUnionCase.h"
#include "template.h"
#include <string>
#include <vector>

class CFEUnionCase;

/** \class CFEUnionType
 *  \ingroup frontend
 *  \brief represents a union
 */
class CFEUnionType : public CFEConstructedType
{

// standard constructor/destructor
public:
    /** \brief constructor for object of union type
     *  \param pUnionBody the "real" union
     *  \param sTag the tag if one
     */
    CFEUnionType(string sTag,
        vector<CFEUnionCase*> *pUnionBody);
    virtual ~CFEUnionType();

// Operations
public:
    virtual void Accept(CVisitor&);
    virtual CObject* Clone();
    virtual bool IsConstructedType();
    
protected:
    /** a copy construtor used for the tagged union class */
    CFEUnionType(CFEUnionType& src); // copy constructor for tagged union

// attribute
public:
    /** \var CCollection<CFEUnionCase*> m_UnionCases
     *  \brief the elements of the union (it's body)
     */
    CCollection<CFEUnionCase> m_UnionCases;
};

#endif /* __DICE_FE_FEUNIONTYPE_H__ */

