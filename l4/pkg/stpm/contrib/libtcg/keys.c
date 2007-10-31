/****************************************************************************/
/*                                                                          */
/*                         TCG Key Handling Routines                        */
/*                                                                          */
/* This file is copyright 2003 IBM. See "License" for details               */
/*                                                                          */
/* Beautified by Bernhard Kauer <kauer@tudos.org>                           */
/*                                                                          */
/****************************************************************************/
#include <tcg/keys.h>
#include <tcg/oiaposap.h>

/**
 * Read the public part of the EndorsementKey
 */
TPM_TRANSMIT_FUNC(ReadPubek,
		  (pubkeydata * k),
		  unsigned char nonce[TCG_HASH_SIZE];
		  if (k == NULL)
		  return -2;
		  if (rand_buffer(nonce, TCG_NONCE_SIZE)<1)
		  return -3;
		  ,
		  PubKeyExtract(buffer + TCG_DATA_OFFSET, k, 0);,
		  "%",		      
		  TCG_HASH_SIZE, 
		  nonce);

/****************************************************************************/
/*                                                                          */
/* Create and Wrap a Key                                                    */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* keyhandle is the TCG_KEY_HANDLE of the parent key of the new key        */
/*           0x40000000 for the SRK                                         */
/* keyauth   is the authorization data (password) for the parent key        */
/* newauth   is the authorization data (password) for the new key           */
/* migauth   is the authorization data (password) for migration of the new  */
/*           key, or NULL if the new key is not migratable                  */
/*           all authorization values must be 20 bytes long                 */
/* keyparms  is a pointer to a keydata structure with parms set for the new */
/*           key                                                            */
/* key       is a pointer to a keydata structure returned filled in         */
/*           with the public key data for the new key                       */
/*                                                                          */
/****************************************************************************/
TPM_TRANSMIT_OSAP_FUNC(CreateWrapKey,
		       (unsigned long keyhandle,
			unsigned char *keyauth,
			unsigned char *newauth,
			unsigned char *migauth,
			keydata *keyparms,
			keydata *key),
		       keyauth,
		       0x0001,
		       keyhandle,
		       unsigned char encauth1[TCG_HASH_SIZE];
		       unsigned char encauth2[TCG_HASH_SIZE];
		       unsigned char kparmbuf[TCG_MAX_BUFF_SIZE];
		       int kparmbufsize;

		       if (keyauth == NULL 
			   || newauth == NULL 
			   || keyparms == NULL
			   || key == NULL)
			 return -1;

		       if (0 > (kparmbufsize = BuildKey(kparmbuf, keyparms)))
			 return -2;
		       ,
		       CALC_ENC_AUTH(encauth1, newauth);
		       CALC_ENC_AUTH(encauth2, migauth);
		       ,
		       KeyExtract(buffer + TCG_DATA_OFFSET, key);
		       ,
		       "I % % %",
		       "L % % %",
		       keyhandle,
		       TCG_HASH_SIZE, encauth1, 
		       TCG_HASH_SIZE, encauth2,
		       kparmbufsize, kparmbuf);


/**
 * Load a key into a TPM.
 * @param keyhandle the handle of the parent key.
 * @param keyauth   the auth for the parent key.
 * @param keyparams the key created via CreateWrapKey
 * @param newhandle the handle of the loaded key
 */
TPM_TRANSMIT_OIAP_FUNC(LoadKey,
		       (unsigned long keyhandle,
			unsigned char *keyauth,
			keydata *keyparms,
			unsigned long *newhandle),
		       keyauth,

		       unsigned char kparmbuf[TCG_MAX_BUFF_SIZE];
		       int kparmbufsize;

		       if (keyauth == NULL || keyparms == NULL || newhandle == NULL)
			 return -1;

		       kparmbufsize = BuildKey(kparmbuf, keyparms);
		       if (0 > (kparmbufsize = BuildKey(kparmbuf, keyparms)))
			 return -2;

		       ,
		       *newhandle = TPM_EXTRACT_LONG(0);
		       ,
                       0,
		       "I %",
		       "L %",
		       keyhandle,
		       kparmbufsize,
		       kparmbuf);


/**
 * Load a key into a TPM.
 * @param keyhandle the handle of the parent key.
 * @param keyauth   the auth for the parent key.
 * @param keyparams the key created via CreateWrapKey
 * @param newhandle the handle of the loaded key
 */
TPM_TRANSMIT_OIAP_FUNC(LoadKey2,
                      (unsigned long keyhandle,
                       unsigned char *keyauth,
                       keydata *keyparms,
                       unsigned long *newhandle),
                      keyauth,

                      unsigned char kparmbuf[TCG_MAX_BUFF_SIZE];
                      int kparmbufsize;

                      if (keyauth == NULL || keyparms == NULL || newhandle == NULL)
                        return -1;

                      kparmbufsize = BuildKey(kparmbuf, keyparms);
                      if (0 > (kparmbufsize = BuildKey(kparmbuf, keyparms)))
                        return -2;

                      ,
                      *newhandle = TPM_EXTRACT_LONG(0);
                      ,
                      4,
                      "I %",
                      "L %",
                      keyhandle,
                      kparmbufsize,
                      kparmbuf);


/**
 * Delete a key from the tpm.
 * Deprecated since TCGA 1.2 spec.
 */
TPM_TRANSMIT_FUNC(EvictKey,
		      (unsigned long keyhandle),
		      ,
		      ,
		      "L",
		      keyhandle);

/**
 * Delete various handles, for example keys.
 * Replacement of EvictKey
 */
