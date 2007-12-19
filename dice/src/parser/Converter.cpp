/**
 *  \file    dice/src/parser/Converter.cpp
 *  \brief   contains the definition of helper functions for the parsers
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

#include "Converter.h"
#include "Messages.h"
#include "fe/FEFile.h"
#include "Compiler.h"
#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif
#include <errno.h>
#include <limits>
#include <cmath>
#include <string>

using namespace dice::parser;

/** \brief check which integer suffix is used
 *  \ingroup parser
 *  \param str the string with the suffix
 *  \return the integer type deducted
 */
IntType dice::parser::int_type (char *str)
{
	// check ending
	// if "ll" or "ull" -> longlong
	// could end on u,ul,ull,l,ll,lu,llu
	std::string s(str);
	std::transform(s.rbegin(), s.rend(), s.rbegin(), tolower);

	std::string::size_type l = s.length();
	if (l > 3 && (s.substr(l-3) == "ull" || s.substr(l-3) == "llu"))
		return INT_ULLONG;
	if (l > 2 && s.substr(l-2) == "ll")
		return INT_LLONG;
	if (l > 2 && (s.substr(l-2) == "lu" || s.substr(l-2) == "ul"))
	{
		// if compiling for amd64, this is unsigned long long
		if (CCompiler::IsBackEndPlatformSet(PROGRAM_BE_AMD64))
			return INT_ULLONG;
		return INT_ULONG;
	}
	if (l > 1 && s[l-1] == 'l')
	{
		// if compiling for amd64, this is unsigned long long
		if (CCompiler::IsBackEndPlatformSet(PROGRAM_BE_AMD64))
			return INT_LLONG;
		return INT_LONG;
	}
	if (l > 1 && s[l-1] == 'u')
		return INT_ULONG;
	return INT_INT;
}

/** \brief convert octal coded characters to ASCII char
 *  \ingroup parser
 *  \param s the string to extract the number from
 *  \return the ascii code
 */
int dice::parser::oct_to_char (char *s)
{
	// check for leading 'L'
	if (s[0] == 'L') s++;
	// skip the first ' and backslash
	s += 2;
	// now we can use the integer converter to obtain ASCII code
	return (int) strtol (s, 0, 8);
}

/** \brief convert hexadecimal coded characters to ASCII char
 *  \ingroup parser
 *  \param s the string to extract the character from
 *  \return the ascii code of the character
 */
int dice::parser::hex_to_char (char *s)
{
	// check for leading 'L'
	if (s[0] == 'L') s++;
	// skip the first ' and backslash
	s += 2;
	// now we can use the integer converter to obtain ASCII code
	return (int) strtol (s, 0, 16);
}

/** \brief converts escape sequences to character
 *  \ingroup parser
 *  \param s the string to extract the number from
 *  \return the character code of the escape sequence
 */
int dice::parser::escape_to_char (char *s)
{
	// check for leading 'L'
	if (s[0] == 'L') s++;
	// skip the first ' and backslash
	s += 2; switch (s[0])
	{
	case 'n':
		return '\n';
		break;
	case 't':
		return '\t';
		break;
	case 'v':
		return '\v';
		break;
	case 'b':
		return '\b';
		break;
	case 'r':
		return '\r';
		break;
	case 'f':
		return '\f';
		break;
	case 'a':
		return '\a';
		break;
	case '\\':
		return '\\';
		break;
	case '?':
		return '?';
		break;
	case '\'':
		return '\'';
		break;
	case '"':
		return '"';
		break;
	}
	return 0;
}

/** \brief decode a decimal number from a string containing a hexadecimal number
 *  \ingroup parser
 *  \param s the string containing the hexadecimal number
 *  \retval t the integer type to return
 *  \return the decoded decimal value
 */
#if HAVE_ATOLL
long
#endif
long dice::parser::hex_to_long (char *s, IntType& t)
{
	errno = 0;
	t = int_type(s);
	switch (t)
	{
	case INT_ULLONG:
#if HAVE_ATOLL
		{
			unsigned long long n = strtoull(s, 0, 16);
			if (! (std::numeric_limits<unsigned long long>::min() <= n &&
					n <= std::numeric_limits<unsigned long long>::max() &&
					errno != ERANGE))
				t = INVALID;
			return n;
		}
		break;
#else
		t = INT_ULONG;
#endif
	case INT_ULONG:
		{
			unsigned long n = strtoul(s, 0, 16);
			if (! (std::numeric_limits<unsigned long>::min() <= n &&
					n <= std::numeric_limits<unsigned long>::max() &&
					errno != ERANGE))
				t = INVALID;
			return n;
		}
		break;
	case INT_LLONG:
#if HAVE_ATOLL
		{
			long long n = strtoll(s, 0, 16);
			if (! (std::numeric_limits<long long>::min() <= n &&
					n <= std::numeric_limits<long long>::max() &&
					errno != ERANGE))
				t = INVALID;
			return n;
		}
		break;
#else
		t = INT_LONG;
#endif
	case INT_LONG:
		{
			long n = strtol(s, 0, 16);
			if (! (std::numeric_limits<long>::min() <= n &&
					n <= std::numeric_limits<long>::max() &&
					errno != ERANGE))
			{
#if HAVE_ATOLL
				// check if this was just unsigned max
				long long m = strtoll(s, 0, 16);
				if (m == std::numeric_limits<unsigned long>::max())
				{
					t = INT_ULONG;
					return m;
				}
#endif
				t = INVALID;
			}
			return n;
		}
		break;
	case INT_INT:
		{
			long n = strtol(s, 0, 16);
			if (! (std::numeric_limits<int>::min() <= n &&
					n <= std::numeric_limits<int>::max() &&
					errno != ERANGE))
			{
#if HAVE_ATOLL
				// check if this was just unsigned max
				long long m = strtoll(s, 0, 16);
				if (m == std::numeric_limits<unsigned int>::max())
				{
					t = INT_ULONG;
					return m;
				}
				n = m;
				if (n < 0 &&
					std::numeric_limits<int>::min() <= n &&
					n <= std::numeric_limits<int>::max())
				{
					// first bit was set, so this was a signed long
					t = INT_LONG;
					return n;
				}

#endif
				t = INVALID;
			}
			return n;
		}
		break;
	default:
		break;
	}
	return 0;
}

