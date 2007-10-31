/****************************************************************************/
/*                                                                          */
/*                       TPM_Ownership routines                             */
/*                                                                          */
/* This file is copyright 2003 IBM. See "License" for details               */
/*                                                                          */
/* Beautified by Bernhard Kauer <kauer@tudos.org>                           */
/*                                                                          */
/****************************************************************************/

#include <stdlib.h>
#include <tcg/ord.h>
#include <tcg/oiaposap.h>

TPM_TRANSMIT_OSAP_FUNC(OwnerClear,
		       (unsigned char *auth),
		       auth, 0x0002, 0,
		       ,
		       ,
		       ,
		       "",
		       "");



#include <tcg/keys.h>
#ifdef USE_OPENSSL
#include <openssl/rsa.h>
#include <openssl/bn.h>
#include <openssl/crypto.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#else
#include "rsaglue.h"
#include <l4/crypto/rsaref2/rsa.h>
#include <l4/crypto/oaep.h>
#define RSA_MODULUS_SIZE MAX_RSA_MODULUS_LEN
#endif


#ifdef USE_OPENSSL
/****************************************************************************/
/*                                                                          */
/* Convert a TCPA public key to an OpenSSL RSA public key                   */
/*                                                                          */
/****************************************************************************/
RSA *convpubkey(pubkeydata * k)
{
    RSA *rsa;
    BIGNUM *mod;
    BIGNUM *exp;

    /* create the necessary structures */
    rsa = RSA_new();
    mod = BN_new();
    exp = BN_new();
    if (rsa == NULL || mod == NULL || exp == NULL)
        return NULL;
    /* convert the raw public key values to BIGNUMS */
    BN_bin2bn(k->modulus, k->keylength, mod);
    BN_bin2bn(k->exponent, k->expsize, exp);
    /* set up the RSA public key structure */
    rsa->n = mod;
    rsa->e = exp;
    return rsa;
}
#else
/****************************************************************************/
/*                                                                          */
/* Convert a TCPA public key to an RSAREF2 key                              */
/*                                                                          */
/****************************************************************************/
pubkey_t *convpubkey(pubkeydata *k)
{
    pubkey_t *pk = malloc(sizeof(*pk));

    if (pk == NULL)
        return NULL;

    pk->bits = k->keybitlen;
    memcpy(pk->modulus, k->modulus, k->keylength);
    memset(pk->exponent, 0, sizeof(pk->exponent));
    memcpy(pk->exponent + sizeof(pk->exponent) - k->expsize,
           k->exponent, k->expsize);

    return pk;
}
#endif

