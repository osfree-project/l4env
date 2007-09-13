/*
 * \brief   Header for RSA glue code.
 * \date    2004-11-12
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

#ifndef _STPM_RSAGLUE_H
#define _STPM_RSAGLUE_H

#include <l4/crypto/rsaref2/global.h>
#include <l4/crypto/rsaref2/rsaref.h>

typedef R_RSA_PRIVATE_KEY privkey_t;
typedef R_RSA_PUBLIC_KEY pubkey_t;
typedef R_RANDOM_STRUCT random_t;

typedef struct
{
	privkey_t priv;
	pubkey_t  pub;
	random_t random;
} rsa_key_t;

#define RSA_SIGSIZE MAX_SIGNATURE_LEN
#define RSA_HASHSIZE 20


int
rsa_sign(rsa_key_t *key, int dstlen, unsigned char *dst, ...);

int
rsa_create(rsa_key_t *key, int bits);

int
rsa_initrandom(rsa_key_t *key);

int
rsa_insertrandom(rsa_key_t *key, int srclen, unsigned char *src);

int
rsa_encrypt(rsa_key_t *key, int dstlen, unsigned char *dst, ... );

int
rsa_decrypt(rsa_key_t *key, int dstlen, unsigned char *dst, ...);

#endif /* _STPM_RSAGLUE_H */
