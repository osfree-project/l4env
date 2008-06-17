/****************************************************************************/
/*                                                                          */
/*  PCRS.H  03 Apr 2003                                                     */
/*                                                                          */
/* This file is copyright 2003 IBM. See "License" for details               */
/*                                                                          */
/* Extended by Bernhard Kauer <kauer@tudos.org>                             */
/*                                                                          */
/****************************************************************************/
#ifndef PCRS_H
#define PCRS_H

unsigned long 
STPM_PcrRead(unsigned long pcrindex, unsigned char *pcrvalue);
#ifndef TPM_PcrRead
#define TPM_PcrRead(...) STPM_PcrRead(__VA_ARGS__)
#endif

unsigned long 
STPM_Extend(unsigned long pcrindex, unsigned char *hash);
#ifndef TPM_Extend
#define TPM_Extend(...) STPM_Extend(__VA_ARGS__)
#endif

unsigned long 
STPM_GenPCRInfo(int pcrmapsize, const unsigned char * pcrmap,
                unsigned char *pcrinfo, unsigned int *len);

#endif
