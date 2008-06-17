/*
 * \brief   Header for monotonic counter.
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

#ifndef _COUNTER_H
#define _COUNTER_H

typedef struct counter_value
{
  unsigned short tag;
  unsigned long label;
  unsigned long value;
} counter_value;


unsigned long
STPM_CreateCounter(unsigned char *owner_auth,
		  unsigned char *counter_auth,
		  unsigned long label,
		  unsigned long *id,
		  counter_value *value);
unsigned long
STPM_IncrementCounter(unsigned long id,
		     unsigned char *auth,
		     counter_value *value);
unsigned long
STPM_ReadCounter(unsigned long id,
		counter_value *value);

unsigned long
STPM_ReleaseCounter(unsigned long id,
		   unsigned char *auth);
unsigned long
STPM_ReleaseCounterOwner(unsigned long id,
			unsigned char *owner_auth);
#endif
