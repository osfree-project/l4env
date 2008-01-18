/* Software-Based Trusted Platform Module (TPM) Emulator for Linux
 * Copyright (C) 2004 Mario Strasser <mast@gmx.net>,
 *                    Swiss Federal Institute of Technology (ETH) Zurich
 *
 * This module is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * This module is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * $Id: tpm_authorization.c 139 2006-11-10 16:09:00Z mast $
 */

#include "tpm_emulator.h"
#include "tpm_commands.h"
#include "tpm_handles.h"
#include "tpm_data.h"
#include "crypto/hmac.h"
#include "crypto/sha1.h"

/*
 * Authorization Changing ([TPM_Part3], Section 17)
 */

extern int tpm_encrypt_sealed_data(TPM_KEY_DATA *key, TPM_SEALED_DATA *seal,
                                   BYTE *enc, UINT32 *enc_size);
                               
extern int tpm_decrypt_sealed_data(TPM_KEY_DATA *key, BYTE *enc, UINT32 enc_size,
                                   TPM_SEALED_DATA *seal, BYTE **buf);
                               
extern int tpm_encrypt_private_key(TPM_KEY_DATA *key, TPM_STORE_ASYMKEY *store,
                                   BYTE *enc, UINT32 *enc_size);
                  
extern int tpm_decrypt_private_key(TPM_KEY_DATA *key, BYTE *enc, UINT32 enc_size, 
                                   TPM_STORE_ASYMKEY *store, BYTE **buf);

TPM_RESULT TPM_ChangeAuth(TPM_KEY_HANDLE parentHandle,
                          TPM_PROTOCOL_ID protocolID, TPM_ENCAUTH *newAuth,
                          TPM_ENTITY_TYPE entityType, UINT32 encDataSize,
                          BYTE *encData, TPM_AUTH *auth1, TPM_AUTH *auth2,
                          UINT32 *outDataSize, BYTE **outData)
{
  TPM_RESULT res;
  TPM_KEY_DATA *parent;
  TPM_SESSION_DATA *session;
  TPM_SECRET plainAuth;
  info("TPM_ChangeAuth()");
  /* get parent key */
  parent = tpm_get_key(parentHandle);
  if (parent == NULL) return TPM_INVALID_KEYHANDLE;
  /* verify entity authorization */ 
  auth2->continueAuthSession = FALSE;
  session = tpm_get_auth(auth2->authHandle);
  if (session->type != TPM_ST_OIAP) return TPM_BAD_MODE; 
  /* verify parent authorization */
  res = tpm_verify_auth(auth1, parent->usageAuth, parentHandle);
  if (res != TPM_SUCCESS) return res;
  auth1->continueAuthSession = FALSE;
  session = tpm_get_auth(auth1->authHandle);
  if (session->type != TPM_ST_OSAP) return TPM_BAD_MODE;  
  /* decrypt auth */
  tpm_decrypt_auth_secret(*newAuth, session->sharedSecret,
                          &session->lastNonceEven, plainAuth);
  /* decrypt the entity, replace authData, and encrypt it again */
  if (entityType == TPM_ET_DATA) {
    TPM_SEALED_DATA seal;
    BYTE *seal_buf;
    /* decrypt entity */
    if (tpm_decrypt_sealed_data(parent, encData, encDataSize,
        &seal, &seal_buf)) return TPM_DECRYPT_ERROR;
    /* verify auth2 */
    res = tpm_verify_auth(auth2, seal.authData, TPM_INVALID_HANDLE);
    if (res != TPM_SUCCESS) return (res == TPM_AUTHFAIL) ? TPM_AUTH2FAIL : res;
    /* change authData and use it also for auth2 */
    memcpy(seal.authData, plainAuth, sizeof(TPM_SECRET));    
    /* encrypt entity */
    *outDataSize = parent->key.size >> 3;
    *outData = tpm_malloc(*outDataSize);
    if (tpm_encrypt_sealed_data(parent, &seal, *outData, outDataSize)) {
      tpm_free(encData);
      tpm_free(seal_buf);      
      return TPM_ENCRYPT_ERROR;
    }                    
    tpm_free(seal_buf); 
  } else if (entityType == TPM_ET_KEY) {
    TPM_STORE_ASYMKEY store;
    BYTE *store_buf;
    /* decrypt entity */
    if (tpm_decrypt_private_key(parent, encData, encDataSize,
        &store, &store_buf)) return TPM_DECRYPT_ERROR;
    /* verify auth2 */
    res = tpm_verify_auth(auth2, store.usageAuth, TPM_INVALID_HANDLE);
    if (res != TPM_SUCCESS) return (res == TPM_AUTHFAIL) ? TPM_AUTH2FAIL : res;
    /* change usageAuth and use it also for auth2 */
    memcpy(store.usageAuth, plainAuth, sizeof(TPM_SECRET));  
    /* encrypt entity */
    *outDataSize = parent->key.size >> 3;
    *outData = tpm_malloc(*outDataSize);
    if (tpm_encrypt_private_key(parent, &store, *outData, outDataSize)) {
      tpm_free(encData);
      tpm_free(store_buf);      
      return TPM_ENCRYPT_ERROR;
    }                    
    tpm_free(store_buf); 
  } else {
    return TPM_WRONG_ENTITYTYPE;
  }
  return TPM_SUCCESS;
}

