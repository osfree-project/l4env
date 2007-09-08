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
unsigned long GenPCRInfo(unsigned long pcrmap, unsigned char *pcrinfo,
			 unsigned int *len)
{
	struct pcrinfo {
		unsigned short selsize;
		unsigned char select[TCG_PCR_MASK_SIZE];
		unsigned char relhash[TCG_HASH_SIZE];
		unsigned char crthash[TCG_HASH_SIZE];
	} myinfo;
	int i;
	int j;
	unsigned long work;
	unsigned char *valarray;
	unsigned long numregs;
	unsigned long ret;
	unsigned long valsize;
	SHA1_CTX sha;

	/* check arguments */
	if (pcrinfo == NULL || len == NULL)
		return -1;
	/* build pcr selection array */
	work = pcrmap;
	memset(myinfo.select, 0, TCG_PCR_MASK_SIZE);
	for (i = 0; i < TCG_PCR_MASK_SIZE; ++i) {
		myinfo.select[i] = work & 0x000000FF;
		work = work >> 8;
	}
	/* calculate number of PCR registers requested */
	numregs = 0;
	work = pcrmap;
	for (i = 0; i < (TCG_PCR_MASK_SIZE * 8); ++i) {
		if (work & 1)
			++numregs;
		work = work >> 1;
	}
	if (numregs == 0) {
		*len = 0;
		return 0;
	}
	/* create the array of PCR values */
	valarray = (unsigned char *) malloc(TCG_HASH_SIZE * numregs);
	/* read the PCR values into the value array */
	work = pcrmap;
	j = 0;
	for (i = 0; i < (TCG_PCR_MASK_SIZE * 8); ++i, work = work >> 1) {
		if ((work & 1) == 0)
			continue;
		ret = TPM_PcrRead(i, &(valarray[(j * TCG_HASH_SIZE)]));
		if (ret)
			return ret;
		++j;
	}
	myinfo.selsize = ntohs(TCG_PCR_MASK_SIZE);
	valsize = ntohl(numregs * TCG_HASH_SIZE);
	/* calculate composite hash */
	SHA1_init(&sha);
	SHA1_update(&sha, &myinfo.selsize, 2);
	SHA1_update(&sha, &myinfo.select, TCG_PCR_MASK_SIZE);
	SHA1_update(&sha, &valsize, 4);
	for (i = 0; i < numregs; ++i) {
		SHA1_update(&sha, &(valarray[(i * TCG_HASH_SIZE)]),
			    TCG_HASH_SIZE);
	}
	SHA1_final(&sha,myinfo.relhash);
	memcpy(myinfo.crthash, myinfo.relhash, TCG_HASH_SIZE);
	memcpy(pcrinfo, &myinfo, sizeof(struct pcrinfo));
	*len = sizeof(struct pcrinfo);
	return 0;
}
