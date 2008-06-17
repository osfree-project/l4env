/****************************************************************************/
/*                                                                          */
/*  TPM.H  03 Apr 2003                                                      */
/*                                                                          */
/* This file is copyright 2003 IBM. See "License" for details               */
/*                                                                          */
/* Modified and extended with defines by Bernhard Kauer <kauer@tudos.org>   */
/*                                                                          */
/****************************************************************************/
#ifndef TPM_H
#define TPM_H

#include <stdint.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include "ord.h"
#include "transmit.h"
#include "buildbuff.h"

#define TCG_MAX_BUFF_SIZE              4096
#define TCG_HASH_SIZE                  20
#define TCG_NONCE_SIZE                 20
#define TCG_PARAMSIZE_OFFSET           2
#define TCG_RETURN_OFFSET		6
#define TCG_DATA_OFFSET                10
#define TCG_SRK_PARAM_BUFF_SIZE        256

#define TPM_TAG_RSP_COMMAND             0x00C4
#define TPM_TAG_RSP_AUTH1_COMMAND       0x00C5
#define TPM_TAG_RSP_AUTH2_COMMAND       0x00C6
#define RSA_MODULUS_BYTE_SIZE           256
#define RSA_MODULUS_BIT_SIZE  ( RSA_MODULUS_BYTE_SIZE * 8 )

/**
 * Defines a simple transmit function, which is used several times in
 * the lib, e.g. TPM_PcrRead, TPM_EvictKey, TPM_FlushSpecific.
 */
#define TPM_TRANSMIT_FUNC(NAME,PARAMS,PRECOND,POSTCOND,FMT,...) \
unsigned long STPM_##NAME PARAMS { \
  unsigned char buffer[TCG_MAX_BUFF_SIZE]; /* request/response buffer */ \
  unsigned long ret;         \
  PRECOND               \
  ret = buildbuff("00 C1 T L " FMT, buffer, TPM_ORD_##NAME, ##__VA_ARGS__ ); \
  if (ret < 0)          \
     return -1;         \
  ret = TPM_Transmit(buffer, #NAME ); \
  if (ret != 0)         \
      return ret;       \
  POSTCOND              \
  return ret;           \
} 

/**
 * Copy values from the buffer is often needed.
 */
#define TPM_COPY_FROM(DEST,OFFSET,SIZE) \
    memcpy(DEST, &buffer[TCG_DATA_OFFSET + OFFSET], SIZE)

/**
 * Extract long values from the buffer is often needed.
 */
#define TPM_EXTRACT_LONG(OFFSET) \
    ntohl(*(unsigned long *)(buffer+TCG_DATA_OFFSET+OFFSET))
/**
 * Extract short values from the buffer is often needed.
 */
#define TPM_EXTRACT_SHORT(OFFSET) \
    ntohs(*(unsigned short *)(buffer+TCG_DATA_OFFSET+OFFSET))


// Debugging

#define DEBUG_PRINT_HASH 0

#if DEBUG_PRINT_HASH

#define PRINT_HASH_SIZE(var,size) {  \
 int i;                              \
 printf("%20s: ",#var);              \
 for (i=0; i<size; i++)              \
   printf("%02X",(unsigned char)var[i]);            \
 printf("\n");                       \
}


#define PRINT_HASH(var) PRINT_HASH_SIZE(var,20);

#define PRINT_LONG(var) printf("%20s: %lx\n",#var,var);

#else

#define PRINT_HASH_SIZE(v,s)
#define PRINT_HASH(v)
#define PRINT_LONG(v)

#endif

#endif
