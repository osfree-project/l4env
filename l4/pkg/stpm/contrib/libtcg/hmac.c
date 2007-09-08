/****************************************************************************/
/*                                                                          */
/*               TPM Specific HMAC routines                                 */
/*                                                                          */
/* This file is copyright 2003 IBM. See "License" for details               */
/*                                                                          */
/* Beautified by Bernhard Kauer <kauer@tudos.org>                           */
/*                                                                          */
/****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <netinet/in.h>
#include <tcg/tpm.h>
#include <tcg/hmac.h>
#include <cryptoglue.h>

/**
 *
 * Validate the HMAC in AUTH1 or AUTH2 response.
 *
 * Note: Only the last HMAC is checked on an AUTH2 response.
 *
 * @param buffer  - a pointer to response buffer
 * @param command - the command code from the original request
 * @param ononce  - a 20 byte array containing the oddNonce
 * @param key     - a 20 byte array containing the key used in the request HMAC
 * @param data_len - the number of response bytes after TCG_DATA_OFFSET in the buffer
 *
 */

int 
checkhmac(unsigned char *buffer, unsigned long command,
	   unsigned char *ononce, unsigned char *key, int data_len)
{
	unsigned long bufsize;
	unsigned short tag;
	unsigned long ordinal;
	unsigned long result;
	unsigned char *enonce;
	unsigned char *continueflag;
	unsigned char *authdata;
	unsigned char testhmac[20];
	unsigned char paramdigest[20];
	SHA1_CTX sha;
	
	bufsize = TPM_EXTRACT_LONG(-8);
	tag = TPM_EXTRACT_SHORT(-10);
	ordinal = ntohl(command);
	result = *(unsigned long *) (buffer + TCG_RETURN_OFFSET);

	printf("> checkhmac1 %ld %x %lx %lx\n", bufsize, tag, ordinal, result);

	if (tag == TPM_TAG_RSP_COMMAND)
		return 0;
	if (tag != TPM_TAG_RSP_AUTH1_COMMAND && tag != TPM_TAG_RSP_AUTH2_COMMAND)
		return -1;
	authdata = buffer + bufsize - TCG_HASH_SIZE;
	continueflag = authdata - 1;
	enonce = continueflag - TCG_NONCE_SIZE;
    
	SHA1_init(&sha);
	SHA1_update(&sha, &result, 4);
	SHA1_update(&sha, &ordinal, 4);
	SHA1_update(&sha, buffer + TCG_DATA_OFFSET, data_len);
	PRINT_HASH_SIZE((buffer + TCG_DATA_OFFSET), data_len);
	SHA1_final(&sha, paramdigest);

	rawhmac(testhmac, key, TCG_HASH_SIZE, TCG_HASH_SIZE, paramdigest,
		TCG_NONCE_SIZE, enonce,
		TCG_NONCE_SIZE, ononce, 1, continueflag, 0, 0);
	PRINT_HASH(testhmac);
	PRINT_HASH(authdata);
	if (memcmp(testhmac, authdata, TCG_HASH_SIZE) != 0)
		return -2;
	printf("< checkhmac1()\n");
	return 0;
}

#define DEBUG
/**
 * Helper function to split paramdigest calulation from hmac
 */
static int 
calcParamDigest(unsigned char *paramdigest,va_list argp)
{
	SHA1_CTX sha;
	unsigned int dlen;
	unsigned char *data;

	SHA1_init(&sha);
	for (;;) {
		dlen = (unsigned int) va_arg(argp, unsigned int);
		if (dlen == 0)
			break;
		data = (unsigned char *) va_arg(argp, int);
		if (data == NULL)
			return -1;
#ifdef DEBUG
		PRINT_HASH_SIZE(data,dlen);
#endif
		SHA1_update(&sha, data, dlen);
	}
	SHA1_final(&sha, paramdigest);
#ifdef DEBUG
	PRINT_HASH(paramdigest);
#endif
	return 0;
}

