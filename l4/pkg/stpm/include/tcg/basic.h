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
STPM_Reset(void);

unsigned long 
STPM_SelfTestFull(void);
#ifndef TPM_SelfTestFull
#define TPM_SelfTestFull(...) STPM_SelfTestFull(__VA_ARGS__)
#endif

unsigned long 
STPM_GetCapability_Version(int *major, 
			  int *minor,
			  int *version, 
			  int *rev);
#ifndef TPM_GetCapability_Version
#define TPM_GetCapability_Version(...) STPM_GetCapability_Version(__VA_ARGS__)
#endif

/**
 * Get the number of the available key slots.
 */			  			  
unsigned long 
STPM_GetCapability_Slots(unsigned long *slots);
#ifndef TPM_GetCapability_Slots
#define TPM_GetCapability_Slots(...) STPM_GetCapability_Slots(__VA_ARGS__)
#endif

/**
 * Get the number of the pcrs.
 */			  			  
unsigned long 
STPM_GetCapability_Pcrs(unsigned long *pcrs);
#ifndef TPM_GetCapability_Pcrs
#define TPM_GetCapability_Pcrs(...) STPM_GetCapability_Pcrs(__VA_ARGS__)
#endif

unsigned long 
STPM_GetCapability_Key_Handle(unsigned short *num, unsigned long keys[]);
#ifndef TPM_GetCapability_Key_Handle
#define TPM_GetCapability_Key_Handle(...) STPM_GetCapability_Key_Handle(__VA_ARGS__)
#endif

/**
 * Get the timeout values of 4 function classes defined in TPM specification
 */
unsigned long
STPM_GetCapability_Timeouts(unsigned long *timeout_a, unsigned long *timeout_b,
                            unsigned long *timeout_c, unsigned long *timeout_d);
#ifndef TPM_GetCapability_Timeouts
#define TPM_GetCapability_Timeouts(...) STPM_GetCapability_Timeouts(__VA_ARGS__)
#endif

#endif
