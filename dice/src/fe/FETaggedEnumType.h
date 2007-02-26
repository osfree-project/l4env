/**
 *	\file	dice/src/fe/FETaggedEnumType.h 
 *	\brief	contains the declaration of the class CFETaggedEnumType
 *
 *	\date	03/22/2001
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
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
#ifndef __DICE_FE_FETAGGEDENUMTYPE_H__
#define __DICE_FE_FETAGGEDENUMTYPE_H__

#include "FEEnumType.h"

/**	\class CFETaggedEnumType
 *	\ingroup frontend
 *	\brief represents a tagged enum type
 *
 * A tagged struct type is a enum type with an additional name
 * set after the 'enum' keyword: "enum &lt;name&gt; { ..."
 */
class CFETaggedEnumType : public CFEEnumType  
{
DECLARE_DYNAMIC(CFETaggedEnumType);

public:
	/** constructs a taged enum object
	 *	\param sTag the tag of the enum
	 *	\param pMembers the members of the enum
	 */
	CFETaggedEnumType(String sTag, Vector *pMembers);
	virtual ~CFETaggedEnumType();

protected:
	/** \brief copy constructor
	 *	\param src the source to copy from
	 */
	CFETaggedEnumType(CFETaggedEnumType &src);

// Operations
public:
	virtual String GetTag();
	virtual CObject* Clone();
    virtual bool CheckConsistency();

// Attributes
protected:
	/**	\var String m_sTag
	 *	\brief the tag of the enum
	 */
	String m_sTag;
};

#endif // __DICE_FE_FETAGGEDENUMTYPE_H__
