/****************************************************************************/
/*                                                                          */
/* OIAPOSAP.H 03 Apr 2003                                                   */
/*                                                                          */
/* This file is copyright 2003 IBM. See "License" for details               */
/*                                                                          */
/* Extended function defines by Bernhard Kauer <kauer@tudos.org>            */
/*                                                                          */
/****************************************************************************/
#ifndef OIAPOSAP_H
#define OIAPOSAP_H

#include <tcg/tpm.h>
#include <tcg/rand.h>
#include <tcg/hmac.h>


typedef struct osapsess {
    unsigned long handle;
    unsigned char enonce[TCG_NONCE_SIZE];
    unsigned char enonceOSAP[TCG_NONCE_SIZE];
    unsigned char ononceOSAP[TCG_NONCE_SIZE];
    unsigned char ssecret[TCG_HASH_SIZE];
    unsigned char ononce[TCG_NONCE_SIZE];
} osapsess;


typedef struct oiapsess {
    unsigned long handle;
    unsigned char enonce[TCG_NONCE_SIZE];
    unsigned char ononce[TCG_NONCE_SIZE];
} oiapsess;




unsigned long TPM_OIAP(oiapsess *sess);
unsigned long TPM_OSAP(osapsess *sess, unsigned char *key, unsigned short etype,
                  unsigned long evalue);
unsigned long TPM_Terminate_Handle(unsigned long handle);

/**
 * Defines an OIAP authenticated transmit function, which is used several times in
 * the lib, e.g. TPM_LoadKey
 */
