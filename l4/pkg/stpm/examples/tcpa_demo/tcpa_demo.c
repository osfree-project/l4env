/****************************************************************************/
/*                                                                          */
/*                      TCPA simple Demonstration Program                   */
/*                                                                          */
/*  This file is copyright 2003 IBM. See "License" for details              */
/****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tcg/pcrs.h>
#include <tcg/keys.h>
#include <tcg/basic.h>


int l4libc_heapsize = 64*1024;

#define PRINT_VALUE(var,name) { \
  printf("%20s: %#10x\n",#name,var.name);\
}
#define PRINT_VALUEL(var,name) { \
  printf("%20s: %#10lx\n",#name,var.name);\
}

int main(int argc, char *argv[])
{
	pubkeydata pubek;
	unsigned long slots;
	unsigned long pcrs;
	unsigned short num;
	unsigned long keys[256];
	char pcr_data[20];
	int major, minor, version, rev, i, j;

	printf("started\n");
#if 0
	if (TPM_Reset())
		exit(-1);
	printf("TPM successfully reset\n");
#endif

	if (TPM_GetCapability_Version(&major,&minor,&version,&rev))
		exit(-1);
	printf("TPM version %d.%d.%d.%d\n",major,minor,version,rev);

	if(TPM_GetCapability_Pcrs(&pcrs))
		exit(-1);
	printf("%ld PCR registers are available\n",pcrs);

#if 1
	{
		int ret;
		// extend the last pcr
		strncpy(pcr_data,"12345678901234567890",20);
		ret=TPM_Extend(15,(unsigned char*)pcr_data);
		printf("Extended: %d\n",ret);
	}
#endif

	for(i=0;i<pcrs;i++){
		if(TPM_PcrRead((unsigned long)i,(unsigned char*)pcr_data))
			exit(-1);
		printf("PCR-%02d: ",i);
		for(j=0;j<20;j++)
			printf("%02X ",pcr_data[j]);
		printf("\n");
	}

	if(TPM_GetCapability_Slots(&slots))
		exit(-1);
	printf("%ld Key slots are available\n",slots);

	if(TPM_GetCapability_Key_Handle(&num, keys))
		exit(-1);
	if(num==0)
		printf("No keys are loaded\n");
	else 
		for(i=0;i<num;i++)
			printf("Key Handle %04lX loaded\n",keys[i]);

	if (TPM_ReadPubek(&pubek))
		exit(-1);
	PRINT_VALUEL(pubek,algorithm);
	PRINT_VALUE(pubek,encscheme);
	PRINT_VALUE(pubek,sigscheme);
	PRINT_VALUEL(pubek,keybitlen);
	PRINT_VALUEL(pubek,numprimes);
	PRINT_VALUEL(pubek,expsize);
	{
		int exp=(pubek.exponent[0]<<16)+
			(pubek.exponent[1]<<8)+
			(pubek.exponent[2]<<0);
		printf("%20s: %#10x\n","exponent",exp);
	}
	PRINT_VALUEL(pubek,keylength);
	PRINT_VALUEL(pubek,pcrinfolen);

	return (0);
}
