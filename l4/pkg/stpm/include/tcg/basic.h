/****************************************************************************/
/*                                                                          */
/*  TCPA.H  03 Apr 2003                                                     */
/*                                                                          */
/* This file is copyright 2003 IBM. See "License" for details               */
/*                                                                          */
/* Splitted by Bernhard Kauer <kauer@tudos.org>                             */
/*                                                                          */
/****************************************************************************/
#ifndef TCPA_H
#define TCPA_H

#include "tpm.h"

unsigned long 
TPM_Reset(void);

unsigned long 
TPM_SelfTestFull(void);

unsigned long 
TPM_GetCapability_Version(int *major, 
			  int *minor,
			  int *version, 
			  int *rev);

unsigned long 
TPM_GetCapability_Slots(unsigned long *slots);

unsigned long 
TPM_GetCapability_Pcrs(unsigned long *pcrs);

unsigned long 
TPM_GetCapability_Key_Handle(unsigned short *num, unsigned long keys[]);

#endif
