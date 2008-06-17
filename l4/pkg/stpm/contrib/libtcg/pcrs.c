/****************************************************************************/
/*                                                                          */
/*                        TCPA PCR Processing Functions                     */
/*                                                                          */
/* This file is copyright 2003 IBM. See "License" for details               */
/*                                                                          */
/* Beautified by Bernhard Kauer <kauer@tudos.org>                           */
/*                                                                          */
/****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <tcg/buildbuff.h>
#include <cryptoglue.h>
#include <tcg/pcrs.h>
#include <tcg/tpm.h>
#include <tcg/transmit.h>
#include <tcg/ord.h>

/**
 * Read a pcr value.
 * Returns the value of the pcr in pcrvalue.
 */
TPM_TRANSMIT_FUNC(PcrRead, 
		  (unsigned long pcrindex, unsigned char *pcrvalue),
		  if (pcrvalue==NULL) return -1;,
		  TPM_COPY_FROM(pcrvalue, 0, TCG_HASH_SIZE);,
		  "L", 
		  pcrindex);

/**
 * Extend a pcr with a hash.
 * Returns the value of the pcr in hash.
 */
TPM_TRANSMIT_FUNC(Extend, 
		  (unsigned long pcrindex, unsigned char *hash),
		  if (hash==NULL) return -2;,
		  TPM_COPY_FROM(hash, 0, TCG_HASH_SIZE);,
		  "L %",
		  pcrindex, 
		  TCG_HASH_SIZE, hash);
     

/****************************************************************************/
/*                                                                          */
/*  Create PCR_INFO structure using current PCR values                      */
/*                                                                          */
/****************************************************************************/
unsigned long
STPM_GenPCRInfo(int pcrmapsize, const unsigned char * pcrmap, unsigned char *pcrinfo,
                unsigned int *len)
{
/*	struct pcrinfo {
		unsigned short selsize;
		unsigned char select[pcrinfolen];
		unsigned char relhash[TCG_HASH_SIZE];
		unsigned char crthash[TCG_HASH_SIZE];
	} myinfo;
*/
	int i, j, k;
	unsigned char work;
	unsigned char *valarray = 0;
	unsigned long numregs;
	unsigned long ret;
	unsigned long valsize;
  unsigned long pcrinfolen = 2 + pcrmapsize + 2 * TCG_HASH_SIZE;
  unsigned char buffer[pcrinfolen]; 
	SHA1_CTX sha;

	/* check arguments */
	if (pcrinfo == NULL || len == NULL || len == 0 || *len < pcrinfolen || pcrmap == 0)
		return -1;

  /* set selection size */
  *(unsigned short *)buffer = htons(pcrmapsize);

	/* build pcr selection array */
	memcpy(buffer + 2, pcrmap, pcrmapsize);

	/* calculate number of PCR registers requested */
	numregs = 0;
	for (j = 0; j < pcrmapsize; j++) {
		work = pcrmap[j];
		for (i = 0; i < 8; ++i) {
			if (work & 1)
				++numregs;
			work = work >> 1;
		}
	}

	if (numregs != 0) { 
		/* create the array of PCR values */
		valarray = (unsigned char *) malloc(TCG_HASH_SIZE * numregs);
		/* read the PCR values into the value array */
    k=0;
		for (j = 0; j < pcrmapsize; j++) { 
			work = pcrmap[j];
			for (i = 0; i < 8; i++, work = work >> 1) {
				if ((work & 1) == 0)
					continue;
				ret = STPM_PcrRead(i + j * 8, &(valarray[(k * TCG_HASH_SIZE)]));
				if (ret)
				{        
					//free memory
					free(valarray);
					return ret;
				}
				k++;
			}
		}
	}

	valsize = htonl(numregs * TCG_HASH_SIZE);
	/* calculate composite hash */
	SHA1_init(&sha);
	SHA1_update(&sha, &buffer[0], 2);
	SHA1_update(&sha, &buffer[2], pcrmapsize);
	SHA1_update(&sha, &valsize, 4);
	for (i = 0; i < numregs; ++i) {
		SHA1_update(&sha, &(valarray[(i * TCG_HASH_SIZE)]),
		            TCG_HASH_SIZE);
	}
	SHA1_final(&sha, &buffer[2 + pcrmapsize]); //myinfo.relhash
  //myinfo.crthash <- myinfo.relhash
	memcpy(&buffer[2 + pcrmapsize + TCG_HASH_SIZE], &buffer[2 + pcrmapsize], TCG_HASH_SIZE); 
	memcpy(pcrinfo, &buffer, pcrinfolen);
	*len = pcrinfolen;

	//free memory
	if (numregs > 0)
		free(valarray);

	return 0;
}
