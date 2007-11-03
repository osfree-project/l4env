/**
 *  \file   dice/src/fe/FETypeAttribute.h
 *  \brief  contains the declaration of the class CFETypeAttribute
 *
 *  \date   01/31/2001
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef __DICE_FE_FETYPEATTRIBUTE_H__
#define __DICE_FE_FETYPEATTRIBUTE_H__

#include "fe/FEAttribute.h"

class CFETypeSpec;

/* CFETypeAttribute : SWITCH_TYPE, TRANSMIT_AS */
/** \class CFETypeAttribute
 *  \ingroup frontend
 *  \brief represents all attributes, which have a type as argument
 */
class CFETypeAttribute : public CFEAttribute
{

// standard constructor/destructor
public:
    /** constructor of the type attribute
     *  \param nType the type of the attribute (Switch-type, transmit-as)
     *  \param pType the type which is parameter of the attribute */
    CFETypeAttribute(ATTR_TYPE nType, CFETypeSpec *pType);
    virtual ~CFETypeAttribute();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFETypeAttribute(CFETypeAttribute* src);

// Operations
public:
    virtual CFETypeSpec* GetType();
	virtual CFETypeAttribute* Clone();

// attributes
protected:
    /** \var CFETypeSpec *m_pType
     *  \brief the type argument of the attribute
     */
    CFETypeSpec *m_pType;
};

#endif /* __DICE_FE_FETYPEATTRIBUTE_H__ */