/** \brief decode a decimal number from a string containing a octal number
 *  \ingroup parser
 *  \param s the string containing the octal number
 *  \retval t the integer type to return
 *  \return the decoded decimal value
 */
#if HAVE_ATOLL
long
#endif
long dice::parser::oct_to_long (char *s, IntType& t)
{
	errno = 0;
	t = int_type(s);
	switch (t)
	{
	case INT_ULLONG:
#if HAVE_ATOLL
		{
			unsigned long long n = strtoull(s, 0, 8);
			if (! (std::numeric_limits<unsigned long long>::min() <= n &&
					n <= std::numeric_limits<unsigned long long>::max() &&
					errno != ERANGE))
				t = INVALID;
			return n;
		}
		break;
#else
		t = INT_ULONG;
#endif
	case INT_ULONG:
		{
			unsigned long n = strtoul(s, 0, 8);
			if (! (std::numeric_limits<unsigned long>::min() <= n &&
					n <= std::numeric_limits<unsigned long>::max() &&
					errno != ERANGE))
				t = INVALID;
			return n;
		}
		break;
	case INT_LLONG:
#if HAVE_ATOLL
		{
			long long n = strtoll(s, 0, 8);
			if (! (std::numeric_limits<long long>::min() &&
					n <= std::numeric_limits<long long>::max() &&
					errno != ERANGE))
				t = INVALID;
			return n;
		}
		break;
#else
		t = INT_LONG;
#endif
	case INT_LONG:
		{
			long n = strtol(s, 0, 8);
			if (! (std::numeric_limits<long>::min() <= n &&
					n <= std::numeric_limits<long>::max() &&
					errno != ERANGE))
				t = INVALID;
			return n;
		}
		break;
	case INT_INT:
		{
			long n = strtol(s, 0, 8);
			if (! (std::numeric_limits<int>::min() <= n &&
					n <= std::numeric_limits<int>::max() &&
					errno != ERANGE))
				t = INVALID;
			return n;
		}
		break;
	default:
		break;
	}
	return 0;
}

/** \brief decode a decimal number from a string containing a integer number
 *  \ingroup parser
 *  \param s the string containing the integer number
 *  \retval t the integer type to return
 *  \return the decoded decimal value
 */
#if HAVE_ATOLL
long
#endif
long dice::parser::int_to_long (char *s, IntType& t)
{
	errno = 0;
	t = int_type(s);
	switch (t)
	{
	case INT_ULLONG:
#if HAVE_ATOLL
		{
			unsigned long long n = strtoull (s, 0, 10);
			if (! (std::numeric_limits<unsigned long long>::min() <= n &&
					n <= std::numeric_limits<unsigned long long>::max() &&
					errno != ERANGE))
				t = INVALID;
			return n;
		}
		break;
#else
		t = INT_ULONG;
#endif
	case INT_ULONG:
		{
			unsigned long n = strtoul(s, 0, 10);
			if (! (std::numeric_limits<unsigned long>::min() <= n &&
					n <= std::numeric_limits<unsigned long>::max() &&
					errno != ERANGE))
				t = INVALID;
			return n;
		}
		break;
	case INT_LLONG:
#if HAVE_ATOLL
		{
			long long n = strtoll (s, 0, 10);
			if (! (std::numeric_limits<long long>::min() <= n &&
					n <= std::numeric_limits<long long>::max() &&
					errno != ERANGE))
				t = INVALID;
			return n;
		}
		break;
#else
		t = INT_LONG;
#endif
	case INT_LONG:
	case INT_INT:
		{
			long n = strtol (s, 0, 10);
			if (! (std::numeric_limits<long>::min() <= n &&
					n <= std::numeric_limits<long>::max() &&
					errno != ERANGE))
				t = INVALID;
			return n;
		}
		break;
	default:
		break;
	}
	return 0;
}
