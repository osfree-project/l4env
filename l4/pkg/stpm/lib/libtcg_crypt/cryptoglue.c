/*
 * \brief   This file defines some functions to implement sha1 and hmac in a
 *          openssl-like interface.
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

#include <string.h>
#include <cryptoglue.h>

int SHA1_init  (SHA1_CTX * ctx)
{
  sha1_digest_setup(ctx);
  return 0;
}

int SHA1_update(SHA1_CTX * ctx, void * value, int len)
{
  // we hope the sha1 code can handle large strings to hash
  sha1_digest_update(ctx, value, len);
  return 0;

#if 0
  int res=-1;
  int count=0;  

  while (len>0)
    {
      res=SHA1Input(ctx,((char *)value)+count,len<MAX_DATA_LEN?len:MAX_DATA_LEN);
      count+=MAX_DATA_LEN;
      len-=MAX_DATA_LEN;
    }
  return res;
#endif
}

int SHA1_final (SHA1_CTX * ctx, void * output)
{
  sha1_digest_final(ctx, output);
  return 0;
}

int HMAC_SHA1_init(HMAC1_CTX * ctx, void * key, int keylen)
{
  int res;
  unsigned char h_key[64];
  unsigned char h_ipad[64];
  unsigned char h_opad[64];
  unsigned i;

  // hash key
  if (keylen>64)
    {
      SHA1_CTX hctx;

      if ((res = SHA1_init(&hctx))) return res;
      if ((res = SHA1_update(&hctx, key, keylen))) return res;
      if ((res = SHA1_final(&hctx, h_key))) return res;
      keylen = SHA1HashSize;
    }
  else
    memcpy(h_key, key, keylen);
  
  // copy key into inner and outer padding
  memset(h_ipad, 0, sizeof(h_ipad));
  memset(h_opad, 0, sizeof(h_opad));
  memcpy(h_ipad, h_key, keylen);
  memcpy(h_opad, h_key, keylen);

  // xor with padding
  for (i=0; i<64; i++)
    {
      h_ipad[i] ^= 0x36;
      h_opad[i] ^= 0x5c;
    }
  
  // start inner hash
  if ((res = SHA1_init(&ctx->hash))) return res;
  if ((res = SHA1_update(&ctx->hash, h_ipad, 64))) return res;
  

  // start outer hash
  if ((res = SHA1_init(&ctx->hmac))) return res;
  if ((res = SHA1_update(&ctx->hmac, h_opad, 64))) return res;

  return res;
}


int HMAC_update   (HMAC1_CTX * ctx, void * value,  int len)
{
  return SHA1_update(&ctx->hash, value, len);
}

int HMAC_final    (HMAC1_CTX * ctx, void * output, int *len)
{
  int res;
  char digest[SHA1HashSize];

  if ((res = SHA1_final(&ctx->hash,digest))) return res;
  if ((res = SHA1_update(&ctx->hmac, digest, SHA1HashSize))) return res;
  if ((res = SHA1_final(&ctx->hmac, output))) return res;

  *len = SHA1HashSize;

  return res;
}
