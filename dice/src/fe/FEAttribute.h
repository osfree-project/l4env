/**
 *	\file	dice/src/fe/FEAttribute.h 
 *	\brief	contains the declaration of the class CFEAttribute
 *
 *	\date	01/31/2001
 *	\author	Ronald Aigner <ra3@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2003
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
    ATTR_NONE,	            /**< interface: empty attribute */
    ATTR_UUID,	            /**< interface: uuid attribute */
    ATTR_VERSION,	        /**< interface: version attribute */
    ATTR_ENDPOINT,	        /**< interface: end_point attribute */
    ATTR_EXCEPTIONS,	    /**< interface: exceptions attribute */
    ATTR_LOCAL,	            /**< interface: local attribute */
    ATTR_POINTER_DEFAULT,   /**< interface: pointer_default attribute */
    ATTR_OBJECT,	        /**< interface: object attribute */
    ATTR_UUID_REP,	        /**< library: uuid attribute */
    ATTR_CONTROL,	        /**< library: control attribute */
    ATTR_HELPCONTEXT,	    /**< library: helpcontext attribute */
    ATTR_HELPFILE,	        /**< library: helpfile attribute */
    ATTR_HELPSTRING,	    /**< library: helpstring attribute */
    ATTR_HIDDEN,	        /**< library: hidden attribute */
    ATTR_LCID,	            /**< library: lcid (library id) attribute */
    ATTR_RESTRICTED,	    /**< library: restricted attribute */
    ATTR_SWITCH_IS,	        /**< union: switch_is attribute - identifies the switch variable*/
    ATTR_IDEMPOTENT,	    /**< operation: idempotent attribute */
    ATTR_BROADCAST,	        /**< operation: broadcast attribute */
    ATTR_MAYBE,             /**< operation: maybe attribute */
    ATTR_REFLECT_DELETIONS,	/**< operation: reflect_deletions attribute */
    ATTR_TRANSMIT_AS,	    /**< type: transmit_as attribute */
    ATTR_HANDLE,	        /**< type: handle attribute */
    ATTR_FIRST_IS,	        /**< parameter: field attribute first_is */
    ATTR_LAST_IS,	        /**< parameter: field attribute last_is */
    ATTR_LENGTH_IS,	        /**< parameter: filed attribute length_is */
    ATTR_SIZE_IS,	        /**< parameter: field attribute size_is */
    ATTR_MAX_IS,	        /**< parameter: field attribute max_is */
    ATTR_MIN_IS,	        /**< parameter: field attribute min_is */
    ATTR_IGNORE,	        /**< parameter: field attribute ignore - ignores this member of a struct */
    ATTR_IN,	            /**< parameter: directional attribute in */
    ATTR_OUT,	            /**< parameter: directional attribute out */
    ATTR_REF,	            /**< parameter: pointer attribute ref */
    ATTR_UNIQUE,	        /**< parameter: pointer attribute unique */
    ATTR_PTR,	            /**< parameter: pointer attribute ptr */
    ATTR_IID_IS,	        /**< parameter: iid_is attribute - used to specify a varibale defining the way this parameter should be marshalled */
    ATTR_STRING,	        /**< parameter: usage attribute string */
    ATTR_CONTEXT_HANDLE,	/**< parameter: usage attribute context_handle */
    ATTR_SWITCH_TYPE,	    /**< parameter: attribute switch_type defines the type of the switch variable */
    ATTR_ABSTRACT,	        /**< CORBA: abstract interfaces */
    ATTR_DEFAULT_FUNCTION,  /**< interface: default function (if no opcode matches) */
    ATTR_ERROR_FUNCTION,    /**< interface: error function (if IPC wait returns without receiving a message) */
    ATTR_SERVER_PARAMETER,  /**< interface: same as -fserver-parameter on per interface basis */
    ATTR_INIT_RCVSTRING,    /**< interface: same as -finit-rcvstring on per interface basis */
	ATTR_PREALLOC,      /**< parameter: init the recieve buffer with the input value */
	ATTR_ALLOW_REPLY_ONLY,  /**< function: allow that function can reply only to the client */
    ATTR_LAST_ATTR          /**< the last attribute (used for iteration */
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

