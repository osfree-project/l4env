/**
 *    \file    dice/src/fe/FEIsAttribute.h
 *  \brief   contains the declaration of the class CFEIsAttribute
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
#ifndef __DICE_FE_FEISATTRIBUTE_H__
#define __DICE_FE_FEISATTRIBUTE_H__

#include "fe/FEAttribute.h"
#include "template.h"
#include <vector>

class CFEDeclarator;

/** \class CFEIsAttribute
 *  \ingroup frontend
 *  \brief represents all the attributes, which end on "_IS"
 *
 *
 * FIRST_IS, LAST_IS, LENGTH_IS, MIN_IS, MAX_IS, SIZE_IS, SWITCH_IS, IID_IS
 */
class CFEIsAttribute : public CFEAttribute
{

// standard constructor/destructor
public:
    /** \brief constructed an "is-attribute"
     *  \param nType the type of the attribute
     *  \param pAttrParameters the parameters of the attribute
     */
    CFEIsAttribute(ATTR_TYPE nType, vector<CFEDeclarator*> *pAttrParameters);
    virtual ~CFEIsAttribute();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFEIsAttribute(CFEIsAttribute &src);

public:
    virtual CObject* Clone();

// attributes
public:
    /** \var CCollection<CFEDeclarator> m_AttrParameters
     *  \brief the parameters (declarators) the attribute can contain
     */
    CCollection<CFEDeclarator> m_AttrParameters;
};

#endif /* __DICE_FE_FEISATTRIBUTE_H__ */

