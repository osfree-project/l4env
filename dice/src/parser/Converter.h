/**
 *  \file    dice/src/parser/Converter.h
 *  \brief   contains the declaration helper functions for the parsers
 *
 *  \date    06/14/2007
 *  \author  Ronald Aigner <ra3@os.inf.tu-dresden.de>
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
#ifndef __DICE_PARSER_CONVERTER_H__
#define __DICE_PARSER_CONVERTER_H__

#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

namespace dice {
	namespace parser {

		/** \enum IntType
		 *  \ingroup parser
		 *  \brief the integer type of the parsed string
		 */
		enum IntType {
			INVALID = 0,	/**< invalid integer */
			INT_ULLONG,		/**< unsigned long long integer */
			INT_LLONG,		/**< long long integer */
			INT_ULONG,		/**< unsigned long integer */
			INT_LONG,		/**< long integer */
			INT_INT			/**< integer */
		};

		// number conversion
		// character conversion
		int oct_to_char (char *);
		int hex_to_char (char *);
		int escape_to_char (char *);
		// check integer type
		IntType int_type (char *);
		// get decimal number from different strings
#if HAVE_ATOLL
		long long oct_to_long (char *, IntType&);
		long long hex_to_long (char *, IntType&);
		long long int_to_long (char *, IntType&);
#else
		long oct_to_long (char *, IntType&);
		long hex_to_long (char *, IntType&);
		long int_to_long (char *, IntType&);
#endif

	};
};

#endif /* __DICE_PARSER_CONVERTER_H__ */