#define TPM_TRANSMIT_OIAP_FUNC(NAME,PARAMS,KEY_AUTH,PRECOND,POSTCOND,DATA_SKIP,AUTHFMT,FMT,...)\
  unsigned long TPM_##NAME PARAMS {					\
    unsigned char buffer[TCG_MAX_BUFF_SIZE];				\
    oiapsess sess;							\
    unsigned char pubauth[TCG_HASH_SIZE];				\
    unsigned long ret;							\
    unsigned char c;							\
    									\
    PRECOND								\
									\
    if (KEY_AUTH==0)							\
      return -10;							\
									\
    if ((ret = TPM_OIAP(&sess)))					\
      return ret;							\
    c = 0;								\
      /* try to build the param buffer */				\
      ret = buildbuff("L" AUTHFMT, buffer,				\
		      TPM_ORD_##NAME,					\
		      ##__VA_ARGS__);					\
    if (ret < 0) {							\
      TPM_Terminate_Handle(sess.handle);				\
      return -20;							\
    }									\
									\
    /* hash the param buffer */						\
    ret=authhmac(pubauth,						\
		 KEY_AUTH, TCG_HASH_SIZE,				\
		 sess.enonce, sess.ononce, c,				\
		 ret, buffer,						\
		 0,0);							\
    if (ret < 0) {							\
      TPM_Terminate_Handle(sess.handle);				\
      return -30;							\
    }									\
    ret = buildbuff("00 C2 T L " FMT " L % o %",			\
		    buffer,						\
		    TPM_ORD_##NAME,					\
		    ##__VA_ARGS__,					\
		    sess.handle,					\
		    TCG_NONCE_SIZE, sess.ononce,			\
		    c,							\
		    TCG_HASH_SIZE, pubauth);				\
    if (ret < 0) {							\
      TPM_Terminate_Handle(sess.handle);				\
      return -40;							\
    }									\
    ret = TPM_Transmit(buffer, #NAME );					\
    if (ret != 0) {							\
      TPM_Terminate_Handle(sess.handle);				\
      return ret;							\
    }									\
    {									\
      unsigned count = TPM_EXTRACT_LONG(-8)				\
	- TCG_DATA_OFFSET						\
        - DATA_SKIP                                                     \
	- TCG_NONCE_SIZE						\
	- 1								\
	- TCG_HASH_SIZE;						\
      ret = checkhmac(buffer, TPM_ORD_##NAME,				\
		      sess.ononce,					\
		      KEY_AUTH,						\
                      count, DATA_SKIP);                                \
      if (ret < 0) {							\
	TPM_Terminate_Handle(sess.handle);				\
	return -50;							\
      }									\
    }									\
									\
    POSTCOND								\
									\
    TPM_Terminate_Handle(sess.handle);					\
    return ret;								\
  }



/**
 * Defines an authenticated transmit function, which is used several times in
 * the lib, e.g. TPM_ClearOwner
 */
#define TPM_TRANSMIT_OSAP_FUNC(NAME,PARAMS,KEY_AUTH,KEY_TYPE,KEY_VALUE,OSAPCOND,PRECOND,POSTCOND,AUTHFMT,FMT,...) \
  unsigned long TPM_##NAME PARAMS {					\
    unsigned char buffer[TCG_MAX_BUFF_SIZE]; /* request/response buffer */ \
    unsigned char pubauth[TCG_HASH_SIZE];				\
    osapsess sess;							\
    unsigned long ret;							\
    unsigned char c;							\
									\
    OSAPCOND								\
									\
      if (KEY_AUTH==0)							\
	return -10;							\
    ret = TPM_OSAP(&sess, KEY_AUTH, KEY_TYPE, KEY_VALUE);		\
    if (ret)								\
      return ret;							\
    c = 0;								\
									\
    PRECOND								\
									\
      /* try to build the param buffer */				\
      ret = buildbuff("L" AUTHFMT, buffer,				\
		      TPM_ORD_##NAME,					\
		      ##__VA_ARGS__);					\
    if (ret < 0) {							\
      TPM_Terminate_Handle(sess.handle);				\
      return -20;							\
    }									\
									\
    /* hash the param buffer */						\
    ret=authhmac(pubauth,						\
		 sess.ssecret, TCG_HASH_SIZE,				\
		 sess.enonce, sess.ononce, c,				\
		 ret, buffer,						\
		 0,0);							\
    if (ret < 0) {							\
      TPM_Terminate_Handle(sess.handle);				\
      return -30;							\
    }									\
    ret = buildbuff("00 C2 T L " FMT " L % o %",			\
		    buffer,						\
		    TPM_ORD_##NAME,					\
		    ##__VA_ARGS__,					\
		    sess.handle,					\
		    TCG_NONCE_SIZE, sess.ononce,			\
		    c,							\
		    TCG_HASH_SIZE, pubauth);				\
    if (ret < 0) {							\
      TPM_Terminate_Handle(sess.handle);				\
      return -40;							\
    }									\
    ret = TPM_Transmit(buffer, #NAME );					\
    if (ret != 0) {							\
      TPM_Terminate_Handle(sess.handle);				\
      return ret;							\
    }									\
    {									\
      unsigned count = TPM_EXTRACT_LONG(-8)				\
	- TCG_DATA_OFFSET						\
	- TCG_NONCE_SIZE						\
	- 1								\
	- TCG_HASH_SIZE;						\
      ret = checkhmac(buffer, TPM_ORD_##NAME,				\
		      sess.ononce,					\
		      sess.ssecret,					\
		      count, 0);					\
      if (ret < 0) {							\
	TPM_Terminate_Handle(sess.handle);				\
	return -50;							\
      }									\
    }									\
									\
    POSTCOND								\
									\
      TPM_Terminate_Handle(sess.handle);				\
    return ret;								\
  }



/**
 * Defines an authenticated transmit function with 2 AUTH sessions.
 */
#define TPM_TRANSMIT_AUTH2_FUNC(NAME,PARAMS,KEY1_AUTH,KEY2_AUTH, PRECOND,POSTCOND,AUTHFMT,FMT,...) \
  unsigned long TPM_##NAME PARAMS {					\
    unsigned char buffer[TCG_MAX_BUFF_SIZE];				\
    unsigned char pubauth1[TCG_HASH_SIZE];				\
    unsigned char pubauth2[TCG_HASH_SIZE];				\
    oiapsess sess1;							\
    oiapsess sess2;							\
    unsigned long ret;							\
    unsigned long length;						\
    unsigned char c;							\
    									\
    PRECOND								\
									\
    if (KEY1_AUTH==NULL || KEY2_AUTH==0)				\
      return -10;							\
									\
    if ((ret = TPM_OIAP(&sess1)))					\
      return ret;							\
    if ((ret = TPM_OIAP(&sess2)))					\
      return ret;							\
    c = 0;								\
    /* try to build the param buffer */					\
    length = buildbuff("L" AUTHFMT, buffer,				\
		    TPM_ORD_##NAME,					\
		    ##__VA_ARGS__);					\
    if (ret < 0) {							\
      TPM_Terminate_Handle(sess1.handle);				\
      TPM_Terminate_Handle(sess2.handle);				\
      return -20;							\
    }									\
									\
    /* hash the param buffer */						\
    ret=authhmac(pubauth1,						\
		 KEY1_AUTH, TCG_HASH_SIZE,				\
		 sess1.enonce, sess1.ononce, c,				\
		 length, buffer,					\
		 0,0);							\
    ret=authhmac(pubauth2,						\
		 KEY2_AUTH, TCG_HASH_SIZE,				\
		 sess2.enonce, sess2.ononce, c,				\
		 length, buffer,					\
		 0,0);							\
    if (ret < 0) {							\
      TPM_Terminate_Handle(sess1.handle);				\
      TPM_Terminate_Handle(sess2.handle);				\
      return -30;							\
    }									\
    ret = buildbuff("00 C3 T L " FMT " L % o % L % o %",		\
		    buffer,						\
		    TPM_ORD_##NAME,					\
		    ##__VA_ARGS__,					\
		    sess1.handle,					\
		    TCG_NONCE_SIZE, sess1.ononce,			\
		    c,							\
		    TCG_HASH_SIZE, pubauth1,				\
		    sess2.handle,					\
		    TCG_NONCE_SIZE, sess2.ononce,			\
		    c,							\
		    TCG_HASH_SIZE, pubauth2);				\
    if (ret < 0) {							\
      TPM_Terminate_Handle(sess1.handle);				\
      TPM_Terminate_Handle(sess2.handle);				\
      return -40;							\
    }									\
    ret = TPM_Transmit(buffer, #NAME );					\
    if (ret != 0) {							\
      TPM_Terminate_Handle(sess1.handle);				\
      TPM_Terminate_Handle(sess2.handle);				\
      return ret;							\
    }									\
    {									\
      unsigned count = TPM_EXTRACT_LONG(-8)				\
	- TCG_DATA_OFFSET						\
	- TCG_NONCE_SIZE*2						\
	- 1*2								\
	- TCG_HASH_SIZE*2;						\
      ret = checkhmac(buffer, TPM_ORD_##NAME,				\
		      sess2.ononce,					\
		      KEY2_AUTH,					\
		      count, 0);					\
      if (ret < 0) {							\
	TPM_Terminate_Handle(sess1.handle);				\
	TPM_Terminate_Handle(sess2.handle);				\
	return -50;							\
      }									\
      									\
      *(unsigned long *)(buffer+TCG_DATA_OFFSET-8) =			\
      htonl(TPM_EXTRACT_LONG(-8) - TCG_NONCE_SIZE - 1 - TCG_HASH_SIZE);	\
									\
      ret = checkhmac(buffer, TPM_ORD_##NAME,				\
		      sess1.ononce,					\
		      KEY1_AUTH,					\
		      count, 0);					\
      if (ret < 0) {							\
	TPM_Terminate_Handle(sess1.handle);				\
	TPM_Terminate_Handle(sess2.handle);				\
	return -60;							\
      }									\
    }									\
									\
    POSTCOND								\
      									\
    TPM_Terminate_Handle(sess1.handle);				        \
    TPM_Terminate_Handle(sess2.handle);					\
    return ret;								\
  }



/**
 * Defines an macro to calculate a new encrypted authorization value.
 * Used e.g. TPM_Seal or TPM_CreateWrapKey
 */
#define CALC_ENC_AUTH(DEST, SRC)					\
     if (SRC)								\
       {								\
	 unsigned char xorwork[TCG_HASH_SIZE * 2];			\
	 unsigned char xorhash[TCG_HASH_SIZE];				\
	 unsigned i;							\
									\
	 memcpy(xorwork, sess.ssecret, TCG_HASH_SIZE);			\
	 memcpy(xorwork + TCG_HASH_SIZE, sess.enonce, TCG_HASH_SIZE);	\
	 sha1(xorwork, TCG_HASH_SIZE * 2, xorhash);			\
	 for (i = 0; i < TCG_HASH_SIZE; i++)				\
	   DEST[i] = xorhash[i] ^ SRC[i];				\
	 PRINT_HASH(DEST);						\
       }								\
     else								\
       memset(DEST, 0, TCG_HASH_SIZE);

#endif
