/*
 * \brief   TPM SHA1 functions.
 * \date    2004-09-14
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2004  Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */


#include <netinet/in.h>
#include <tcg/tpm.h>
#include <tcg/ord.h>
#include <tcg/transmit.h>
#include <tcg/buildbuff.h>

TPM_TRANSMIT_FUNC(SHA1Start,
		  (unsigned long *value),
		  ,
		  *value = TPM_EXTRACT_LONG(0);,
		  "");


TPM_TRANSMIT_FUNC(SHA1Update,
		  (unsigned char *data, unsigned long datalen),
		  ,
		  ,
		  "@",		  
		  datalen, data);

TPM_TRANSMIT_FUNC(SHA1Complete,
		  (unsigned char *data, unsigned long datalen, unsigned char *hash),
		  ,
		  TPM_COPY_FROM(hash, 0, TCG_HASH_SIZE);
		  ,
		  "@",
		  datalen, data);