TPM_RESULT TPM_ChangeAuthOwner(TPM_PROTOCOL_ID protocolID, 
                               TPM_ENCAUTH *newAuth, 
                               TPM_ENTITY_TYPE entityType, TPM_AUTH *auth1)
{
  TPM_RESULT res;
  TPM_SESSION_DATA *session;
  TPM_SECRET plainAuth;
  int i;
  info("TPM_ChangeAuthOwner()");
  /* verify authorization */
  res = tpm_verify_auth(auth1, tpmData.permanent.data.ownerAuth, TPM_KH_OWNER);
  if (res != TPM_SUCCESS) return res;
  auth1->continueAuthSession = FALSE;
  session = tpm_get_auth(auth1->authHandle);
  if (session->type != TPM_ST_OSAP) return TPM_AUTHFAIL;
  /* decrypt auth */
  tpm_decrypt_auth_secret(*newAuth, session->sharedSecret,
                          &session->lastNonceEven, plainAuth);
  /* change authorization data */
  if (entityType == TPM_ET_OWNER) {
    memcpy(tpmData.permanent.data.ownerAuth, plainAuth, sizeof(TPM_SECRET));
    /* invalidate all associated sessions but the current one */
    for (i = 0; i < TPM_MAX_SESSIONS; i++) {
      if (tpmData.stany.data.sessions[i].handle == TPM_KH_OWNER
          && &tpmData.stany.data.sessions[i] != session) {
          memset(&tpmData.stany.data.sessions[i], 0, sizeof(TPM_SESSION_DATA));
      }
    }
  } else if (entityType == TPM_ET_SRK) {
    memcpy(tpmData.permanent.data.srk.usageAuth, plainAuth, sizeof(TPM_SECRET));
/* probably not correct; spec. v1.2 rev94 says nothing about authDataUsage
    tpmData.permanent.data.srk.authDataUsage = TPM_AUTH_ALWAYS;
*/
    /* invalidate all associated sessions but the current one */
    for (i = 0; i < TPM_MAX_SESSIONS; i++) {
      if (tpmData.stany.data.sessions[i].handle == TPM_KH_SRK
          && &tpmData.stany.data.sessions[i] != session) {
          memset(&tpmData.stany.data.sessions[i], 0, sizeof(TPM_SESSION_DATA));
      }
    }
  } else {
    return TPM_WRONG_ENTITYTYPE;
  }
  return TPM_SUCCESS;
}

/*
 * Authorization Sessions ([TPM_Part3], Section 18)
 */

UINT32 tpm_get_free_session(BYTE type)
{
  UINT32 i;
  for (i = 0; i < TPM_MAX_SESSIONS; i++) {
    if (tpmData.stany.data.sessions[i].type == TPM_ST_INVALID) {
      tpmData.stany.data.sessions[i].type = type;
      if (type == TPM_ST_TRANSPORT) return INDEX_TO_TRANS_HANDLE(i);
      else return INDEX_TO_AUTH_HANDLE(i);
    }
  }
  return TPM_INVALID_HANDLE;
}

