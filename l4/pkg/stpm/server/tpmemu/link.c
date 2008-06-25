/*
 * \author  Alexander Boettcher <boettcher@tudos.org>
 */
/*
 * Copyright (C) 2008
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the libcrypto package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */
#include <l4/crypto/aes.h> //aes
#include <l4/crypto/cbc.h> // cbc_encrypt
#include <tcg/basic.h> //TCG_HASH_SIZE
#include <tcg/seal.h>  //STPM_Seal
#include <tcg/pcrs.h>  //STPM_GenPCRInfo
#include <tcg/oiaposap.h> //sha1 
#include "local.h"

#include <stdlib.h> //malloc

static const char null_iv[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static int getMaxPCRs(void)
{
  int major, minor, version, rev;
  int error;
  unsigned long rpcrs;

  error = STPM_GetCapability_Pcrs(&rpcrs);

  if (error)
  {
    error = STPM_GetCapability_Version(&major, &minor, &version, &rev);

    if (error)
      return -1;
    else
    {
      if (major == 1 && minor == 1)
        if (version == 0 && rev == 0)
        {
          return 24;
        }
      // 16 should be ever available
      return 16;
    }
  } else
    return rpcrs;
 
}

int
_seal_TPM(unsigned char * outdata, unsigned int * outdatalen,
         unsigned char * indata, int indatalen)
{
  int error;
  unsigned short select_count;
  //TODO start
  unsigned int keyhandle = 0x40000000;
  unsigned char keyhash [20] = "b";
  unsigned char authhash [20] = "test";
  //TODO end
  int maxPCRs;
  unsigned char aes_key[AES128_KEY_SIZE];
  const unsigned int aes_key_bytes = sizeof(aes_key);
  const unsigned long aes_outdatalen = CRYPTO_FIT_SIZE_TO_CIPHER(indatalen, AES_BLOCK_SIZE);
  unsigned char * aes_outdata;
  unsigned long sizeoutdata = *outdatalen;

  // sanity check
  if (aes_outdatalen > *outdatalen)
  {
//    LOG("aes_outdatalen %lu outdatalen=%d", aes_outdatalen, *outdatalen );
    return -1;
  }

  // init steps
  sha1(authhash, strlen((char *)authhash), authhash);
  sha1(keyhash, strlen((char *)keyhash), keyhash);

  maxPCRs = getMaxPCRs();
  if (maxPCRs < 16)
    return -2;

  select_count = maxPCRs >> 3;
  if (maxPCRs % 8 != 0)
    select_count += 1;

  unsigned int pcrinfolen = 2 + select_count + 2 * TCG_HASH_SIZE;
  unsigned char pcrinfo [pcrinfolen];
  unsigned char pcrmap [select_count];

  //we want all PCRs
  memset(pcrmap, 0xFF, select_count);

  //reads all PCRs
  if (STPM_GenPCRInfo(select_count, pcrmap, pcrinfo, &pcrinfolen))
    return -3;

  // generate AES key 
  if (rand_buffer(aes_key, aes_key_bytes) < 1)
    return -4;

  crypto_aes_ctx_t  aes_ctx;
  unsigned int      aes_flags = 0;
  if (aes_cipher_set_key(&aes_ctx, (char *)aes_key, AES128_KEY_SIZE, &aes_flags))
    return -5;

  aes_outdata = malloc (aes_outdatalen );
  if (aes_outdata == 0)
    return -6;

  // encrypt with AES
  crypto_cbc_encrypt(aes_cipher_encrypt, &aes_ctx, AES_BLOCK_SIZE,
                     (char *)indata, (char *)aes_outdata, null_iv, indatalen);

  //seal AES key
  error = STPM_Seal(keyhandle, pcrinfo, pcrinfolen, keyhash, authhash,
                    aes_key, aes_key_bytes, outdata + sizeof(*outdatalen), outdatalen);

  if (!error)
  {
//    LOG("key encrypted size=%d, aes encrypted size=%lu, input data=%d",
//        *outdatalen, aes_outdatalen, indatalen);
    if (sizeoutdata < *outdatalen + aes_outdatalen + sizeof(*outdatalen))
    {
      free(aes_outdata);
      return -7;
    }
    // remember size of sealed data
    *(unsigned int *)outdata = *outdatalen;
    // copy AES encrypted data behind TPM sealed aes key
    memcpy(outdata + sizeof(*outdatalen) + *outdatalen, aes_outdata, aes_outdatalen);
    *outdatalen += aes_outdatalen + sizeof(*outdatalen);
  }

  free(aes_outdata);

  return error;
}

int
_unseal_TPM(unsigned int sealedbloblen,
            unsigned char * sealedblob,
            unsigned int * unsealedlen,
            unsigned char ** unsealedblob)
{
  unsigned long error;
  unsigned int len1;
  //TODO start
  unsigned int keyhandle = 0x40000000;
  unsigned char keyhash [20] = "b";
  unsigned char authhash [20] = "test";
  //TODO end
  unsigned short select_count;
  unsigned char aes_key[AES128_KEY_SIZE];
  unsigned int aes_key_bytes = sizeof(aes_key);
  crypto_aes_ctx_t  aes_ctx;
  unsigned int      aes_flags = 0;
  int maxPCRs;

  sha1(authhash, strlen((char *)authhash), authhash);
  sha1(keyhash, strlen((char *)keyhash), keyhash);

  maxPCRs = getMaxPCRs();
  if (maxPCRs < 16)
    return -1;
  select_count = maxPCRs >> 3;
  if (maxPCRs % 8 != 0)
    select_count += 1;

  // unseal AES key
  len1 = *(unsigned int *)sealedblob;

  error = STPM_Unseal(keyhandle, keyhash, authhash,
                      sealedblob + sizeof(len1), len1,
                      aes_key, &aes_key_bytes);
  if (error)
    return -2;

  if (aes_key_bytes != sizeof(aes_key))
    return -3;

  // decrypt with AES key vTPM data
  if (aes_cipher_set_key(&aes_ctx, (char *)aes_key, AES128_KEY_SIZE, &aes_flags))
    return -4;

  *unsealedlen = sealedbloblen - sizeof(len1) - len1;
  *unsealedblob = malloc(*unsealedlen);
  if (unsealedblob == 0)
    return -5;

  crypto_cbc_decrypt(aes_cipher_decrypt, &aes_ctx, AES_BLOCK_SIZE,
                     (char *) (sealedblob + sizeof(len1) + len1), (char *)*unsealedblob, null_iv, *unsealedlen);

  return error;
}

