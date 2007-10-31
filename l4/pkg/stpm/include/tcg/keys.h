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


unsigned long TPM_ReadPubek(pubkeydata * k);
unsigned long TPM_CreateWrapKey(unsigned long keyhandle,
				unsigned char *keyauth,
				unsigned char *newauth,
				unsigned char *migauth,
				keydata *keyparms, keydata *key);
/**
 * Deprecated in TCGA 1.2 spec
 */
unsigned long TPM_LoadKey(unsigned long keyhandle, unsigned char *keyauth,
			  keydata *keyparms, unsigned long *newhandle);
/**
 * Load a given key into TPM. Replacement of TPM_Loadkey.
 */
unsigned long TPM_LoadKey2(unsigned long keyhandle, unsigned char *keyauth,
 			   keydata *keyparms, unsigned long *newhandle);
/**
 * Deprecated in TCGA 1.2 spec
 */
unsigned long TPM_EvictKey(unsigned long keyhandle);

/**
 * Replacement of TPM_EvictKey in TCGA 1.2 spec
 */
unsigned long TPM_FlushSpecific(unsigned long keyhandle, unsigned long type);

int KeyExtract(unsigned char *keybuff, keydata *k);
int PubKeyExtract(unsigned char *pkeybuff, pubkeydata *k, int pcrpresent);
int BuildKey(unsigned char *buffer, keydata *k);
int KeySize(unsigned char *keybuff);
int PubKeySize(unsigned char *keybuff, int pcrpresent);

#endif
