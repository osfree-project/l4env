/*
 * \brief   DOpE utility functions
 * \date    2003-08-02
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 *
 * This file contains several utility functions used by
 * other components. This way inconsistencies between
 * different libC functionalities can be avoided by just
 * not using libC.
 */

/*
 * Copyright (C) 2002-2003  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include "dopestd.h"


/*** CONVERT A FLOAT INTO A STRING ***
 *
 * This function performs zero-termination of the string.
 *
 * \param v       float value to convert
 * \param prec    number of digits after comma
 * \param dst     destination buffer
 * \param max_len destination buffer size
 */
int dope_ftoa(float v, int prec, char *dst, int max_len) {
	int dig = 0, neg = 0, zero = 0;

	if (!dst) return -1;

	if (v < 0) { v = -v; dig++; neg = 1; }
	
	for (; (int)v > 0; v = v/10.0) dig++;
	
	if (dig == 0) { dig = 1; zero = 1; }
	
	if (dig+prec+2 > max_len) {
		prec = max_len - dig - 1;
		if (prec < 0) {
			*dst = 0;
			return -1;
		}
	}
	
	if (neg)  { *(dst++) = '-'; dig--; }
	if (zero) { *(dst++) = '0'; dig--; }
	
	for (;dig-->0;) {
		v = v*10;
		*(dst++) = '0' + (int)v;
		v = v - (int)v;
	}
	
	if (prec) *(dst++) = '.';
	
	for (;prec--;) {
		v = v*10;
		*(dst++) = '0' + (int)v;
		v = v - (int)v;
	}
	*dst = 0;
	return 0;
}


/*** DETERMINES IF TWO STRINGS ARE EQUAL ***/
int dope_streq(char *s1, char *s2, int max_len) {
	int i;
	if (!s1 || !s2) return 0;
	for (i=0;i<max_len;i++) {
		if (*(s1) != *(s2++)) return 0;
		if (*(s1++) == 0) return 1;
	}
	return 1;
}
