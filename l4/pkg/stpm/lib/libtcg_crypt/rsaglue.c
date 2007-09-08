/*
 * \brief   Simple glue code for different rsa implementations.
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

/**
 * 
 */

#include <stdio.h>
#include <stdarg.h>
#include <assert.h>
#include "rsaglue.h"


#define RSA_FUNCTION(NAME, VARS, INIT, BODY, END)		\
int rsa_##NAME(rsa_key_t *key, int enclen, char *enc, ...)	\
{								\
	int res;						\
	va_list argp;						\
	int len;						\
	char *data;						\
	VARS;							\
								\
	va_start(argp, enc);					\
	assert(enc);						\
	INIT;							\
								\
	if (res)						\
		return -1;					\
	while (1) {						\
		len = (int) va_arg(argp, int);			\
		if (len<=0)					\
			break;					\
		data = (char *) va_arg(argp, char *);		\
		assert(data);					\
		BODY;						\
                assert(!res);					\
	}							\
	END;							\
	return res;						\
}

/**
 * Sign data with a key. The variable arguments are (char *value, int
 * count) pairs. The last variable argument must be 0!
 */

RSA_FUNCTION(sign,
	      R_SIGNATURE_CTX ctx;
	      ,
	      assert(enclen>=MAX_SIGNATURE_LEN);
	      res=R_SignInit(&ctx, DA_MD5);
	      ,
	      res=R_SignUpdate(&ctx,(unsigned char*)data,len);
	      ,
	      res=R_SignFinal(&ctx, (unsigned char*)enc, (unsigned int*)&enclen, &key->priv);
	      if (res)
	      		return -2;
	      res=enclen;
	);

/**
 * Encrypt data with a pubkey. The varible arguments are (char *value,
 * int count) pairs, which are encrypted into enc.
 *
 * Returns the number of bytes written.
 *
 * The output is consists of [enckey_len,enckey,encdata]
 */

RSA_FUNCTION(encrypt,
	      R_ENVELOPE_CTX ctx;
	      unsigned char *encryptedKeys[1];
	      R_RSA_PUBLIC_KEY *pubkey[1];
	      unsigned char iv[8];
	      int outlen;
	      unsigned int l;
	      ,
	      // store the size and the encrypted key as first part of the enc
	      outlen=sizeof(unsigned int)+MAX_ENCRYPTED_KEY_LEN;
	      encryptedKeys[0]=enc+sizeof(unsigned int);

	      assert(enclen>=outlen);
	      
	      pubkey[0]=&key->pub;
	      res=R_SealInit(&ctx, 
			     encryptedKeys, 
			     (unsigned int *)enc,
			     iv,
			     1, 
			     pubkey,
			     EA_DES_EDE3_CBC,
			     &key->random);

	      // correct outlen
	      l=sizeof(unsigned int)+(*(unsigned int *)enc);
	      assert(l<=outlen);
	      outlen=l;
	      ,
	      if ((enclen-outlen)<((len+7)/8)*8)
		{
		  printf("=%s() enclen %d outlen %d len %d\n",__func__,enclen,outlen,len);
		  return -2;
		}
	      res=R_SealUpdate(&ctx,(unsigned char*)enc+outlen,&l,(unsigned char*)data,len);
	      assert(l>0);
	      outlen+=l;
	      ,
	      if (enclen<(outlen+8))
	        return -3;
	      res=R_SealFinal(&ctx,(unsigned char*)enc,(unsigned int*)&l);
	      if (res)
	      return -4;
	      assert(l==8);
	      outlen+=l;
	      res=outlen;	      
	);



/**
 * Decrypt data with a privkey. The varible arguments are (char
 * *value, int count) pairs, which are decrypted from enc.
 *
 * Returns the number of bytes written.
 */
RSA_FUNCTION(decrypt,
	      R_ENVELOPE_CTX ctx;
	      unsigned char iv[8];
	      int outlen;
	      unsigned int l;
	      ,
	      outlen=sizeof(unsigned int) + *(unsigned int *)enc;
	      assert(enclen>=outlen);
	      res=R_OpenInit(&ctx,
			     EA_DES_EDE3_CBC,
			     (unsigned char*)enc+sizeof(unsigned int),
			     *(unsigned int *)enc,
			     iv,
			     &key->priv);	      
	      ,	      
	      if ((enclen-outlen)<((len+7)/8)*8)
			return -2;
	      res=R_OpenUpdate(&ctx,(unsigned char*)enc+outlen,&l,(unsigned char*)data,len);
	      assert(l>0);
	      outlen+=l;
	      ,
	      if (enclen<(outlen+8))
			return -3;
	      res=R_OpenFinal(&ctx,(unsigned char*)enc,&l);
	      assert(l<8);
	      outlen+=l;
	      if (res)
	      		return -4;
	      res=outlen;
	);


/**
 * Create a new rsa key.
 */
int
rsa_create(rsa_key_t *key, int bits)
{
	int res;
	R_RSA_PROTO_KEY proto;

	proto.bits=bits;
	proto.useFermat4=1;

	res = R_GeneratePEMKeys(&key->pub,&key->priv,&proto,&key->random);
	return res;
}


/**
 * Init random for key generation.
 */
int
rsa_initrandom(rsa_key_t *key)
{
	R_RandomInit(&key->random);
        return 0;
}


/**
 * Insert random for key generation.
 *
 * Returns the bytes of random needed after this insert.
 */
int
rsa_insertrandom(rsa_key_t *key, int srclen, char *src)
{
	int res;
        unsigned int needed;
	res=R_RandomUpdate(&key->random,(unsigned char*)src,srclen);
	if (res)
		return res;
	R_GetRandomBytesNeeded(&needed,&key->random);
	return needed;
}
