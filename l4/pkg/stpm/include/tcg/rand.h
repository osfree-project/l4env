/*
 * \brief   Header for random number functions.
 * \date    2006-07-24
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2006  Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


#ifndef _RAND_H
#define _RAND_H


int rand_buffer(unsigned char *buffer,int num);

unsigned long
TPM_GetRandom(unsigned long count,
	      unsigned long *length,
	      unsigned char *random);

unsigned long
TPM_StirRandom(unsigned long count,
	       unsigned char *random);

#endif
