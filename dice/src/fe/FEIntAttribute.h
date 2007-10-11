/**
 *  \file   dice/src/fe/FEIntAttribute.h
 *  \brief  contains the declaration of the class CFEIntAttribute
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
#ifndef __DICE_FE_FEINTATTRIBUTE_H__
#define __DICE_FE_FEINTATTRIBUTE_H__

#include "fe/FEAttribute.h"

/** \class CFEIntAttribute
 *  \ingroup frontend
 *  \brief an integer value attribute
 *
 * This class is used to represent integer value attributes.
 */
class CFEIntAttribute : public CFEAttribute
{
// Constructor
public:
    /** constructs an int attribute
     *  \param nType the type of the attribute
     *  \param nValue the integer value of the attribute
	 *  \param bAbs true if absolute value
     */
    CFEIntAttribute(ATTR_TYPE nType, int nValue, bool bAbs = true);
    virtual ~CFEIntAttribute();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFEIntAttribute(CFEIntAttribute* src);

// Operations
public:
	virtual CObject* Clone();

	/** \brief retrieves the integer values of this attribute
	 *  \return  the integer values of this attribute
	 */
	int GetIntValue()
	{ return m_nIntValue; }

	/** \brief return whether or not this is an absolute value
	 *  \return true if absolute value
	 */
	bool IsAbsolute()
	{ return m_bAbsoluteValue; }

// Attributes
protected:
    /** \var int m_nIntValue
     *  \brief the integer value, which is associated with this attribute
     */
    int m_nIntValue;
	/** \var bool m_bAbsoluteValue
	 *  \brief differentiates between absolute and relative values
	 */
	bool m_bAbsoluteValue;
};

#endif /* __DICE_FE_FEINTATTRIBUTE_H__ */