TPM_RESULT TPM_OIAP(TPM_AUTHHANDLE *authHandle, TPM_NONCE *nonceEven)
{
  TPM_SESSION_DATA *session;
  info("TPM_OIAP()");
  /* get a free session if any is left */
  *authHandle = tpm_get_free_session(TPM_ST_OIAP);
  session = tpm_get_auth(*authHandle);
  if (session == NULL) return TPM_RESOURCES;
  /* setup session */
  tpm_get_random_bytes(nonceEven->nonce, sizeof(nonceEven->nonce));
  memcpy(&session->nonceEven, nonceEven, sizeof(TPM_NONCE));
  return TPM_SUCCESS;
}

TPM_RESULT TPM_OSAP(TPM_ENTITY_TYPE entityType, UINT32 entityValue, 
                    TPM_NONCE *nonceOddOSAP, TPM_AUTHHANDLE *authHandle,
                    TPM_NONCE *nonceEven, TPM_NONCE *nonceEvenOSAP)
{
  tpm_hmac_ctx_t ctx;
  TPM_SESSION_DATA *session;
  TPM_SECRET *secret = NULL;
  info("TPM_OSAP()");
  /* get a free session if any is left */
  *authHandle = tpm_get_free_session(TPM_ST_OSAP);
  session = tpm_get_auth(*authHandle);
  if (session == NULL) return TPM_RESOURCES;
  /* check whether ADIP encryption scheme is supported */
  switch (entityType & 0xFF00) {
    case TPM_ET_XOR:
      break;
    default:
      return TPM_INAPPROPRIATE_ENC;
  }
  /* get resource handle and the dedicated secret */
  switch (entityType & 0x00FF) {
    case TPM_ET_KEYHANDLE:
      session->handle = entityValue;
      if (session->handle == TPM_KH_OPERATOR) return TPM_BAD_HANDLE;
      if (tpm_get_key(session->handle) != NULL)
        secret = &tpm_get_key(session->handle)->usageAuth;
      break;
    case TPM_ET_OWNER:
      session->handle = TPM_KH_OWNER;
      if (tpmData.permanent.flags.owned)
        secret = &tpmData.permanent.data.ownerAuth;
      break;
    case TPM_ET_SRK:
      session->handle = TPM_KH_SRK;
      if (tpmData.permanent.data.srk.valid)
        secret = &tpmData.permanent.data.srk.usageAuth;
      break;
    case TPM_ET_COUNTER:
      session->handle = entityValue;
      if (tpm_get_counter(session->handle) != NULL)
        secret = &tpm_get_counter(session->handle)->usageAuth;
      break;
    case TPM_ET_NV:
      /* TODO: session->handle = entityValue;
      if (tpm_get_nvdata(session->handle) != NULL)
        secret = &tpm_get_nvdata(session->handle)->usageAuth;*/
      break;
    default:
      return TPM_BAD_PARAMETER;
  }
  if (secret == NULL) {
    memset(session, 0, sizeof(*session));
    return TPM_BAD_PARAMETER;
  }
  /* save entity type */
  session->entityType = entityType;
  /* generate nonces */
  tpm_get_random_bytes(nonceEven->nonce, sizeof(nonceEven->nonce));
  memcpy(&session->nonceEven, nonceEven, sizeof(TPM_NONCE));
  tpm_get_random_bytes(nonceEvenOSAP->nonce, sizeof(nonceEvenOSAP->nonce));
  /* compute shared secret */
  tpm_hmac_init(&ctx, *secret, sizeof(*secret));
  tpm_hmac_update(&ctx, nonceEvenOSAP->nonce, sizeof(nonceEvenOSAP->nonce));
  tpm_hmac_update(&ctx, nonceOddOSAP->nonce, sizeof(nonceOddOSAP->nonce));
  tpm_hmac_final(&ctx, session->sharedSecret);
  return TPM_SUCCESS;
}

