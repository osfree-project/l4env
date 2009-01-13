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
 * $Id: tpm_transport.c 228 2007-12-16 13:22:53Z hstamer $
 */

/* 
 * Thanks go to Edison Su (<sudison@gmail.com>) for providing
 * the initial Transport Session patch.
 */

#include "tpm_emulator.h"
#include "tpm_commands.h"
#include "tpm_handles.h"
#include "tpm_marshalling.h"
#include "tpm_data.h"
#include "crypto/rsa.h"
#include "crypto/sha1.h"

/*
 * Transport Sessions ([TPM_Part3], Section 24)
 */

static int decrypt_transport_auth(TPM_KEY_DATA *key, BYTE *enc, UINT32 enc_size,
                                  TPM_TRANSPORT_AUTH *trans_auth) 
{
  BYTE *buf;
  size_t buf_size;
  int scheme;
  switch (key->encScheme) {
    case TPM_ES_RSAESOAEP_SHA1_MGF1: scheme = RSA_ES_OAEP_SHA1; break;
    case TPM_ES_RSAESPKCSv15: scheme = RSA_ES_PKCSV15; break;
    default: return -1;
  }
  buf = tpm_malloc(key->key.size);
  if (buf == NULL
      || tpm_rsa_decrypt(&key->key, scheme, enc, enc_size, buf, &buf_size)
      || buf_size != sizeof_TPM_TRANSPORT_AUTH(x)
      || (((UINT16)buf[0] << 8) | buf[1]) != TPM_TAG_TRANSPORT_AUTH) {
    tpm_free(buf);
    return -1;
  }
  trans_auth->tag = TPM_TAG_TRANSPORT_AUTH;
  memcpy(trans_auth->authData, &buf[2], sizeof(TPM_AUTHDATA));
  tpm_free(buf);
  return 0;
}

static void transport_log_in(TPM_COMMAND_CODE ordinal, BYTE parameters[20],
                             BYTE pubKeyHash[20], TPM_DIGEST *transDigest)
{
  UINT32 tag = CPU_TO_BE32(TPM_TAG_TRANSPORT_LOG_IN);
  BYTE *ptr, buf[sizeof_TPM_TRANSPORT_LOG_IN(x)];
  UINT32 len;
  tpm_sha1_ctx_t sha1;

  ptr = buf; len = sizeof(buf);
  tpm_marshal_TPM_TAG(&ptr, &len, tag);
/* removed since v1.2 rev 94
  tpm_marshal_TPM_COMMAND_CODE(&ptr, &len, ordinal);
*/
  tpm_marshal_BYTE_ARRAY(&ptr, &len, parameters, 20);
  tpm_marshal_BYTE_ARRAY(&ptr, &len, pubKeyHash, 20);
  tpm_sha1_init(&sha1);
  tpm_sha1_update(&sha1, transDigest->digest, sizeof(transDigest->digest));
  tpm_sha1_update(&sha1, buf, sizeof(buf));
  tpm_sha1_final(&sha1, transDigest->digest);
}

static void transport_log_out(TPM_CURRENT_TICKS *currentTicks, BYTE parameters[20],
                              TPM_MODIFIER_INDICATOR locality, TPM_DIGEST *transDigest)
{
  UINT32 tag = CPU_TO_BE32(TPM_TAG_TRANSPORT_LOG_OUT);
  BYTE *ptr, buf[sizeof_TPM_TRANSPORT_LOG_OUT(x)];
  UINT32 len;
  tpm_sha1_ctx_t sha1;

  ptr = buf; len = sizeof(buf);
  tpm_marshal_TPM_TAG(&ptr, &len, tag);
  tpm_marshal_TPM_CURRENT_TICKS(&ptr, &len, currentTicks);
  tpm_marshal_BYTE_ARRAY(&ptr, &len, parameters, 20);
  tpm_marshal_TPM_MODIFIER_INDICATOR(&ptr, &len, locality);
  tpm_sha1_init(&sha1);
  tpm_sha1_update(&sha1, transDigest->digest, sizeof(transDigest->digest));
  tpm_sha1_update(&sha1, buf, sizeof(buf));
  tpm_sha1_final(&sha1, transDigest->digest);
}

