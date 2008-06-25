/*
 * Copyright (C) 2007 Alexander Boettcher <boettcher@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <tcg/quote.h>
#include <tcg/tpm.h>
#include <tcg/pcrs.h>
#include <tcg/seal.h>
#include <cryptoglue.h> //SHA1

#include "tpmrun.h"
#include "encap.h" //stpm_check_server

#include <stdlib.h>
/*
struct tpm_pcr_selection2
{
        unsigned short size;
        unsigned short select;
};

struct tpm_pcr_value
{
        unsigned char value[20];
};


struct tpm_pcr_composite
{
        struct tpm_pcr_selection2 select;
        unsigned long valuesize;
        struct tpm_pcr_value value[16]; // number of PCRs 
};

*/
int quotePCRs(unsigned int    keyhandle,
              unsigned char * passhash,
              unsigned char * noncehash,
              unsigned char * output,
              unsigned int  * outputlen,
              unsigned char * pcrcomposite,
              unsigned int  pcrlen,
              unsigned int maxPCRs)
{
  unsigned char pcrselect[20];
  unsigned short select_count;
  unsigned long size_composite;
  unsigned long value_count;
  int res;

  // the select count must be in bytes and in big endian
  select_count = maxPCRs >> 3;
  if (maxPCRs % 8 != 0)
    select_count += 1;

  // sanity check
  if (select_count + 2 > sizeof(pcrselect))
    return -2;

  // 2 bytes select count + select bytes used +
  //  4 byte for value count + maxPCRs * 20 value bytes
  size_composite = 2 + select_count + 4 + (maxPCRs * 20);

  // sanity check
  if (size_composite > pcrlen)
    return -3;

  // set the size/count to pcrselect (the first 2 bytes)
  // convert it to network byte order (big endian)
  *((unsigned short *)pcrselect) = htons(select_count);

  // select starts after first 2 bytes
  // request all PCRs
  memset(&pcrselect[2], 0xFF, select_count);
    
  res= TPM_Quote(keyhandle, passhash, 
                  pcrselect, 
                  noncehash, 
                  pcrcomposite,
                  output, outputlen);

  if (res)
    return res;

  value_count = ntohl(*(unsigned long *)&pcrcomposite[2 + select_count]);   
  
  //sanity check
  if (value_count % 20 != 0)
    return -4;

  return 0;
}

int quote_vTPM(unsigned int    keyhandle,
               unsigned char * passhash,
               unsigned char * noncehash,
               unsigned char * output,
               unsigned int  * outputlen,
               unsigned char * pcrcomposite,
               unsigned int  pcrlen,
               unsigned int maxPCRs,
               unsigned char * sTPM,
               unsigned char * vTPM)
{
  int error;
  unsigned short select_count;
	SHA1_CTX sha;
  unsigned char nonce_vTPM_hash[TCG_HASH_SIZE];
  
  select_count = maxPCRs >> 3;
  if (maxPCRs % 8 != 0)
    select_count += 1;

  error = stpm_check_server((char *)vTPM, 1);
  if (error)
    return -1;

  unsigned int pcrinfolen = 2 + select_count + 2 * TCG_HASH_SIZE;
  unsigned char pcrinfo [pcrinfolen];
  unsigned char pcrmap [select_count];

  //we want all PCRs
  memset(pcrmap, 0xFF, select_count);

  //reads all PCRs by calling TPM_PcrRead several times and hash them to a sha1 value
  if (STPM_GenPCRInfo(select_count, pcrmap, pcrinfo, &pcrinfolen))
    return -2;

  // hash nonce + pcrinfo(vtpm)
	SHA1_init(&sha);
	SHA1_update(&sha, noncehash, TCG_HASH_SIZE);
	SHA1_update(&sha, pcrinfo, pcrinfolen);

	SHA1_final(&sha, &nonce_vTPM_hash);

  // Quote of sTPM
  error = stpm_check_server((char *)sTPM, 1);
  if (error)
    return -3;

  return quotePCRs(keyhandle, passhash, nonce_vTPM_hash, output, outputlen,
                   pcrcomposite, pcrlen, maxPCRs);
}

int seal_TPM(int keyhandle, unsigned char * keyhash, unsigned char * authhash,
             int maxPCRs, unsigned int * outdatalen, unsigned char * outdata,
             unsigned int indatalen, unsigned char * indata)
{
  int error;
  unsigned short select_count;

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
  {
    return -2;
  }

  error = TPM_Seal(keyhandle, pcrinfo, pcrinfolen, keyhash, authhash,
                   indata, indatalen, outdata, outdatalen);

  return error;
}

int unseal_TPM(int keyhandle, unsigned char *keyhash, unsigned char *authhash,
               int maxPCRs, unsigned int sealedbloblen,
               unsigned char * sealedblob)
{
  unsigned long error;
  unsigned char *unsealedblob;
  unsigned int unsealedlen = 512;

  unsealedblob = malloc(unsealedlen);
  if (unsealedblob == 0)
    return -1;

  error = STPM_Unseal(keyhandle, keyhash, authhash,
                      sealedblob, sealedbloblen,
                      unsealedblob, &unsealedlen);
  printf("\n%s\n",unsealedblob);
  free(unsealedblob);

  return error;
}
