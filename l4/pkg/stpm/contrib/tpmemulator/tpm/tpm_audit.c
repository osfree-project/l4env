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
 * $Id: tpm_audit.c 139 2006-11-10 16:09:00Z mast $
 */

#include "tpm_emulator.h"
#include "tpm_commands.h"
#include "tpm_data.h"
#include "tpm_marshalling.h"
#include "tpm_handles.h"
#include <crypto/sha1.h>

/*
 * Auditing ([TPM_Part3], Section 8)
 * The TPM generates an audit event in response to the TPM executing a 
 * function that has the audit flag set to TRUE for that function. The 
 * TPM maintains an extended value for all audited operations. 
 */
 
#define AUDIT_STATUS tpmData.permanent.data.ordinalAuditStatus

void tpm_audit_request(TPM_COMMAND_CODE ordinal, TPM_REQUEST *req)
{
  tpm_sha1_ctx_t sha1_ctx;
  BYTE buf[sizeof_TPM_AUDIT_EVENT_IN(x)], *ptr;
  UINT32 len;
  TPM_COMMAND_CODE ord = ordinal & TPM_ORD_INDEX_MASK;
  if (ord < TPM_ORD_MAX
      && (AUDIT_STATUS[ord / 8] & (1 << (ord & 0x07)))) {
    info("tpm_audit_request()");
    /* is there already an audit session running? */
    if (!tpmData.stany.data.auditSession) {       
      tpmData.stany.data.auditSession = TRUE;
      tpmData.permanent.data.auditMonotonicCounter++;
    }
    /* update audit digest */
    ptr = buf; len = sizeof(buf);
    tpm_marshal_TPM_TAG(&ptr, &len, TPM_TAG_AUDIT_EVENT_IN);
    tpm_marshal_TPM_COMMAND_CODE(&ptr, &len, ordinal);
    tpm_sha1_init(&sha1_ctx);
    tpm_sha1_update(&sha1_ctx, req->param, req->paramSize);
    tpm_sha1_final(&sha1_ctx, ptr);
    ptr += 20; len -= 20;
    tpm_marshal_TPM_TAG(&ptr, &len, TPM_TAG_COUNTER_VALUE);
    tpm_marshal_UINT32(&ptr, &len, 0);
    tpm_marshal_UINT32(&ptr, &len, tpmData.permanent.data.auditMonotonicCounter);
    tpm_sha1_init(&sha1_ctx);
    tpm_sha1_update(&sha1_ctx, tpmData.stany.data.auditDigest.digest, sizeof(TPM_DIGEST));
    tpm_sha1_update(&sha1_ctx, buf, sizeof(buf));
    tpm_sha1_final(&sha1_ctx, tpmData.stany.data.auditDigest.digest);
  }
}

void tpm_audit_response(TPM_COMMAND_CODE ordinal, TPM_RESPONSE *rsp)
{
  tpm_sha1_ctx_t sha1_ctx;
  BYTE buf[sizeof_TPM_AUDIT_EVENT_OUT()], *ptr;
  UINT32 len;
  TPM_COMMAND_CODE ord = ordinal & TPM_ORD_INDEX_MASK;
  if (ord < TPM_ORD_MAX
      && (AUDIT_STATUS[ord / 8] & (1 << (ord & 0x07)))) {
    info("tpm_audit_response()");
    /* update audit digest */
    ptr = buf; len = sizeof(buf);
    tpm_marshal_TPM_TAG(&ptr, &len, TPM_TAG_AUDIT_EVENT_OUT);
    tpm_marshal_TPM_COMMAND_CODE(&ptr, &len, ordinal);
    tpm_sha1_init(&sha1_ctx);
    tpm_sha1_update(&sha1_ctx, rsp->param, rsp->paramSize);
    tpm_sha1_final(&sha1_ctx, ptr);
    ptr += 20; len -= 20;
    tpm_marshal_TPM_TAG(&ptr, &len, TPM_TAG_COUNTER_VALUE);
    tpm_marshal_UINT32(&ptr, &len, 0);
    tpm_marshal_UINT32(&ptr, &len, tpmData.permanent.data.auditMonotonicCounter);
    tpm_marshal_TPM_RESULT(&ptr, &len, rsp->result);
    tpm_sha1_init(&sha1_ctx);
    tpm_sha1_update(&sha1_ctx, tpmData.stany.data.auditDigest.digest, sizeof(TPM_DIGEST));
    tpm_sha1_update(&sha1_ctx, buf, sizeof(buf));
    tpm_sha1_final(&sha1_ctx, tpmData.stany.data.auditDigest.digest);
  }
}

static uint8_t bits[] = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 }; 

