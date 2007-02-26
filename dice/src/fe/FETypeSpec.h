/**
 *	\file	dice/src/fe/FETypeSpec.h 
 *	\brief	contains the declaration of the class CFETypeSpec
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
#ifndef __DICE_FE_FETYPESPEC_H__
#define __DICE_FE_FETYPESPEC_H__


enum TYPESPEC_TYPE {
	TYPE_NONE				= 0,	/**< a non-initialzed type */
	TYPE_INTEGER			= 1,	/**< the integer type */
	TYPE_VOID				= 2,	/**< the void type */
	TYPE_FLOAT				= 3,	/**< the float type */
	TYPE_DOUBLE				= 4,	/**< the double type */
	TYPE_CHAR				= 5,	/**< the character type */
	TYPE_BOOLEAN			= 6,	/**< the boolean type */
	TYPE_BYTE				= 7,	/**< the byte type */
	TYPE_VOID_ASTERISK		= 8,	/**< a type for constant expressions: void* (pointer) */
	TYPE_CHAR_ASTERISK		= 9,	/**< a type for constant expressions: char* (string) */
	TYPE_STRUCT				= 10,	/**< the constructed type struct */
	TYPE_UNION				= 11,	/**< the constructed type union */
	TYPE_ENUM				= 12,	/**< the constructed type enum */
	TYPE_PIPE				= 13,	/**< the constructed type pipe */
	TYPE_TAGGED_STRUCT		= 14,	/**< the constructed type tagged struct */
	TYPE_TAGGED_UNION		= 15,	/**< the constructed type tagged union */
	TYPE_HANDLE_T			= 16,	/**< the predefined type handle_t */
	TYPE_ISO_LATIN_1		= 17,	/**< the predefined type iso_latin_1 (language type) */
	TYPE_ISO_MULTILINGUAL	= 18,	/**< the predefined type iso_multi_lingual (language type) */
	TYPE_ISO_UCS			= 19,	/**< the predefined type iso_ucs (language type) */
	TYPE_ERROR_STATUS_T		= 20,	/**< the predefined type error_status_t */
	TYPE_FLEXPAGE			= 21,	/**< the predefined type fpage (describes a memory page) */
	TYPE_RCV_FLEXPAGE		= 22,	/**< the helper type rcv_fpage (describes the receive window for mapped memory pages) */
	TYPE_USER_DEFINED		= 23,	/**< a user defined type */
	TYPE_LONG_DOUBLE		= 24,	/**< CORBA: the long double type */
	TYPE_WCHAR				= 25,	/**< CORBA: the wide character type */
	TYPE_OCTET				= 26,	/**< CORBA: the octet type */
	TYPE_ANY				= 27,	/**< CORBA: the any type */
	TYPE_OBJECT				= 28,	/**< CORBA: the object type */
	TYPE_TAGGED_ENUM		= 29,	/**< CORBA: the tagged enum type */
	TYPE_STRING				= 30,	/**< CORBA: an extra string type */
	TYPE_WSTRING			= 31,	/**< CORBA: an extra wide character string type */
	TYPE_ARRAY				= 32,	/**< CORBA: an sequence type */
	TYPE_REFSTRING			= 33,	/**< CORBA: a refstring (Flick relict) - temporary: is replaced during integrity check */
	TYPE_GCC				= 34,	/**< GCC: gcc specific type (can be ignored) */
    TYPE_MWORD              = 35,   /**< DICE: used to get the size and string for a machine word */
    TYPE_MAX                = 36    /**< delimiter value */
};

#include "fe/FEInterfaceComponent.h"

/**	\class CFETypeSpec
 *	\ingroup frontend
 *	\brief the base class for all types
 */
class CFETypeSpec : public CFEInterfaceComponent
{
DECLARE_DYNAMIC(CFETypeSpec);

// standard constructor/destructor
public:
	/** simple constructor for type spec object
	 *	\param nType the type of the type spec */
    CFETypeSpec(TYPESPEC_TYPE nType = TYPE_NONE);
    virtual ~CFETypeSpec();

protected:
	/** \brief copy constructor
	 *	\param src the source to copy from
	 */
	CFETypeSpec(CFETypeSpec &src);

// operations
public:
	virtual bool CheckConsistency();
	static bool IsConstructedType(CFETypeSpec *pType);
	virtual TYPESPEC_TYPE GetType();

// attributes
protected:
	/**	\var TYPESPEC_TYPE m_nType
	 *	\brief which type is represented
	 */
    TYPESPEC_TYPE m_nType;
};

#endif /* __DICE_FE_FETYPESPEC_H__ */