extern UINT32 tpm_get_free_session(BYTE type);

TPM_RESULT TPM_EstablishTransport(TPM_KEY_HANDLE encHandle,
                                  TPM_TRANSPORT_PUBLIC *transPublic,
                                  UINT32 secretSize, BYTE *secret,
                                  TPM_AUTH *auth1,
                                  TPM_TRANSHANDLE *transHandle,
                                  TPM_MODIFIER_INDICATOR *locality,
                                  TPM_CURRENT_TICKS *currentTicks,
                                  TPM_NONCE *transNonce)
{
  TPM_RESULT res;
  TPM_KEY_DATA *key;
  TPM_TRANSPORT_AUTH trans_auth;
  TPM_SESSION_DATA *session;
  BYTE *ptr, buf[4 + 4 + 4 + sizeof_TPM_CURRENT_TICKS(x) + 20];
  UINT32 len;
  tpm_sha1_ctx_t sha1;

  info("TPM_EstablishTransport()");
  /* setup authorization data */
  if (encHandle == TPM_KH_TRANSPORT) {
    if (auth1->authHandle != TPM_INVALID_HANDLE) return TPM_BADTAG;
    if (transPublic->transAttributes & TPM_TRANSPORT_ENCRYPT) return TPM_BAD_SCHEME;
    if (secretSize != 20) return TPM_BAD_PARAM_SIZE;
    memcpy(trans_auth.authData, secret, 20);
  } else {
    /* get key and verify its usage */
    key = tpm_get_key(encHandle);
    if (key == NULL) return TPM_INVALID_KEYHANDLE;
    if (key->keyUsage != TPM_KEY_STORAGE && key->keyUsage != TPM_KEY_LEGACY)
        return TPM_INVALID_KEYUSAGE;
    /* verify authorization */ 
    if (key->authDataUsage != TPM_AUTH_NEVER) {
      res = tpm_verify_auth(auth1, key->usageAuth, encHandle);
      if (res != TPM_SUCCESS) return res;
      if (decrypt_transport_auth(key, secret, secretSize, &trans_auth))
        return TPM_DECRYPT_ERROR;
    }
  }
  /* check whether the transport has to be encrypted */
  if (transPublic->transAttributes & TPM_TRANSPORT_ENCRYPT) {
    if (tpmData.permanent.flags.FIPS
        && transPublic->algID == TPM_ALG_MGF1) return TPM_INAPPROPRIATE_ENC;
    /* until now, only MGF1 is supported */
    if (transPublic->algID != TPM_ALG_MGF1) return TPM_BAD_KEY_PROPERTY;
  }
  /* initialize transport session */
  tpm_get_random_bytes(transNonce->nonce, sizeof(transNonce->nonce));
  *transHandle = tpm_get_free_session(TPM_ST_TRANSPORT);
  session = tpm_get_transport(*transHandle);
  if (session == NULL) return TPM_RESOURCES;
  session->transInternal.transHandle = *transHandle;
  memset(&session->transInternal.transDigest, 0, sizeof(TPM_DIGEST));
  memcpy(&session->transInternal.transPublic, transPublic,
    sizeof_TPM_TRANSPORT_PUBLIC((*transPublic)));
  memcpy(&session->transInternal.transNonceEven, transNonce, sizeof(TPM_NONCE));
  memcpy(&session->transInternal.authData, trans_auth.authData, sizeof(TPM_AUTHDATA));
  *locality = tpmData.stany.flags.localityModifier;
  memcpy(currentTicks, &tpmData.stany.data.currentTicks, sizeof(TPM_CURRENT_TICKS));
  /* perform transport logging */
  if (transPublic->transAttributes & TPM_TRANSPORT_LOG) {
    memset(buf, 0, 20); /* set pubKeyHash to all zeros */
    transport_log_in(TPM_ORD_EstablishTransport, auth1->digest, buf,
      &session->transInternal.transDigest);
    /* compute SHA-1 (output parameters) */
    ptr = buf; len = sizeof(buf);
    tpm_marshal_UINT32(&ptr, &len, TPM_SUCCESS); /* return code */
    tpm_marshal_TPM_COMMAND_CODE(&ptr, &len, TPM_ORD_EstablishTransport);
    tpm_marshal_TPM_MODIFIER_INDICATOR(&ptr, &len, *locality);
    tpm_marshal_TPM_CURRENT_TICKS(&ptr, &len, currentTicks);
    tpm_marshal_TPM_NONCE(&ptr, &len, transNonce);
    tpm_sha1_init(&sha1);
    tpm_sha1_update(&sha1, buf, sizeof(buf));
    tpm_sha1_final(&sha1, buf);
    transport_log_out(currentTicks, buf, *locality,
      &session->transInternal.transDigest);
  }
  /* check whether this is a exclusive transport session */
  if (transPublic->transAttributes & TPM_TRANSPORT_EXCLUSIVE) {
    tpmData.stany.flags.transportExclusive = TRUE;
    tpmData.stany.data.transExclusive = *transHandle;
  }
  return TPM_SUCCESS;
}

