/*
 * Copyright (C) 2007 Alexander Boettcher <boettcher@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef __tmprun_h
#define __tpmrun_h

#include <tcg/keys.h>
#include <tcg/oiaposap.h> //sha1 
#include <tcg/basic.h>    //TPM_GetCapability_Key_Handle

int createKey(keydata * key, unsigned char * keyauth, unsigned char * newauth);

int quotePCRs(unsigned int keyhandle,
              unsigned char * passhash,
              unsigned char * nouncehash,
              unsigned char * output,
              unsigned int  * outputlen,
              unsigned char * pcrs,
              unsigned int  pcrlen,
              unsigned int maxpcrs);

void dumpkey(keydata * key);
void redumpkey(const char * dumped, keydata * _key);

int loadkeyfile(const char * servername, const char * filename, keydata * key);

// TPM_RESOURCE_TYPE
#define TPM_RT_KEY 0x00000001
#endif