/****************************************************************************/
/*                                                                          */
/* Calculate HMAC value for an AUTH1 command                                */
/*                                                                          */
/* This function calculates the Authorization Digest for all OIAP           */
/* commands.                                                                */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* digest - a pointer to a 20 byte array that will receive the result       */
/* key    - a pointer to the key to be used in the HMAC calculation         */
/* keylen - the size of the key in bytes                                    */
/* h1     - a pointer to a 20 byte array containing the evenNonce           */
/* h2     - a pointer to a 20 byte array containing the oddNonce            */
/* h3     - an unsigned character containing the continueAuthSession value  */
/* followed by a variable length set of arguments, which must come in       */
/* pairs.                                                                   */
/* The first value in each pair is the length of the data in the second     */
/*   argument of the pair                                                   */
/* The second value in each pair is a pointer to the data to be hashed      */
/*   into the paramdigest.                                                  */
/* The last pair must be followed by a pair containing 0,0                  */
/*                                                                          */
/****************************************************************************/
int 
authhmac(unsigned char *digest,
	 unsigned char *key,
	 unsigned int keylen,
	 unsigned char *h1, 
	 unsigned char *h2,
	 unsigned char h3,
	 ...)
{
        unsigned char c;
	va_list argp;
	unsigned char paramdigest[20];
	int res;  

	if (h1 == NULL || h2 == NULL)
		return -1;
  
	va_start(argp, h3);
	res = calcParamDigest(paramdigest,argp);
        if (res != 0)
            return res;
	va_end(argp);

        c = h3;
	/* calculate authorization HMAC value */
	res=rawhmac(digest,
		    key, keylen,
		    TCG_HASH_SIZE, paramdigest, 
		    TCG_NONCE_SIZE, h1, 
		    TCG_NONCE_SIZE, h2, 
		    1, &c,
		    0, 0);      
	return res;
}



/****************************************************************************/
/*                                                                          */
/* Calculate Raw HMAC value                                                 */
/*                                                                          */
/* This function calculates an HMAC digest                                  */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* digest - a pointer to a 20 byte array that will receive the result       */
/* key    - a pointer to the key to be used in the HMAC calculation         */
/* keylen - the size of the key in bytes                                    */
/* followed by a variable length set of arguments, which must come in       */
/* pairs.                                                                   */
/* The first value in each pair is the length of the data in the second     */
/*   argument of the pair                                                   */
/* The second value in each pair is a pointer to the data to be hashed      */
/*   into the paramdigest.                                                  */
/* The last pair must be followed by a pair containing 0,0                  */
/*                                                                          */
/****************************************************************************/

int 
rawhmac(unsigned char *digest, unsigned char *key,
	unsigned int keylen, ...)
{
	HMAC1_CTX hmac;
	unsigned int dlen;
	unsigned char *data;
	va_list argp;
    
	HMAC_SHA1_init(&hmac, key, keylen);
#ifdef DEBUG
	PRINT_HASH(key);
#endif
	va_start(argp, keylen);
	for (;;) {
		dlen = (unsigned int) va_arg(argp, unsigned int);
		if (dlen == 0)
			break;
		data = (unsigned char *) va_arg(argp, int);
		if (data == NULL)
			return -1;
#ifdef DEBUG
		PRINT_HASH_SIZE(data,dlen)
#endif
		HMAC_update(&hmac, data, dlen);
	}

	dlen=TCG_HASH_SIZE;    // this was missing in the original source
	HMAC_final(&hmac, digest, &dlen);
	va_end(argp);
	return 0;
}

/****************************************************************************/
/*                                                                          */
/* Perform a SHA1 hash on a single buffer                                   */
/*                                                                          */
/****************************************************************************/
void
sha1(unsigned char *input, int len, unsigned char *output)
{
	SHA1_CTX sha;
	int res;
    
	//printf("> %s\n",__func__);
	SHA1_init(&sha);
	SHA1_update(&sha, input, len);
	res=SHA1_final(&sha, output);
	if (res!=0)
		printf("%s() Error: final %d\n",__func__,res);
	// printf("< %s\n",__func__);


}
