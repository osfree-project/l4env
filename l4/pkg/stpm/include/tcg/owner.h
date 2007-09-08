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
TPM_OwnerClear(unsigned char *ownerauth);

uint32_t 
TPM_TakeOwnership(unsigned char *ownpass, unsigned char *srkpass, keydata *key);

#endif