TPM_RESULT TPM_DSAP(  
  TPM_KEY_HANDLE KeyHandle,
  UINT32 entityValueSize,
  BYTE *entityValue,
  TPM_NONCE *nonceOddDSAP,
  TPM_AUTHHANDLE *authHandle,
  TPM_NONCE *nonceEven,
  TPM_NONCE *nonceEvenDSAP
)
{
  info("TPM_DSAP() not implemented yet");
  /* TODO: implement TPM_DSAP() */
  return TPM_FAIL;
}

TPM_RESULT TPM_SetOwnerPointer(TPM_ENTITY_TYPE entityType, UINT32 entityValue)
{
  info("TPM_SetOwnerPointer() is not supported");
  return TPM_DISABLED_CMD;
}

TPM_RESULT tpm_verify_auth(TPM_AUTH *auth, TPM_SECRET secret, 
                           TPM_HANDLE handle)
{
  tpm_hmac_ctx_t ctx;
  TPM_SESSION_DATA *session;
  UINT32 auth_handle = CPU_TO_BE32(auth->authHandle);
  
  info("tpm_verify_auth()");
  debug("[ handle=%.8x ]", auth->authHandle);
  /* get dedicated authorization or transport session */
  session = tpm_get_auth(auth->authHandle);
  if (session == NULL) session = tpm_get_transport(auth->authHandle);
  if (session == NULL) return TPM_INVALID_AUTHHANDLE;
  /* setup authorization */
  if (session->type == TPM_ST_OIAP) {
    debug("[TPM_ST_OIAP]");
    /* We copy the secret because it might be deleted or invalidated 
       afterwards, but we need it again for authorizing the response. */
    memcpy(session->sharedSecret, secret, sizeof(TPM_SECRET));
  } else if (session->type == TPM_ST_OSAP) {
    debug("[TPM_ST_OSAP]");
    if (session->handle != handle) return TPM_AUTHFAIL;
  } else if (session->type == TPM_ST_TRANSPORT) {
    debug("[TPM_ST_TRANSPORT]");
    memcpy(session->sharedSecret, secret, sizeof(TPM_SECRET));
  } else {
    return TPM_INVALID_AUTHHANDLE;
  }
  auth->secret = &session->sharedSecret;
  /* verify authorization */
  tpm_hmac_init(&ctx, *auth->secret, sizeof(*auth->secret));
  tpm_hmac_update(&ctx, auth->digest, sizeof(auth->digest));
  if (session->type == TPM_ST_OIAP && FALSE) 
    tpm_hmac_update(&ctx, (BYTE*)&auth_handle, 4);
  tpm_hmac_update(&ctx, session->nonceEven.nonce, sizeof(session->nonceEven.nonce));
  tpm_hmac_update(&ctx, auth->nonceOdd.nonce, sizeof(auth->nonceOdd.nonce));
  tpm_hmac_update(&ctx, &auth->continueAuthSession, 1);
  tpm_hmac_final(&ctx, auth->digest);
  if (memcmp(auth->digest, auth->auth, sizeof(auth->digest))) return TPM_AUTHFAIL;
  /* generate new nonceEven */
  memcpy(&session->lastNonceEven, &session->nonceEven, sizeof(TPM_NONCE));
  tpm_get_random_bytes(auth->nonceEven.nonce, sizeof(auth->nonceEven.nonce));
  memcpy(&session->nonceEven, &auth->nonceEven, sizeof(TPM_NONCE));
  return TPM_SUCCESS;
}

void tpm_decrypt_auth_secret(TPM_ENCAUTH encAuth, TPM_SECRET secret,
                             TPM_NONCE *nonce, TPM_SECRET plainAuth)
{
  unsigned int i;
  tpm_sha1_ctx_t ctx;
  tpm_sha1_init(&ctx);
  tpm_sha1_update(&ctx, secret, sizeof(TPM_SECRET));
  tpm_sha1_update(&ctx, nonce->nonce, sizeof(nonce->nonce));
  tpm_sha1_final(&ctx, plainAuth);
  for (i = 0; i < sizeof(TPM_SECRET); i++)
    plainAuth[i] ^= encAuth[i];
}

