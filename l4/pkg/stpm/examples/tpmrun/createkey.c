/*
 * Copyright (C) 2007 Alexander Boettcher <boettcher@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#include "tpmrun.h"

// valid values for TCPA_KEY_USAGE
#define TPM_KEY_SIGNING    0x0010
#define TPM_KEY_STORAGE    0x0011
#define TPM_KEY_IDENTITY   0x0012
#define TPM_KEY_AUTHCHANGE 0x0013
#define TPM_KEY_BIND       0x0014
#define TPM_KEY_LEGACY     0x0015
// valid values for TCP_KEY_FLAGS
// valid values for TCPA_AUTH_DATA_USAGE
#define TPM_AUTH_NEVER     0x00 //disallowed in TCGA 1.2 spec
#define TPM_AUTH_ALWAYS    0x01
// valid values for TCPA_ALGORITHM_ID
#define TCPA_ALG_RSA       0x00000001UL
// valid values for TCPA_ENC_SCHEME
#define TCPA_ES_NONE                0x0001
#define TCPA_ES_RSAESPKCSv15        0x0002
#define TCPA_ES_RSAESOAEP_SHA1_MGF1 0x0003
// valid values for TCPA_SIG_SCHEME
#define TCPA_SS_NONE                0x0001
#define TCPA_SS_RSASSAPKCS1v15_SHA1 0x0002
#define TCPA_SS_RSASSAPKCS1v15_DER  0x0003
// Magic key handle
#define SRK_KEYHANDLE      0x40000000

int createKey(keydata * key, unsigned char * keyauth, unsigned char * newauth)
{
  unsigned long res;
  unsigned long keyhandle = SRK_KEYHANDLE;
  unsigned char * migauth = NULL; // non migratable
  keydata keyparms;

  memset(&keyparms, 0, sizeof(keyparms));

  // Version 1.1 compliant keyparms
  keyparms.version[0] = 1; // major 1.1
  keyparms.version[1] = 1; // minor 1.1
  keyparms.version[2] = 0; 
  keyparms.version[3] = 0;
  // The key will be used for ...
  keyparms.keyusage = TPM_KEY_SIGNING;
  // TODO, what this good for ?
  keyparms.keyflags = 0; // TODO ?
  // The auth data for the new key is required ...
  keyparms.authdatausage = TPM_AUTH_ALWAYS;
  // Info about what kind of key is required ...
  keyparms.pub.algorithm = TCPA_ALG_RSA;
  keyparms.pub.encscheme = TCPA_ES_NONE;
  keyparms.pub.sigscheme = TCPA_SS_RSASSAPKCS1v15_SHA1;
  keyparms.pub.keybitlen = RSA_MODULUS_BIT_SIZE;
  keyparms.pub.numprimes = 2;
  keyparms.pub.expsize = 0;        /* defaults to 0x010001 */
  keyparms.pub.keylength = 0;      /* not used here */
  keyparms.pub.pcrinfolen = 0;     /* not used here */
  // TODO, should be ok !?
  keyparms.privkeylen = 0; 
  //keyparms.privkey[1024];

  res = TPM_CreateWrapKey(keyhandle, keyauth, newauth, migauth,
                          &keyparms, key);

  return res;
}
