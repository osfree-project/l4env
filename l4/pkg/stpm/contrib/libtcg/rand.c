/*
 * \brief   Random number functions.
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


#include <tcg/tpm.h>


/**
 * Get hardware random from the TPM.
 */
TPM_TRANSMIT_FUNC(GetRandom,
		  (unsigned long count,
		   unsigned long *length,
		   unsigned char *random),
		  if (length == NULL || random == NULL)
		    return -1;
		  ,
		  *length = TPM_EXTRACT_LONG(0);
		  TPM_COPY_FROM(random, 4, *length);
		  ,
		  "L",
		  count);

/**
 * Add entropy to the random number generator of the TPM.
 */
TPM_TRANSMIT_FUNC(StirRandom,
		  (unsigned long count,
		   unsigned char *random),
		  if (random == NULL)
		    return -1;
		  // check limit mentioned in the spec
		  if (count >= 256)
		    return -2;
		  ,
		  ,
		  "@",
		  count,
		  random);