extern UINT32 tpm_get_in_param_offset(TPM_COMMAND_CODE ordinal);
extern void tpm_compute_in_param_digest(TPM_REQUEST *req);
extern void tpm_execute_command(TPM_REQUEST *req, TPM_RESPONSE *rsp);

static void decrypt_wrapped_command(BYTE *buf, UINT32 buf_len,
                                    TPM_AUTH *auth, TPM_SECRET secret)

{
  UINT32 i, j;
  BYTE mask[SHA1_DIGEST_LENGTH];
  tpm_sha1_ctx_t sha1;
  for (i = 0; buf_len > 0; i++) {
    tpm_sha1_init(&sha1);
    tpm_sha1_update(&sha1, auth->nonceEven.nonce, sizeof(auth->nonceEven.nonce));
    tpm_sha1_update(&sha1, auth->nonceOdd.nonce, sizeof(auth->nonceOdd.nonce));
    tpm_sha1_update(&sha1, (uint8_t*)"in", 2);
    tpm_sha1_update(&sha1, secret, sizeof(TPM_SECRET));
    j = CPU_TO_BE32(i);
    tpm_sha1_update(&sha1, (BYTE*)&j, 4);
    tpm_sha1_final(&sha1, mask);
    for (j = 0; j < sizeof(mask) && buf_len > 0; j++) { 
      *buf++ ^= mask[j];
      buf_len--;
    }
  }
}

static void encrypt_wrapped_command(BYTE *buf, UINT32 buf_len,
                                    TPM_AUTH *auth, TPM_SECRET secret)
{
  UINT32 i, j;
  BYTE mask[SHA1_DIGEST_LENGTH];
  tpm_sha1_ctx_t sha1;
  for (i = 0; buf_len > 0; i++) {
    tpm_sha1_init(&sha1);
    tpm_sha1_update(&sha1, auth->nonceEven.nonce, sizeof(auth->nonceEven.nonce));
    tpm_sha1_update(&sha1, auth->nonceOdd.nonce, sizeof(auth->nonceOdd.nonce));
    tpm_sha1_update(&sha1, (uint8_t*)"out", 3);
    tpm_sha1_update(&sha1, secret, sizeof(TPM_SECRET));
    j = CPU_TO_BE32(i);
    tpm_sha1_update(&sha1, (BYTE*)&j, 4);
    tpm_sha1_final(&sha1, mask);
    for (j = 0; j < sizeof(mask) && buf_len > 0; j++) { 
      *buf++ ^= mask[j];
      buf_len--;
    }
  }
}

