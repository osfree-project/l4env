/**
 *  \file    dice/src/fe/FEAttribute.h
 *  \brief   contains the declaration of the class CFEAttribute
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
#ifndef __DICE_FE_FEATTRIBUTE_H__
#define __DICE_FE_FEATTRIBUTE_H__

#include "fe/FEBase.h"
#include "Attribute-Type.h"

/** \class CFEAttribute
 *  \ingroup frontend
 *  \brief the base class for all attributes
 *
 * the attributes of an operation or interface
 */
class CFEAttribute : public CFEBase
{
// standard constructor/destructor
public:
    /** constructs an attrbiute (standard constructor) */
    CFEAttribute();
    /** constructs an attribute
     *  \param nType the type of the attribute (see ATTR_TYPE)
     */
    CFEAttribute(ATTR_TYPE nType);
    virtual ~CFEAttribute();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFEAttribute(CFEAttribute* src);

//operations
public:
	virtual CObject* Clone();
    virtual ATTR_TYPE GetAttrType();
    bool Match(ATTR_TYPE type);

// attributes
protected:
    /** \var ATTR_TYPE m_nType
     *  \brief the attribute type
     */
    ATTR_TYPE m_nType;
};

#endif /* __DICE_FE_FEATTRIBUTE_H__ */

