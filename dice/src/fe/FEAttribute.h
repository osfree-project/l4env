/**
 *	\file	dice/src/fe/FEAttribute.h 
 *	\brief	contains the declaration of the class CFEAttribute
 *
 *	\date	01/31/2001
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
#ifndef __DICE_FE_FEATTRIBUTE_H__
#define __DICE_FE_FEATTRIBUTE_H__

enum ATTR_TYPE {
    ATTR_NONE				= 0,	/**< interface: empty attribute */
    ATTR_UUID				= 1,	/**< interface: uuid attribute */
    ATTR_VERSION			= 2,	/**< interface: version attribute */
    ATTR_ENDPOINT			= 3,	/**< interface: end_point attribute */
    ATTR_EXCEPTIONS			= 4,	/**< interface: exceptions attribute */
    ATTR_LOCAL				= 5,	/**< interface: local attribute */
    ATTR_POINTER_DEFAULT	= 6,	/**< interface: pointer_default attribute */
    ATTR_OBJECT				= 7,	/**< interface: object attribute */
    ATTR_UUID_REP			= 8,	/**< library: uuid attribute */
    ATTR_CONTROL			= 9,	/**< library: control attribute */
    ATTR_HELPCONTEXT		= 10,	/**< library: helpcontext attribute */
    ATTR_HELPFILE			= 11,	/**< library: helpfile attribute */
    ATTR_HELPSTRING			= 12,	/**< library: helpstring attribute */
    ATTR_HIDDEN				= 13,	/**< library: hidden attribute */
    ATTR_LCID				= 14,	/**< library: lcid (library id) attribute */
    ATTR_RESTRICTED			= 15,	/**< library: restricted attribute */
    ATTR_SWITCH_IS			= 16,	/**< union: switch_is attribute - identifies the switch variable*/
    ATTR_IDEMPOTENT			= 17,	/**< operation: idempotent attribute */
    ATTR_BROADCAST			= 18,	/**< operation: broadcast attribute */
    ATTR_MAYBE				= 19,	/**< operation: maybe attribute */
    ATTR_REFLECT_DELETIONS	= 20,	/**< operation: reflect_deletions attribute */
    ATTR_TRANSMIT_AS		= 21,	/**< type: transmit_as attribute */
    ATTR_HANDLE				= 22,	/**< type: handle attribute */
    ATTR_FIRST_IS			= 23,	/**< parameter: field attribute first_is */
    ATTR_LAST_IS			= 24,	/**< parameter: field attribute last_is */
    ATTR_LENGTH_IS			= 25,	/**< parameter: filed attribute length_is */
    ATTR_SIZE_IS			= 26,	/**< parameter: field attribute size_is */
    ATTR_MAX_IS				= 27,	/**< parameter: field attribute max_is */
    ATTR_MIN_IS				= 28,	/**< parameter: field attribute min_is */
    ATTR_IGNORE				= 29,	/**< parameter: field attribute ignore - ignores this member of a struct */
    ATTR_IN					= 30,	/**< parameter: directional attribute in */
    ATTR_OUT				= 31,	/**< parameter: directional attribute out */
    ATTR_REF				= 32,	/**< parameter: pointer attribute ref */
    ATTR_UNIQUE				= 33,	/**< parameter: pointer attribute unique */
    ATTR_PTR				= 34,	/**< parameter: pointer attribute ptr */
    ATTR_IID_IS				= 35,	/**< parameter: iid_is attribute - used to specify a varibale defining the way this parameter should be marshalled */
    ATTR_STRING				= 36,	/**< parameter: usage attribute string */
    ATTR_CONTEXT_HANDLE		= 37,	/**< parameter: usage attribute context_handle */
    ATTR_SWITCH_TYPE		= 38,	/**< parameter: attribute switch_type defines the type of the switch variable */
    ATTR_ABSTRACT			= 39,	/**< CORBA: abstract interfaces */
    ATTR_DEFAULT_FUNCTION   = 40,   /**< interface: default function (if no opcode matches) */
    ATTR_ERROR_FUNCTION     = 41,   /**< interface: error function (if IPC wait returns without receiving a message) */
    ATTR_SERVER_PARAMETER   = 42,   /**< interface: same as -fserver-parameter on per interface basis */
    ATTR_INIT_RCVSTRING     = 43,   /**< interface: same as -finit-rcvstring on per interface basis */
	ATTR_INIT_WITH_IN       = 44,   /**< parameter: init the recieve buffer with the input value */
    ATTR_LAST_ATTR          = 45    /**< the last attribute (used for iteration */
};

#include "fe/FEBase.h"

/** \class CFEAttribute
 *	\ingroup frontend
 *	\brief the base class for all attributes
 *
 * the attributes of an operation or interface
 */
class CFEAttribute : public CFEBase
{
DECLARE_DYNAMIC(CFEAttribute);

// standard constructor/destructor
public:
	/** constructs an attrbiute (standard constructor) */
	CFEAttribute();
	/** constructs an attribute
	 *	\param nType the type of the attribute (see ATTR_TYPE)
	 */
    CFEAttribute(ATTR_TYPE nType);
    virtual ~CFEAttribute();

protected:
	/**	\brief copy constructor
	 *	\param src the source to copy from
	 */
	CFEAttribute(CFEAttribute &src);

//operations
public:
	virtual void Serialize(CFile *pFile);
	virtual CObject* Clone();
	virtual ATTR_TYPE GetAttrType();

// attributes
protected:
	/**	\var ATTR_TYPE m_nType
	 *	\brief the attribute type
	 */
    ATTR_TYPE m_nType;
};

#endif /* __DICE_FE_FEATTRIBUTE_H__ */

