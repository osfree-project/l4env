/**
 *  \file   dice/src/TypeSpec-Type.h
 *  \brief  contains the declaration defines used with types
 *
 *  \date   04/15/2003
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
#ifndef __DICE_TYPESPEC_TYPE_H__
#define __DICE_TYPESPEC_TYPE_H__


/** \brief defines the available types
 *
 * We use an anonymous enum and an integer type wherever we use the types as
 * arguments.
 */
enum
{
    TYPE_NONE = 0,		/**< a non-initialzed type */
    TYPE_INTEGER,		/**< the integer type */
    TYPE_LONG,			/**< basically the same as INTEGER, but needed to know them apart */
    TYPE_VOID,			/**< the void type */
    TYPE_FLOAT,			/**< the float type */
    TYPE_DOUBLE,		/**< the double type */
    TYPE_FIXED,			/**< the fixed type */
    TYPE_CHAR,			/**< the character type */
    TYPE_BOOLEAN,		/**< the boolean type */
    TYPE_BYTE,			/**< the byte type */
    TYPE_VOID_ASTERISK,		/**< a type for constant expressions: void* (pointer) */ // 10
    TYPE_CHAR_ASTERISK,		/**< a type for constant expressions: char* (string) */
    TYPE_STRUCT,		/**< the constructed type struct */
    TYPE_UNION,			/**< the constructed type union */
    TYPE_IDL_UNION,		/**< an enclosed, aka. IDL, union */
    TYPE_ENUM,			/**< the constructed type enum */
    TYPE_PIPE,			/**< the constructed type pipe */
    TYPE_TYPEOF,		/**< the type_of construct */
    TYPE_HANDLE_T,		/**< the predefined type handle_t */
    TYPE_ISO_LATIN_1,		/**< the predefined type iso_latin_1 (language type) */
    TYPE_ISO_MULTILINGUAL,	/**< the predefined type iso_multi_lingual (language type) */ // 20
    TYPE_ISO_UCS,		/**< the predefined type iso_ucs (language type) */
    TYPE_ERROR_STATUS_T,	/**< the predefined type error_status_t */
    TYPE_FLEXPAGE,		/**< the predefined type fpage (describes a memory page) */
    TYPE_RCV_FLEXPAGE, /**< the helper type rcv_fpage (describes the receive window for mapped memory pages) */
    TYPE_USER_DEFINED,		/**< a user defined type */
    TYPE_LONG_DOUBLE,		/**< CORBA: the long double type */
    TYPE_WCHAR,			/**< CORBA: the wide character type */
    TYPE_OCTET,			/**< CORBA: the octet type */
    TYPE_ANY,			/**< CORBA: the any type */
    TYPE_OBJECT,		/**< CORBA: the object type */ // 30
    TYPE_STRING,		/**< CORBA: an extra string type */
    TYPE_WSTRING,		/**< CORBA: an extra wide character string type */
    TYPE_ARRAY,			/**< CORBA: an sequence type */
    TYPE_REFSTRING,		/**< CORBA: a refstring */
    TYPE_GCC,			/**< GCC: gcc specific type (can be ignored) */
    TYPE_MWORD,			/**< DICE: used to get the size and string for a machine word */
    TYPE_EXCEPTION,		/**< DICE: CORBA exception type */
    TYPE_MESSAGE,		/**< DICE: used to determine (maximum) size for messages */
	TYPE_MSGDOPE_SIZE,	/**< L4: the l4 message size dope type */
	TYPE_MSGDOPE_SEND,	/**< L4: the l4 message send dope type */
	TYPE_MSGTAG,		/**< L4: the L4 message tag type */
	TYPE_UTCB,			/**< L4: the size of the used UTCB */
    TYPE_MAX			/**< delimiter value */
};

#endif /* __DICE_TYPESPEC_TYPE_H__ */