TPM_TRANSMIT_FUNC(FlushSpecific,
                     (unsigned long keyhandle,
                      unsigned long restype),
                     ,
                     ,
                     "L L",
                     keyhandle,
                     restype);

/****************************************************************************/
/*                                                                          */
/* Create a TCG_KEY buffer from a keydata structure                        */
/*                                                                          */
/****************************************************************************/
int BuildKey(unsigned char *buffer, keydata * k)
{
    char build_key_fmt[] = "% S L o L S S L L L L @ @ @";
    int ret;

    PRINT_HASH_SIZE(((unsigned char *) k), sizeof(*k));
    ret = buildbuff(build_key_fmt, buffer,
                    4, k->version,
                    k->keyusage,
                    k->keyflags,
                    k->authdatausage,
                    k->pub.algorithm,
                    k->pub.encscheme,
                    k->pub.sigscheme,
                    12,
                    k->pub.keybitlen,
                    k->pub.numprimes,
                    0,
                    k->pub.pcrinfolen, k->pub.pcrinfo,
                    k->pub.keylength, k->pub.modulus,
                    k->privkeylen, k->encprivkey);
    return ret;
}


/****************************************************************************/
/*                                                                          */
/* Walk down the TCG_Key Structure extracting information                  */
/*                                                                          */
/****************************************************************************/
int KeyExtract(unsigned char *keybuff, keydata * k)
{
    int offset;
    int pubkeylen;

    /* fill in  keydata structure */
    offset = 0;
    memcpy(k->version, keybuff + offset, sizeof(k->version));
    offset += 4;
    k->keyusage = ntohs(*(unsigned short *) (keybuff + offset));
    offset += 2;
    k->keyflags = ntohl(*(unsigned long *) (keybuff + offset));
    offset += 4;
    k->authdatausage = keybuff[offset];
    offset += 1;
    pubkeylen = PubKeyExtract(keybuff + offset, &(k->pub), 1);
    offset += pubkeylen;
    k->privkeylen = ntohl(*(unsigned long *) (keybuff + offset));
    offset += 4;
    if (k->privkeylen > 0 && k->privkeylen <= 1024)
        memcpy(k->encprivkey, keybuff + offset, k->privkeylen);
    offset += k->privkeylen;
    return offset;
}

/****************************************************************************/
/*                                                                          */
/* Walk down the TCG_PUBKey Structure extracting information               */
/*                                                                          */
/****************************************************************************/
int PubKeyExtract(unsigned char *keybuff, pubkeydata * k, int pcrpresent)
{
    unsigned long parmsize;
    unsigned long pcrisize;
    int offset;

    offset = 0;
    k->algorithm = ntohl(*(unsigned long *) (keybuff + offset));
    offset += 4;
    k->encscheme = ntohs(*(unsigned short *) (keybuff + offset));
    offset += 2;
    k->sigscheme = ntohs(*(unsigned short *) (keybuff + offset));
    offset += 2;
    parmsize = ntohl(*(unsigned long *) (keybuff + offset));
    offset += 4;
    if (k->algorithm == 0x00000001 && parmsize > 0) {   /* RSA */
        k->keybitlen = ntohl(*(unsigned long *) (keybuff + offset));
        offset += 4;
        k->numprimes = ntohl(*(unsigned long *) (keybuff + offset));
        offset += 4;
        k->expsize = ntohl(*(unsigned long *) (keybuff + offset));
        offset += 4;
    } else {
        offset += parmsize;
    }
    if (k->expsize != 0)
        offset += k->expsize;
    else {
        k->exponent[0] = 0x01;
        k->exponent[1] = 0x00;
        k->exponent[2] = 0x01;
        k->expsize = 3;
    }
    if (pcrpresent) {
        pcrisize = ntohl(*(unsigned long *) (keybuff + offset));
        offset += 4;
        if (pcrisize > 0 && pcrisize <= 256)
            memcpy(k->pcrinfo, keybuff + offset, pcrisize);
        offset += pcrisize;
        k->pcrinfolen = pcrisize;
    }
    else{
      k->pcrinfolen = 0;
    }
    k->keylength = ntohl(*(unsigned long *) (keybuff + offset));
    offset += 4;
    if (k->keylength > 0 && k->keylength <= 256)
        memcpy(k->modulus, keybuff + offset, k->keylength);
    offset += k->keylength;
    return offset;
}

/****************************************************************************/
/*                                                                          */
/* Get the size of a TCG_KEY                                               */
/*                                                                          */
/****************************************************************************/
int KeySize(unsigned char *keybuff)
{
    int offset;
    int privkeylen;

    offset = 0 + 4 + 2 + 4 + 1;
    offset += PubKeySize(keybuff + offset, 1);
    privkeylen = ntohl(*(unsigned long *) (keybuff + offset));
    offset += 4 + privkeylen;
    return offset;
}

/****************************************************************************/
/*                                                                          */
/* Get the size of a TCG_PUBKEY                                            */
/*                                                                          */
/****************************************************************************/
int PubKeySize(unsigned char *keybuff, int pcrpresent)
{
    unsigned long parmsize;
    unsigned long pcrisize;
    unsigned long keylength;

    int offset;

    offset = 0 + 4 + 2 + 2;
    parmsize = ntohl(*(unsigned long *) (keybuff + offset));
    offset += 4;
    offset += parmsize;
    if (pcrpresent) {
        pcrisize = ntohl(*(unsigned long *) (keybuff + offset));
        offset += 4;
        offset += pcrisize;
    }
    keylength = ntohl(*(unsigned long *) (keybuff + offset));
    offset += 4;
    offset += keylength;
    return offset;
}
