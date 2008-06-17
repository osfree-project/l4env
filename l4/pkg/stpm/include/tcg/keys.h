/****************************************************************************/
/*                                                                          */
/* KEYS.H 08 Apr 2003                                                       */
/*                                                                          */
/* This file is copyright 2003 IBM. See "License" for details               */
/****************************************************************************/
#ifndef KEYS_H
#define KEYS_H

#include <stdint.h>

typedef struct pubkeydata {
    unsigned long algorithm;
    unsigned short encscheme;
    unsigned short sigscheme;
    unsigned long keybitlen;
    unsigned long numprimes;
    unsigned long expsize;
    unsigned char exponent[3];
    unsigned long keylength;
    unsigned char modulus[256];
    unsigned long pcrinfolen;
    unsigned char pcrinfo[256];
} pubkeydata;

typedef struct keydata {
    unsigned char version[4];
    unsigned short keyusage;
    unsigned long keyflags;
    unsigned char authdatausage;
    pubkeydata pub;
    unsigned long privkeylen;
    unsigned char encprivkey[1024];
} keydata;

/**
 * Read public key of EK
 */
unsigned long STPM_ReadPubek(pubkeydata * k);
#ifndef TPM_ReadPubek
#define TPM_ReadPubek(...) STPM_ReadPubek(__VA_ARGS__)
#endif

/**
 * Create keys
 */
unsigned long STPM_CreateWrapKey(unsigned long keyhandle,
				unsigned char *keyauth,
				unsigned char *newauth,
				unsigned char *migauth,
				keydata *keyparms, keydata *key);
#ifndef TPM_CreateWrapKey
#define TPM_CreateWrapKey(...) STPM_CreateWrapKey(__VA_ARGS__)
#endif

/**
 * Reads the public key of a loaded key specified by keyhandle
 * from TPM
 */
unsigned long STPM_GetPubKey(unsigned long   keyhandle,
                            unsigned char * keyauth,
                            pubkeydata    * pubkey);
#ifndef TPM_GetPubKey
#define TPM_GetPubKey(...) STPM_GetPubKey(__VA_ARGS__)
#endif

/**
 * Deprecated in TCGA 1.2 spec
 */
unsigned long STPM_LoadKey(unsigned long keyhandle, unsigned char *keyauth,
			  keydata *keyparms, unsigned long *newhandle);
#ifndef TPM_LoadKey
#define TPM_LoadKey(...) STPM_LoadKey(__VA_ARGS__)
#endif

/**
 * Load a given key into TPM. Replacement of TPM_Loadkey.
 */
unsigned long STPM_LoadKey2(unsigned long keyhandle, unsigned char *keyauth,
 			   keydata *keyparms, unsigned long *newhandle);
#ifndef TPM_LoadKey2
#define TPM_LoadKey2(...) STPM_LoadKey2(__VA_ARGS__)
#endif

/**
 * Deprecated in TCGA 1.2 spec
 */
unsigned long STPM_EvictKey(unsigned long keyhandle);
#ifndef TPM_EvictKey
#define TPM_EvictKey(...) STPM_EvictKey(__VA_ARGS__)
#endif

/**
 * Replacement of TPM_EvictKey in TCGA 1.2 spec
 */
unsigned long STPM_FlushSpecific(unsigned long keyhandle, unsigned long type);
#ifndef TPM_FlushSpecific
#define TPM_FlushSpecific(...) STPM_FlushSpecific(__VA_ARGS__)
#endif

int KeyExtract(unsigned char *keybuff, keydata *k);
int PubKeyExtract(unsigned char *pkeybuff, pubkeydata *k, int pcrpresent);
int BuildKey(unsigned char *buffer, keydata *k);
int KeySize(unsigned char *keybuff);
int PubKeySize(unsigned char *keybuff, int pcrpresent);

#endif
