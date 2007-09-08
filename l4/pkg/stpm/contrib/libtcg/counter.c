/*
 * \brief   Monotonic counter routines.
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


#include <tcg/oiaposap.h>
#include <tcg/counter.h>

/**
 * Create a new monotonic counter.
 */
TPM_TRANSMIT_OSAP_FUNC(CreateCounter,
		       (unsigned char *owner_auth,
			unsigned char *counter_auth,
			unsigned long label,
			unsigned long *id,
			counter_value *value),
		       owner_auth,
		       0x0002,
		       0,
		       unsigned char enc_auth[TCG_HASH_SIZE];

		       if (counter_auth == NULL
			   || id == NULL 
			   || value == NULL)
			 return -1;
		       ,
		       CALC_ENC_AUTH(enc_auth, counter_auth)
		       ,		       
		       if (TPM_EXTRACT_LONG(6)!=label)
			 return -2;
		       *id = TPM_EXTRACT_LONG(0);
		       TPM_COPY_FROM(value, 4, 10);
		       ,
		       "% L",
		       "% L",
		       TCG_HASH_SIZE,
		       enc_auth,
		       label);



TPM_TRANSMIT_OIAP_FUNC(IncrementCounter,
		       (unsigned long id,
			unsigned char *auth,
			counter_value *value),
		       auth,
		       if (value==NULL)
			 return -1;
		       ,
		       TPM_COPY_FROM(value, 0, 10);
		       ,
		       "L",
		       "L",
		       id);


TPM_TRANSMIT_FUNC(ReadCounter,
		  (unsigned long id,
		   counter_value *value),
		  if (value==NULL)
		    return -1;
		  ,
		  TPM_COPY_FROM(value, 0, 10);
		  ,
		  "L",
		  id
		  );


TPM_TRANSMIT_OIAP_FUNC(ReleaseCounter,
		       (unsigned long id,
			unsigned char *auth),
		       auth,
		       ,		       
		       ,
		       "L",
		       "L",
		       id);



TPM_TRANSMIT_OSAP_FUNC(ReleaseCounterOwner,
		       (unsigned long id,
			unsigned char *owner_auth),
		       owner_auth,
		       0x0002,
		       0,
		       ,
		       ,		       
		       ,
		       "L",
		       "L",
		       id);
