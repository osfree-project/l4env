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
              unsigned char * noncehash,
              unsigned char * output,
              unsigned int  * outputlen,
              unsigned char * pcrs,
              unsigned int  pcrlen,
              unsigned int maxpcrs);

int quote_vTPM(unsigned int    keyhandle,
               unsigned char * passhash,
               unsigned char * noncehash,
               unsigned char * output,
               unsigned int  * outputlen,
               unsigned char * pcrcomposite,
               unsigned int  pcrlen,
               unsigned int maxPCRs,
               unsigned char * sTPM,
               unsigned char * vTPM);

int seal_TPM(int keyhandle, unsigned char * keyhash, unsigned char * authhash,
             int maxPCRs,
             unsigned int * outdatalen, unsigned char * outdata,
             unsigned int indatalen, unsigned char * indata);

int unseal_TPM(int keyhandle, unsigned char *keyhash,
               unsigned char *authhash, int maxPCRs,
               unsigned int sealedbloblen, unsigned char * sealedblob);

void dumpkey(keydata * key);
void redumpkey(const char * dumped, keydata * _key);

int loadkeyfile(const char * servername, const char * filename, keydata * key);

// TPM_RESOURCE_TYPE
#define TPM_RT_KEY 0x00000001
#endif