TPM_RESULT TPM_ExecuteTransport(UINT32 inWrappedCmdSize, BYTE *inWrappedCmd,
                                TPM_AUTH *auth1, TPM_CURRENT_TICKS *currentTicks,
                                TPM_MODIFIER_INDICATOR *locality,
                                UINT32 *outWrappedCmdSize, BYTE **outWrappedCmd)
{
  TPM_RESULT res;
  TPM_SESSION_DATA *session;
  TPM_REQUEST req;
  TPM_RESPONSE rsp;
  BYTE *ptr, buf[4 + 4 + 4 + sizeof_TPM_CURRENT_TICKS(x) + 20];
  UINT32 len, offset;
  tpm_sha1_ctx_t sha1;
  info("TPM_ExecuteTransport()");
  /* get transport session */
  session = tpm_get_transport(auth1->authHandle);
  if (session == NULL) return TPM_BAD_PARAMETER;
  /* unmarshal wrapped command */
  if (tpm_unmarshal_TPM_REQUEST(&inWrappedCmd, &inWrappedCmdSize, &req)) return TPM_FAIL;
  /* decrypt wrapped command if needed */
  ptr = tpm_malloc(req.paramSize);
  if (ptr == NULL) return TPM_FAIL;
  memcpy(ptr, req.param, req.paramSize);
  offset = tpm_get_in_param_offset(req.ordinal);
  if (session->transInternal.transPublic.transAttributes & TPM_TRANSPORT_ENCRYPT)
    decrypt_wrapped_command(ptr + offset, req.paramSize - offset,
                            auth1, session->transInternal.authData);
  req.param = ptr;
  /* verify authorization */
  tpm_compute_in_param_digest(&req);
  tpm_sha1_init(&sha1);
  res = CPU_TO_BE32(TPM_ORD_ExecuteTransport);
  tpm_sha1_update(&sha1, (BYTE*)&res, 4);
  res = CPU_TO_BE32(inWrappedCmdSize);
  tpm_sha1_update(&sha1, (BYTE*)&res, 4);
  tpm_sha1_update(&sha1, req.auth1.digest, sizeof(req.auth1.digest));
  tpm_sha1_final(&sha1, auth1->digest);
  res = tpm_verify_auth(auth1, session->transInternal.authData, TPM_INVALID_HANDLE);
  if (res != TPM_SUCCESS) {
    tpm_free(ptr);
    return res;
  }
  /* nested transport sessions are not allowed */
  if (req.ordinal == TPM_ORD_EstablishTransport
      || req.ordinal == TPM_ORD_ExecuteTransport
      || req.ordinal == TPM_ORD_ReleaseTransportSigned) {
    tpm_free(ptr);
    return TPM_NO_WRAP_TRANSPORT;
  }
  /* log input parameters */
  if (session->transInternal.transPublic.transAttributes & TPM_TRANSPORT_LOG) {
    memset(buf, 0, 20);
    transport_log_in(TPM_ORD_EstablishTransport, req.auth1.digest, buf,
      &session->transInternal.transDigest);
  }
  /* execute and audit command*/
  tpm_audit_request(req.ordinal, &req);
  tpm_execute_command(&req, &rsp);
  tpm_audit_response(req.ordinal, &rsp);
  tpm_free(ptr);
  *locality = tpmData.stany.flags.localityModifier;
  memcpy(currentTicks, &tpmData.stany.data.currentTicks, sizeof(TPM_CURRENT_TICKS));
  /* marshal response */
  *outWrappedCmdSize = len = rsp.size;
  *outWrappedCmd = ptr = tpm_malloc(len);
  if (ptr == NULL) {
    tpm_free(rsp.param);
    return TPM_FAIL;
  }
  tpm_marshal_TPM_RESPONSE(&ptr, &len, &rsp);
  tpm_free(rsp.param);
  /* compute digest of output parameters */
  ptr = buf; len = sizeof(buf);
  tpm_marshal_UINT32(&ptr, &len, TPM_SUCCESS);
  tpm_marshal_TPM_COMMAND_CODE(&ptr, &len, TPM_ORD_ExecuteTransport);
  tpm_marshal_TPM_CURRENT_TICKS(&ptr, &len, currentTicks);
  tpm_marshal_TPM_MODIFIER_INDICATOR(&ptr, &len, *locality);
  memcpy(ptr, rsp.auth1->digest, sizeof(rsp.auth1->digest));
  tpm_sha1_init(&sha1);
  tpm_sha1_update(&sha1, buf, sizeof(buf));
  tpm_sha1_final(&sha1, auth1->digest);
  /* log output parameters */
  if (session->transInternal.transPublic.transAttributes & TPM_TRANSPORT_LOG) {
    transport_log_out(currentTicks, auth1->digest, *locality,
      &session->transInternal.transDigest);
  }
  /* encrypt response */
  if (session->transInternal.transPublic.transAttributes & TPM_TRANSPORT_ENCRYPT) {
    encrypt_wrapped_command(*outWrappedCmd, *outWrappedCmdSize,
                            auth1, session->transInternal.authData);
  }
  return TPM_SUCCESS;
}

