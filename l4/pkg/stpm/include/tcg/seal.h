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



unsigned long TPM_Seal(unsigned long keyhandle,
		       unsigned char *pcrinfo, unsigned long pcrinfosize,
		       unsigned char *keyauth,
		       unsigned char *dataauth,
		       unsigned char *data, unsigned int datalen,
		       unsigned char *blob, unsigned int *bloblen);

unsigned long TPM_Seal_CurrPCR(unsigned long keyhandle, unsigned long pcrmap,
			       unsigned char *keyauth,
			       unsigned char *dataauth,
			       unsigned char *data, unsigned int datalen,
			       unsigned char *blob, unsigned int *bloblen);

unsigned long TPM_Unseal(unsigned long keyhandle,
			 unsigned char *keyauth,
			 unsigned char *dataauth,
			 unsigned char *blob, unsigned int bloblen,
			 unsigned char *rawdata, unsigned int *datalen);

#endif