TPM_RESULT TPM_GetAuditDigest(UINT32 startOrdinal, 
                              TPM_COUNTER_VALUE *counterValue, 
                              TPM_DIGEST *auditDigest, BOOL *more,
                              UINT32 *ordSize, UINT32 **ordList)
{
  UINT32 i, j, len, *ptr;
  info("TPM_GetAuditDigest()");
  /* compute (maximal) size of the ordinal list */
  for (len = 0, i = startOrdinal/8; i < TPM_ORD_MAX/8; i++) {
    len += bits[AUDIT_STATUS[i] & 0x0f];
    len += bits[(AUDIT_STATUS[i] >> 4) & 0x0f];  
  }
  /* setup ordinal list */
  ptr = *ordList = tpm_malloc(len);
  if (ptr == NULL) return TPM_FAIL;
  for (*ordSize = 0, i = startOrdinal/8; i < TPM_ORD_MAX/8; i++) {
    if (AUDIT_STATUS[i]) for (j = 0; j < 8; j++) {
      if ((AUDIT_STATUS[i] & (1 << j)) && i * 8 + j > startOrdinal) {
        *ptr++ = i * 8 + j;
        *ordSize += 4;
      }      
    } 
  }
  counterValue->tag = TPM_TAG_COUNTER_VALUE;
  memset(counterValue->label, 0, sizeof(counterValue->label));
  counterValue->counter = tpmData.permanent.data.auditMonotonicCounter;
  memcpy(auditDigest, &tpmData.stany.data.auditDigest, sizeof(TPM_DIGEST));
  *more = FALSE;
  return TPM_SUCCESS;
}

extern TPM_RESULT tpm_sign(TPM_KEY_DATA *key, TPM_AUTH *auth, BOOL isInfo,
  BYTE *areaToSign, UINT32 areaToSignSize, BYTE **sig, UINT32 *sigSize);

TPM_RESULT TPM_GetAuditDigestSigned(TPM_KEY_HANDLE keyHandle, 
                                    UINT32 startOrdinal, BOOL closeAudit,
                                    TPM_NONCE *antiReplay, TPM_AUTH *auth1,  
                                    TPM_COUNTER_VALUE *counterValue,
                                    TPM_DIGEST *auditDigest, BOOL *more,
                                    UINT32 *ordSize, UINT32 **ordinalList,
                                    UINT32 *sigSize, BYTE **sig)
{
  TPM_RESULT res;
  TPM_KEY_DATA *key;
  BYTE *buf, *ptr; 
  UINT32 buf_size, len;
  info("TPM_GetAuditDigestSigned()");
  /* get key */
  key = tpm_get_key(keyHandle);
  if (key == NULL) return TPM_INVALID_KEYHANDLE;
  /* verify authorization */ 
  if (auth1->authHandle != TPM_INVALID_HANDLE
      || key->authDataUsage != TPM_AUTH_NEVER) {
    res = tpm_verify_auth(auth1, key->usageAuth, keyHandle);
    if (res != TPM_SUCCESS) return res;
  }
  /* get audit digest */    
  res = TPM_GetAuditDigest(startOrdinal, counterValue, auditDigest, 
                           more, ordSize, ordinalList);
  if (res != TPM_SUCCESS) return res;
  /* setup a TPM_SIGN_INFO structure */
  buf_size = 30 + 20 + sizeof_TPM_COUNTER_VALUE((*counterValue)) + *ordSize;
  buf = tpm_malloc(buf_size);
  if (buf == NULL) {
    tpm_free(*ordinalList);
    return TPM_FAIL; 
  }
  memcpy(&buf[0], "\x05\x00ADIG", 6);
  memcpy(&buf[6], antiReplay->nonce, 20);
  ptr = &buf[26]; len = buf_size - 26;
  tpm_marshal_UINT32(&ptr, &len, buf_size - 30);
  memcpy(ptr, auditDigest->digest, 20);
  ptr += 20; len -= 20;
  if (tpm_marshal_TPM_COUNTER_VALUE(&ptr, &len, counterValue)
      || tpm_marshal_UINT32_ARRAY(&ptr, &len, *ordinalList, *ordSize/4)) {
    tpm_free(*ordinalList);
    tpm_free(buf);
    return TPM_FAIL;
  }  
  /* check key usage */
  if (closeAudit && key->keyUsage == TPM_KEY_IDENTITY) {
    memset(&tpmData.stany.data.auditDigest, 0, sizeof(TPM_DIGEST));
    tpmData.stany.data.auditSession = FALSE;
  } else {
    return TPM_INVALID_KEYUSAGE;
  } 
  res = tpm_sign(key, auth1, TRUE, buf, buf_size, sig, sigSize);
  tpm_free(buf);
  return TPM_SUCCESS;
}

TPM_RESULT TPM_SetOrdinalAuditStatus(TPM_COMMAND_CODE ordinalToAudit,
                                     BOOL auditState, TPM_AUTH *auth1)
{
  TPM_RESULT res;
  info("TPM_SetOrdinalAuditStatus()");
  /* verify authorization */
  res = tpm_verify_auth(auth1, tpmData.permanent.data.ownerAuth, TPM_KH_OWNER);
  if (res != TPM_SUCCESS) return res;
  /* set ordinal's audit status */
  if (ordinalToAudit > TPM_ORD_MAX) return TPM_BADINDEX;
  ordinalToAudit &= TPM_ORD_INDEX_MASK;
  if (auditState) {
    AUDIT_STATUS[ordinalToAudit / 8] |= (1 << (ordinalToAudit & 0x07));
  } else {
    AUDIT_STATUS[ordinalToAudit / 8] &= ~(1 << (ordinalToAudit & 0x07)); 
  }
  return TPM_SUCCESS;
}

