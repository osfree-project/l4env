/****************************************************************************/
/*                                                                          */
/*  TCPA.H  03 Apr 2003                                                     */
/*                                                                          */
/* This file is copyright 2003 IBM. See "License" for details               */
/*                                                                          */
/* Splitted by Bernhard Kauer <kauer@tudos.org>                             */
/*                                                                          */
/****************************************************************************/


#ifndef _OWNER_H
#define _OWNER_H

#include "keys.h"

unsigned long
STPM_OwnerClear(unsigned char *ownerauth);
#ifndef TPM_OwnerClear
#define TPM_OwnerClear(...) STPM_OwnerClear(__VA_ARGS__)
#endif

uint32_t 
STPM_TakeOwnership(unsigned char *ownpass, unsigned char *srkpass, keydata *key);
#ifndef TPM_TakeOwnership
#define TPM_TakeOwnership(...) STPM_TakeOwnership(__VA_ARGS__)
#endif

#endif
