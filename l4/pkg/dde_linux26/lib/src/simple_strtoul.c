/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux26/lib/src/simple_strtoul.c
 * \brief  simple string to unsigned long transformation
 *
 * \author Marek Menzer <mm19@os.inf.tu-dresden.de>
 *
 * Original by Lars Wirzenius & Linus Torvalds

 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <stdarg.h>
#include <l4/sys/types.h>

#include "ctype.h"

unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base);
unsigned long simple_strtoul(const char *cp,char **endp,unsigned int base)
{
	unsigned long result = 0,value;

	if (!base) {
		base = 10;
		if (*cp == '0') {
			base = 8;
			cp++;
			if ((*cp == 'x') && isxdigit(cp[1])) {
				cp++;
				base = 16;
			}
		}
	}
	while (isxdigit(*cp) && (value = isdigit(*cp) ? *cp-'0' : (islower(*cp)
	    ? toupper(*cp) : *cp)-'A'+10) < base) {
		result = result*base + value;
		cp++;
	}
	if (endp)
		*endp = (char *)cp;
	return result;
}
