/**
 *  \file   dice/src/fe/FERangeAttribute.h
 *  \brief  contains the declaration of the class CFERangeAttribute
 *
 *  \date   10/08/2007
 *  \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef __DICE_FE_FERANGEATTRIBUTE_H__
#define __DICE_FE_FERANGEATTRIBUTE_H__

#include "fe/FEAttribute.h"

/** \class CFERangeAttribute
 *  \ingroup frontend
 *  \brief an integer range attribute
 *
 * This class is used to represent integer range attributes.
 */
class CFERangeAttribute : public CFEAttribute
{
// Constructor
public:
    /** constructs an int attribute
     *  \param nType the type of the attribute
     *  \param nLower the lower integer value of the attribute
	 *  \param nUpper the upper integer value of the attribute
	 *  \param bAbs true if the values are absolute
     */
    CFERangeAttribute(ATTR_TYPE nType, int nLower, int nUpper, bool bAbs = true);
    virtual ~CFERangeAttribute();

protected:
    /** \brief copy constructor
     *  \param src the source to copy from
     */
    CFERangeAttribute(CFERangeAttribute* src);

// Operations
public:
	virtual CObject* Clone();

	/** \brief retrieves the lower integer values of this attribute
	 *  \return  the integer values of this attribute
	 */
	int GetLowerValue()
	{ return m_nLower; }
	/** \brief retrieves the upper integer values of this attribute
	 *  \return  the integer values of this attribute
	 */
	int GetUpperValue()
	{ return m_nUpper; }
	/** \brief retrieves the lower and upper integer values of this attribute
	 *  \param nLower the lower integer value of this attribute
	 *  \param nUpper the upper integer value of this attribute
	 */
	void GetValues(int& nLower, int& nUpper)
	{
		nLower = m_nLower;
		nUpper = m_nUpper;
	}
	/** \brief returns true if the values are absolute
	 *  \return true if absolute values
	 */
	bool IsAbsolute()
	{ return m_bAbsolute; }

// Attributes
protected:
    /** \var int m_nLower
     *  \brief the lower integer value, which is associated with this attribute
     */
    int m_nLower;
    /** \var int m_nUpper
     *  \brief the upper integer value, which is associated with this attribute
     */
    int m_nUpper;
	/** \var bool m_bAbsolute;
	 *  \brief true if the values are absolute
	 */
	bool m_bAbsolute;
};

#endif /* __DICE_FE_FEINTATTRIBUTE_H__ */
