/*
 * \brief   Quote functions.
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/* 
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include <tcg/quote.h>
#include <tcg/oiaposap.h>

/**
 * Return all PCR values and a signature for it with a given key 
 */
TPM_TRANSMIT_OSAP_FUNC(Quote,
		       (unsigned long keyhandle,
			unsigned char *keyauth,
			unsigned char *pcrselect,
			unsigned char *nounce,
			unsigned char *pcrcomposite,
			unsigned char *blob, 
			unsigned int *bloblen),
		       keyauth,
		       0x0001,
		       keyhandle,
		       unsigned pcrselectsize;
		       if (pcrselect == NULL || blob == NULL || bloblen == NULL)
			 return -1;
		       ,
		       pcrselectsize = ntohs(((struct tpm_pcr_selection2 *)pcrselect)->size)+sizeof(unsigned short);
		       ,
		       {
			 int pcrcompositesize = pcrselectsize + 4 + TPM_EXTRACT_LONG(pcrselectsize);
			 *bloblen = TPM_EXTRACT_LONG(pcrcompositesize);
			 TPM_COPY_FROM(pcrcomposite, 0, pcrcompositesize);

			 TPM_COPY_FROM(blob, pcrcompositesize+4, *bloblen);
		       },
		       "I % %",
		       "L % %",
		       keyhandle,
		       TCG_HASH_SIZE, nounce,
		       pcrselectsize, pcrselect);
  


#include <tcg/basic.h>
#include <stdlib.h>

/**
 * XXX This is a test function for quote...
 */
int quote_stdout(int argc, unsigned char *argv[])
{
	int major, minor, version, rev, i, j;

	if (argc<4){
		printf("Usage: %s keyhandle keypassword nounce\n",argv[0]?(char *)argv[0]:__func__);
		return(-1);
	}

	if (STPM_GetCapability_Version(&major,&minor,&version,&rev)){
		printf("TPM version failed\n");	
		return(-3);
	} else
		printf("TPM version: %d.%d.%d.%d\n",major,minor,version,rev);

	//1.2 TPMs don't like resets	
	if (!(major == 1 && minor == 1 && version == 0 && rev == 0))
	{
		if (STPM_Reset()) {
			printf("TPM reset failed\n");	
		return(-2);
  } else
		printf("TPM successfully reset\n");
	}

	{
		struct tpm_pcr_selection2 pcrselect;
		unsigned char output[1024];
		unsigned char passhash[20];
		unsigned char nouncehash[20];
		struct tpm_pcr_composite pcrcomposite;
		unsigned int outputlen;
		unsigned int keyhandle;
		int res;

		pcrselect.size=0x0200;
		pcrselect.select=0xFFFF;
      
		keyhandle=strtol((char *)argv[1],NULL,16);
		// fill the variables to find bugs
		for (i=0; i<20; i++)
			passhash[i]=i;
		for (i=0; i<20; i++)
			nouncehash[i]=i;

		sha1(argv[2], strlen((char *)argv[2]), passhash);
		sha1(argv[3], strlen((char *)argv[3]), nouncehash);

		printf("keyhandle: %#x\n",keyhandle);
		res=STPM_Quote(keyhandle, passhash, 
			      (unsigned char *) &pcrselect, 
			      nouncehash, 
			      (unsigned char *) &pcrcomposite,
			      (unsigned char *) &output, &outputlen);
		printf("TPM_Quote() = %d\n",res);

		if (res)
			return(-4);

		printf("pcrcomposite (%d pcrs):\n",ntohl(pcrcomposite.valuesize)/20);
      
		for(i=0;i<ntohl(pcrcomposite.valuesize)/20;i++){
			printf("%8s%02d: ","PCR-",i);
			for(j=0;j<20;j++)
				printf("%02x",pcrcomposite.value[i].value[j]);
			printf("\n");
		}

		printf("signature (%d Bytes):\n",outputlen);
		for(i=0;i<(outputlen>>4);i++){
			printf("    ");
			for(j=0;j<16;j++)
				printf("%02x",output[(i<<4)+j]);      
			printf("\n");
		}
	}
	printf("signature end\n");
	return (0);
}