/****************************************************************************/
/*                                                                          */
/*  Take Ownership of the TPM                                               */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* ownpass   is the authorization data (password) for the new owner         */
/* srkpass   is the authorization data (password) for the new root key      */
/*           both authorization values MUST be 20 bytes long                */
/* key       a pointer to a keydata structure to receive the SRK public key */
/*           or NULL if this information is not required                    */
/*                                                                          */
/****************************************************************************/
uint32_t TPM_TakeOwnership(unsigned char *ownpass, unsigned char *srkpass,
                           keydata *key)
{
    char take_owner_fmt[] = "00 c2 T l s @ @ % L % 00 %";
    char tcpa_oaep_pad_str[] = { 'T', 'C', 'P', 'A' };
    //unsigned char tcpa_oaep_pad_str[] = { 'T', 'C', 'G', 0};
    unsigned char tcpadata[TCG_MAX_BUFF_SIZE]; /* request/response buffer */
    pubkeydata tcpapubkey;      /* public endorsement key data */
    uint32_t srkparamsize;      /* SRK parameter buffer size */
    oiapsess sess;
#ifdef USE_OPENSSL
    RSA *pubkey;                /* PubEK converted to OpenSSL format */
#else
    pubkey_t *pubkey;           /* public key structure for RSAREF2 impl. */
#endif
    unsigned char padded[RSA_MODULUS_BYTE_SIZE];
    keydata srk;                /* key info for SRK */
    uint32_t ret;
    uint32_t command;           /* command ordinal */
    uint16_t protocol;          /* protocol ID */
    uint32_t oencdatasize;      /* owner auth data encrypted size */
    unsigned char ownerencr[RSA_MODULUS_BYTE_SIZE];
    uint32_t sencdatasize;      /* srk auth data encrypted size */
    unsigned char srkencr[RSA_MODULUS_BYTE_SIZE];
    unsigned char srk_param_buff[TCG_SRK_PARAM_BUFF_SIZE];
    unsigned char authdata[TCG_HASH_SIZE];     /* auth data */
    unsigned int rsa_output_len;

    /* check that parameters are valid */
    if (ownpass == NULL || srkpass == NULL)
        return -10;
    /* set up command and protocol values for TakeOwnership function */
    command = htonl(0x0d);
    protocol = htons(0x05);
    /* get the TCPA Endorsement Public Key */
    ret = TPM_ReadPubek(&tcpapubkey);
    if (ret)
        return -11;
    /* convert the public key to OpenSSL format */
    pubkey = convpubkey(&tcpapubkey);
    if (pubkey == NULL)
        return -12;
    memset(ownerencr, 0, sizeof ownerencr);
    memset(srkencr, 0, sizeof srkencr);

#ifdef USE_OPENSSL
    /* Pad and then encrypt the owner data using the RSA public key */
    ret = RSA_padding_add_PKCS1_OAEP(padded, RSA_MODULUS_BYTE_SIZE,
                                     ownpass, TCG_HASH_SIZE,
                                     tcpa_oaep_pad_str,
                                     sizeof tcpa_oaep_pad_str);
    if (ret == 0)
        return -13;
    
    ret = RSA_public_encrypt(RSA_MODULUS_BYTE_SIZE, padded, ownerencr,
                             pubkey, RSA_NO_PADDING);
    if (ret < 0)
        return -14;
    oencdatasize = htonl(ret);
    /* Pad and then encrypt the SRK data using the RSA public key */
    ret = RSA_padding_add_PKCS1_OAEP(padded, RSA_MODULUS_BYTE_SIZE,
                                     srkpass, TCG_HASH_SIZE,
                                     tcpa_oaep_pad_str,
                                     sizeof tcpa_oaep_pad_str);
    if (ret == 0)
        return -15;
    ret = RSA_public_encrypt(RSA_MODULUS_BYTE_SIZE, padded, srkencr, pubkey,
                           RSA_NO_PADDING);
    if (ret < 0)
        return -16;
    sencdatasize = htonl(ret);
    RSA_free(pubkey);


#else
    /* pad and encrypt the owner auth data */
    ret = pad_oaep((char *)ownpass, TCG_HASH_SIZE,
                   tcpa_oaep_pad_str, sizeof(tcpa_oaep_pad_str),
                   RSA_MODULUS_BYTE_SIZE, (char *)padded);
    if (ret != 0)
        return -13;

    ret = RSAPublicEncryptRaw(ownerencr, &rsa_output_len, padded,
                              RSA_MODULUS_BYTE_SIZE, pubkey);
    if (ret != 0)
        return -14;
    oencdatasize = htonl(rsa_output_len);

    /* pad and encrypt the srk auth data */
    ret = pad_oaep((char *)srkpass, TCG_HASH_SIZE,
                   tcpa_oaep_pad_str, sizeof(tcpa_oaep_pad_str),
                   RSA_MODULUS_BYTE_SIZE, (char *)padded);
    if (ret != 0)
        return -15;

    ret = RSAPublicEncryptRaw(srkencr, &rsa_output_len, padded,
                              RSA_MODULUS_BYTE_SIZE, pubkey);
    if (ret != 0)
        return -16;
    sencdatasize = htonl(rsa_output_len);
    
    free(pubkey);
#endif

    if (ntohl(oencdatasize) < 0)
        return -17;
    if (ntohl(sencdatasize) < 0)
        return -18;
    /* fill the SRK-params key structure */
    srk.version[0] = 1;         /* version 1.1.0.0 */
    srk.version[1] = 1;
    srk.version[2] = 0;
    srk.version[3] = 0;
    srk.keyusage = 0x0011;      /* Storage Key */
    srk.keyflags = 0;
    srk.authdatausage = 0x01;   /* Key usage must be authorized */
    srk.privkeylen = 0;         /* private key not specified here */
    srk.pub.algorithm = 0x00000001;     /* RSA */
    srk.pub.encscheme = 0x0003; /* RSA OAEP SHA1 MGF1 */
    srk.pub.sigscheme = 0x0001; /* NONE */
    srk.pub.keybitlen = RSA_MODULUS_BIT_SIZE;
    srk.pub.numprimes = 2;
    srk.pub.expsize = 0;        /* defaults to 0x010001 */
    srk.pub.keylength = 0;      /* not used here */
    srk.pub.pcrinfolen = 0;     /* not used here */
    /* convert to a memory buffer */
    srkparamsize = BuildKey(srk_param_buff, &srk);
    /* initiate the OIAP protocol */
    ret = TPM_OIAP(&sess);
    if (ret)
        return ret;
    /* calculate the Authorization Data */
    ret = authhmac(authdata, ownpass, TCG_HASH_SIZE, sess.enonce,
                   sess.ononce, 0, 4, &command, 2,
                   &protocol, 4, &oencdatasize,
                   ntohl(oencdatasize), ownerencr, 4,
                   &sencdatasize, ntohl(sencdatasize), srkencr,
                   srkparamsize, srk_param_buff, 0, 0);
    if (ret < 0) {
        TPM_Terminate_Handle(sess.handle);
        return -20;
    }
    /* insert all the calculated fields into the request buffer */
    ret = buildbuff(take_owner_fmt, tcpadata,
                    command,
                    protocol,
                    ntohl(oencdatasize),
                    ownerencr,
                    ntohl(sencdatasize),
                    srkencr,
                    srkparamsize,
                    srk_param_buff,
                    sess.handle,
                    TCG_HASH_SIZE, sess.ononce, TCG_HASH_SIZE, authdata);
    if (ret <= 0) {
        TPM_Terminate_Handle(sess.handle);
        return -21;
    }
    ret = TPM_Transmit(tcpadata, "Take Ownership");
    TPM_Terminate_Handle(sess.handle);
    if (ret != 0)
        return ret;
    /* check the response HMAC */
    srkparamsize = KeySize(tcpadata + TCG_DATA_OFFSET);
    ret = checkhmac(tcpadata, ntohl(command), sess.ononce, ownpass,
                    srkparamsize, 0);
    if (ret != 0)
        return -22;
    if (key == NULL)
        return 0;
    KeyExtract(tcpadata + TCG_DATA_OFFSET, key);
    return 0;
}
