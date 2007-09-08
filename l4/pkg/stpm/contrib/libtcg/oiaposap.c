/*
 * \brief   OSAP/OIAP functions.
 * \author  Bernhard Kauer <kauer@tudos.org>
 *
 * This file is inspired by code copyrighted 2003 by IBM.
 * See "License" for details.
 */


#include <tcg/tpm.h>
#include <tcg/ord.h>
#include <tcg/oiaposap.h>
#include <tcg/hmac.h>
#include <tcg/rand.h>

// Open a new OIAP session
TPM_TRANSMIT_FUNC(OIAP,
		  (oiapsess *sess),
		  if (sess == 0)
		  return -1;
		  rand_buffer(sess->ononce, TCG_NONCE_SIZE);
		  ,
		  sess->handle=TPM_EXTRACT_LONG(0);
		  TPM_COPY_FROM(sess->enonce, 4, TCG_NONCE_SIZE);
		  PRINT_LONG(sess->handle);
		  PRINT_HASH(sess->enonce);
		  ,
		  "");

// Open a new OSAP session
TPM_TRANSMIT_FUNC(OSAP,
		  (osapsess *sess,
		   unsigned char *key,
		   unsigned short etype,
		   unsigned long evalue)
		  ,
		  if (key == 0)
		  return -1;
		  rand_buffer(sess->ononceOSAP, TCG_NONCE_SIZE);
		  rand_buffer(sess->ononce, TCG_NONCE_SIZE);
		  ,
		  sess->handle=TPM_EXTRACT_LONG(0);
		  TPM_COPY_FROM(sess->enonce, 4, TCG_NONCE_SIZE);
		  TPM_COPY_FROM(sess->enonceOSAP, 4 + TCG_NONCE_SIZE, TCG_NONCE_SIZE);
		  ret = rawhmac(sess->ssecret, key, TCG_HASH_SIZE, TCG_NONCE_SIZE,
				sess->enonceOSAP, TCG_NONCE_SIZE, sess->ononceOSAP, 0,
				0);
		  PRINT_LONG(sess->handle);
		  PRINT_HASH(sess->enonce);
		  PRINT_HASH(sess->enonceOSAP);
		  PRINT_HASH(sess->ononceOSAP);
		  PRINT_HASH(sess->ssecret);
		  , 
		  "S L %",
		  etype,
		  evalue,
		  TCG_NONCE_SIZE,
		  sess->ononceOSAP
		  );



// Terminate the handle opened by TPM_OIAP or TPM_OSAP
TPM_TRANSMIT_FUNC(Terminate_Handle, (unsigned long handle), , , "L", handle);
