/****************************************************************************/
/*                                                                          */
/*  TCPA.H  03 Apr 2003                                                     */
/*                                                                          */
/* This file is copyright 2003 IBM. See "License" for details               */
/*                                                                          */
/* Splitted by Bernhard Kauer <kauer@tudos.org>                             */
/*                                                                          */
/****************************************************************************/
#ifndef _SEAL_H
#define _SEAL_H



unsigned long STPM_Seal(unsigned long keyhandle,
		       unsigned char *pcrinfo, unsigned long pcrinfosize,
		       unsigned char *keyauth,
		       unsigned char *dataauth,
		       unsigned char *data, unsigned int datalen,
		       unsigned char *blob, unsigned int *bloblen);
#ifndef TPM_Seal
#define TPM_Seal(...) STPM_Seal(__VA_ARGS__)
#endif

unsigned long STPM_Seal_CurrPCR(unsigned long keyhandle, 
             int pcrmapsize,
             const unsigned char * pcrmap,
			       unsigned char *keyauth,
			       unsigned char *dataauth,
			       unsigned char *data, unsigned int datalen,
			       unsigned char *blob, unsigned int *bloblen);
#ifndef TPM_Seal_CurrPCR
#define TPM_Seal_CurrPCR(...) STPM_Seal_CurrPCR(__VA_ARGS__)
#endif

unsigned long STPM_Unseal(unsigned long keyhandle,
			 unsigned char *keyauth,
			 unsigned char *dataauth,
			 unsigned char *blob, unsigned int bloblen,
			 unsigned char *rawdata, unsigned int *datalen);
#ifndef TPM_Unseal
#define TPM_Unseal(...) STPM_Unseal(__VA_ARGS__)
#endif

#endif
