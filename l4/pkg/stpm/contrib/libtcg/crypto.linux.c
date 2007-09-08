/*
 * \brief   Simple glue code to run our library under linux with
 *          openssl and /dev/tpm.
 * \date    2006-09-14
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


#include <cryptoglue.h>

int SHA1_init  (SHA1_CTX * ctx)
{
	SHA1_Init  (ctx);
	return 0;
}

int SHA1_update(SHA1_CTX * ctx, void * value, int len)
{
	SHA1_Update(ctx,value,len);
	return 0;
}

int SHA1_final (SHA1_CTX * ctx, void * output)
{
	SHA1_Final(output,ctx);
	return 0;
}

int HMAC_SHA1_init(HMAC1_CTX * ctx, void * key, int keylen)
{
	HMAC_Init(ctx, key, keylen, EVP_sha1());
	return 0;
}

int HMAC_update   (HMAC1_CTX * ctx, void * value,  int len)
{
	HMAC_Update(ctx,value,len);
	return 0;
}

int HMAC_final    (HMAC1_CTX * ctx, void * output, int *len)
{
	HMAC_Final(ctx,output,len);
	return 0;
}


