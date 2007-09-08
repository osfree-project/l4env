/****************************************************************************/
/*                                                                          */
/*                           SEAL/UNSEAL routines                           */
/*                                                                          */
/* This file is copyright 2003 IBM. See "License" for details               */
/*                                                                          */
/* Functions rewritten by Bernhard Kauer <kauer@tudos.org>                  */
/*                                                                          */
/****************************************************************************/

#include <tcg/oiaposap.h>
#include <tcg/pcrs.h>

/****************************************************************************/
/*                                                                          */
/* Seal a data object with caller Specified PCR infro                       */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* keyhandle is the TCG_KEY_HANDLE of the key used to seal the data        */
/*           0x40000000 for the SRK                                         */
/* pcrinfo   is a pointer to a TCG_PCR_INFO structure containing           */
/*           a bit map of the PCR's to seal the data to, and a              */
/*           pair of TCG_COMPOSITE_HASH values for the PCR's               */
/* pcrinfosize is the length of the pcrinfo structure                       */
/* keyauth   is the authorization data (password) for the key               */
/* dataauth  is the authorization data (password) for the data being sealed */
/*           both authorization values must be 20 bytes long ?              */
/* data      is a pointer to the data to be sealed                          */
/* datalen   is the length of the data to be sealed (max 256?)              */
/* blob      is a pointer to an area to received the sealed blob            */
/*           it should be long enough to receive the encrypted data         */
/*           which is 256 bytes, plus some overhead. 512 total recommended? */
/* bloblen   is a pointer to an integer which will receive the length       */
/*           of the sealed blob                                             */
/*                                                                          */
/****************************************************************************/

TPM_TRANSMIT_OSAP_FUNC(Seal,
		       (unsigned long keyhandle,
			unsigned char *pcrinfo, unsigned long pcrinfosize,
			unsigned char *keyauth,
			unsigned char *dataauth,
			unsigned char *data, unsigned int datalen,
			unsigned char *blob, unsigned int *bloblen)
		       ,
		       keyauth,
		       0x0001,
		       keyhandle
		       ,
		       unsigned char encauth[TCG_HASH_SIZE];

		       if (dataauth == NULL || blob == NULL || bloblen==NULL)
		         return -1;
		       PRINT_HASH(keyauth);
		       ,
		       CALC_ENC_AUTH(encauth, dataauth)
		       ,
		       {
			 *bloblen = TPM_EXTRACT_LONG(-8) - TCG_DATA_OFFSET - TCG_NONCE_SIZE - 1 - TCG_HASH_SIZE;
			 TPM_COPY_FROM(blob, 0, *bloblen);
		       }
		       ,
		       "I % @ @",
		       "L % @ @",
		       keyhandle,
		       TCG_HASH_SIZE, encauth,
		       pcrinfosize, pcrinfo,
		       datalen, data);


/****************************************************************************/
/*                                                                          */
/* Seal a data object with current PCR information                          */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* keyhandle is the TCG_KEY_HANDLE of the key used to seal the data        */
/*           0x40000000 for the SRK                                         */
/* pcrmap    is a 32 bit integer containing a bit map of the PCR register   */
/*           numbers to be used when sealing. e.g 0x0000001 specifies       */
/*           PCR 0. 0x00000003 specifies PCR's 0 and 1, etc.                */
/* keyauth   is the authorization data (password) for the key               */
/* dataauth  is the authorization data (password) for the data being sealed */
/*           both authorization values must be 20 bytes long ?              */
/* data      is a pointer to the data to be sealed                          */
/* datalen   is the length of the data to be sealed (max 256?)              */
/* blob      is a pointer to an area to received the sealed blob            */
/*           it should be long enough to receive the encrypted data         */
/*           which is 256 bytes, plus some overhead. 512 total recommended? */
/* bloblen   is a pointer to an integer which will receive the length       */
/*           of the sealed blob                                             */
/*                                                                          */
/****************************************************************************/
unsigned long TPM_Seal_CurrPCR(unsigned long keyhandle, unsigned long pcrmap,
			       unsigned char *keyauth,
			       unsigned char *dataauth,
			       unsigned char *data, unsigned int datalen,
			       unsigned char *blob, unsigned int *bloblen)
{
    unsigned char pcrinfo[MAXPCRINFOLEN];
    unsigned int pcrlen;

    pcrlen=0;
    GenPCRInfo(pcrmap, pcrinfo, &pcrlen);
    return TPM_Seal(keyhandle, pcrinfo, pcrlen,
                    keyauth, dataauth, data, datalen, blob, bloblen);
}

/****************************************************************************/
/*                                                                          */
/* Unseal a data object                                                     */
/*                                                                          */
/* The arguments are...                                                     */
/*                                                                          */
/* keyhandle is the TCG_KEY_HANDLE of the key used to seal the data        */
/*           0x40000000 for the SRK                                         */
/* keyauth   is the authorization data (password) for the key               */
/* dataauth  is the authorization data (password) for the data being sealed */
/*           both authorization values must be 20 bytes long ?              */
/* blob      is a pointer to an area to containing the sealed blob          */
/* bloblen   is the length of the sealed blob                               */
/* rawdata   is a pointer to an area to receive the unsealed data (max 256?)*/
/* datalen   is a pointer to a int to receive the length of the data        */
/*                                                                          */
/****************************************************************************/

TPM_TRANSMIT_AUTH2_FUNC(Unseal,
			(unsigned long key_handle,
			 unsigned char *key_auth,
			 unsigned char *data_auth,
			 unsigned char *blob, unsigned int bloblen,
			 unsigned char *rawdata, unsigned int *datalen),
			key_auth,
			data_auth,
			
			if (rawdata == NULL|| blob == NULL)
			  return -1;
			,
			*datalen = TPM_EXTRACT_LONG(0);
			TPM_COPY_FROM(rawdata, 4, *datalen);
			,
			"I %",
			"L %",
			key_handle,
			bloblen, blob
			);
