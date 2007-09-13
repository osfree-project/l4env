/*
 * \brief   Header to allow libtcg to be compiled under Linux.
 * \date    2004-06-02
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

#ifndef _SHA_H
#define _SHA_H

/**
 * Access to crypto routines.
 *
 * The defines are not identically with openssl, but nearly the same.
 */

#ifdef LINUX

#include <openssl/sha.h>
#include <openssl/hmac.h>

typedef SHA_CTX SHA1_CTX;
typedef HMAC_CTX HMAC1_CTX;

#else

#include <l4/crypto/sha1.h>

typedef crypto_sha1_ctx_t SHA1_CTX;

struct hmac_handle
{
  SHA1_CTX hmac;
  SHA1_CTX hash;
};

typedef struct hmac_handle HMAC1_CTX;
#endif // not LINUX


int SHA1_init  (SHA1_CTX * ctx);
int SHA1_update(SHA1_CTX * ctx, void * value, unsigned int len);
int SHA1_final (SHA1_CTX * ctx, void * output);

int HMAC_SHA1_init(HMAC1_CTX * ctx, void * key,    unsigned int keylen);
int HMAC_update   (HMAC1_CTX * ctx, void * value,  unsigned int len);
int HMAC_final    (HMAC1_CTX * ctx, void * output, unsigned int *len);

#endif
