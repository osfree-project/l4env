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

#include <stdint.h>

#define TCG_PCR_NUM       16   /* number of PCR registers supported */
#define TCG_PCR_MASK_SIZE  2   /* size in bytes of PCR bit mask     */

unsigned long 
TPM_PcrRead(unsigned long pcrindex, unsigned char *pcrvalue);

unsigned long 
TPM_Extend(unsigned long pcrindex, unsigned char *hash);

unsigned long 
GenPCRInfo(unsigned long pcrmap, unsigned char *pcrinfo, unsigned int *len);

#endif