extern TPM_RESULT tpm_sign(TPM_KEY_DATA *key, TPM_AUTH *auth, BOOL isInfo,
  BYTE *areaToSign, UINT32 areaToSignSize, BYTE **sig, UINT32 *sigSize);

TPM_RESULT TPM_ReleaseTransportSigned(TPM_KEY_HANDLE keyHandle,
                                      TPM_NONCE *antiReplay,
                                      TPM_AUTH *auth1, TPM_AUTH *auth2,
                                      TPM_MODIFIER_INDICATOR *locality,
                                      TPM_CURRENT_TICKS *currentTicks,
                                      UINT32 *signSize, BYTE **signature)
{
  TPM_RESULT res;
  TPM_KEY_DATA *key;
  TPM_SESSION_DATA *session;
  BYTE buf[30 + 20];
  info("TPM_ReleaseTransportSigned()");
  /* get transport session and key */
  key = tpm_get_key(keyHandle);
  if (key == NULL) return TPM_INVALID_KEYHANDLE;
  if (key->sigScheme != TPM_SS_RSASSAPKCS1v15_SHA1) return TPM_INAPPROPRIATE_SIG;
  session = tpm_get_transport(auth2->authHandle);
  if (session == NULL) return TPM_INVALID_AUTHHANDLE;
  /* verify authorization */ 
  if (auth1->authHandle != TPM_INVALID_HANDLE
      || key->authDataUsage != TPM_AUTH_NEVER) {
    res = tpm_verify_auth(auth1, key->usageAuth, keyHandle);
    if (res != TPM_SUCCESS) return res;
  }
  res = tpm_verify_auth(auth2, session->transInternal.authData, TPM_INVALID_HANDLE);
  if (res != TPM_SUCCESS) return (res == TPM_AUTHFAIL) ? TPM_AUTH2FAIL : res;
  /* invalidate transport session */
  auth1->continueAuthSession = FALSE;
  /* logging must be enabled */
  if (!(session->transInternal.transPublic.transAttributes & TPM_TRANSPORT_LOG))
    return TPM_BAD_MODE;
  *locality = tpmData.stany.flags.localityModifier;
  memcpy(currentTicks, &tpmData.stany.data.currentTicks, sizeof(TPM_CURRENT_TICKS));
  transport_log_out(currentTicks, auth1->digest, *locality,
    &session->transInternal.transDigest);
  /* setup a TPM_SIGN_INFO structure */
  memcpy(&buf[0], (uint8_t*)"\x05\x00TRAN", 6);
  memcpy(&buf[6], antiReplay->nonce, 20);
  memcpy(&buf[26], (uint8_t*)"\x00\x00\x00\x14", 4);
  memcpy(&buf[30], session->transInternal.transDigest.digest, 20);
  /* sign info structure */ 
  res = tpm_sign(key, auth1, TRUE, buf, sizeof(buf), signature, signSize);
  return res;
}
