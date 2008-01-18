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
 * $Id: tpm_deprecated.c 144 2006-11-11 23:56:11Z mast $
 */

#include "tpm_emulator.h"
#include "tpm_commands.h"
#include "tpm_data.h"
#include "tpm_handles.h"
#include "tpm_marshalling.h"

#define SAVE_KEY_CONTEXT_LABEL  ((uint8_t*)"SaveKeyContext..")
#define SAVE_AUTH_CONTEXT_LABEL ((uint8_t*)"SaveAuthContext.")

/*
 * Deprecated commands ([TPM_Part3], Section 28)
 * This section covers the commands that were in version 1.1 but now have 
 * new functionality in other functions. The deprecated commands are still 
 * available in 1.2 but all new software should use the new functionality. 
 * There is no requirement that the deprecated commands work with new 
 * structures.
 */

TPM_RESULT TPM_EvictKey(TPM_KEY_HANDLE evictHandle)
{
  info("TPM_EvictKey()");
  return TPM_FlushSpecific(evictHandle, TPM_RT_KEY);
}

TPM_RESULT TPM_Terminate_Handle(TPM_AUTHHANDLE handle)
{
  info("TPM_Terminate_Handle()");
  return TPM_FlushSpecific(handle, TPM_RT_AUTH);
}

TPM_RESULT TPM_SaveKeyContext(TPM_KEY_HANDLE keyHandle,  
                              UINT32 *keyContextSize, BYTE **keyContextBlob)
{
  TPM_RESULT res;
  TPM_CONTEXT_BLOB contextBlob;
  BYTE *ptr;
  UINT32 len;
  info("TPM_SaveKeyContext()");
  res = TPM_SaveContext(keyHandle, TPM_RT_KEY, SAVE_KEY_CONTEXT_LABEL,
                        keyContextSize, &contextBlob);
  if (res != TPM_SUCCESS) return res;
  len = *keyContextSize;
  *keyContextBlob = ptr = tpm_malloc(len);
  if (ptr == NULL
      || tpm_marshal_TPM_CONTEXT_BLOB(&ptr, &len, &contextBlob)) res = TPM_FAIL;
  else res = TPM_SUCCESS;
  free_TPM_CONTEXT_BLOB(contextBlob);
  return res;
}

TPM_RESULT TPM_LoadKeyContext(UINT32 keyContextSize,
                              BYTE *keyContextBlob, TPM_KEY_HANDLE *keyHandle)
{
  TPM_CONTEXT_BLOB contextBlob;
  UINT32 len = keyContextSize;
  info("TPM_LoadKeyContext()");
  if (tpm_unmarshal_TPM_CONTEXT_BLOB(&keyContextBlob, 
      &len, &contextBlob)) return TPM_FAIL;
  return TPM_LoadContext(FALSE, TPM_INVALID_HANDLE, keyContextSize, 
                         &contextBlob, keyHandle);
}

TPM_RESULT TPM_SaveAuthContext(TPM_AUTHHANDLE authHandle,  
                               UINT32 *authContextSize, BYTE **authContextBlob)
{
  TPM_RESULT res;
  TPM_CONTEXT_BLOB contextBlob;
  BYTE *ptr;
  UINT32 len;
  info("TPM_SaveAuthContext()");
  res = TPM_SaveContext(authHandle, TPM_RT_KEY, SAVE_AUTH_CONTEXT_LABEL,
                        authContextSize, &contextBlob);
  if (res != TPM_SUCCESS) return res;
  len = *authContextSize;
  *authContextBlob = ptr = tpm_malloc(len);
  if (ptr == NULL
      || tpm_marshal_TPM_CONTEXT_BLOB(&ptr, &len, &contextBlob)) res = TPM_FAIL;
  else res = TPM_SUCCESS;
  free_TPM_CONTEXT_BLOB(contextBlob);
  return res;
}

TPM_RESULT TPM_LoadAuthContext(UINT32 authContextSize, BYTE *authContextBlob, 
                               TPM_KEY_HANDLE *authHandle)
{
  TPM_CONTEXT_BLOB contextBlob;
  UINT32 len = authContextSize;
  info("TPM_LoadAuthContext()");
  if (tpm_unmarshal_TPM_CONTEXT_BLOB(&authContextBlob, 
      &len, &contextBlob)) return TPM_FAIL;
  return TPM_LoadContext(FALSE, TPM_INVALID_HANDLE, authContextSize, 
                         &contextBlob, authHandle);
}

TPM_RESULT TPM_DirWriteAuth(TPM_DIRINDEX dirIndex, 
                            TPM_DIRVALUE *newContents, TPM_AUTH *auth1)
{
  TPM_RESULT res;
  info("TPM_DirWriteAuth()");
  res = tpm_verify_auth(auth1, tpmData.permanent.data.ownerAuth, TPM_KH_OWNER);
  if (res != TPM_SUCCESS) return res;
  if (dirIndex != 1) return TPM_BADINDEX;
  memcpy(&tpmData.permanent.data.DIR, newContents, sizeof(TPM_DIRVALUE));
  return TPM_SUCCESS;
}

TPM_RESULT TPM_DirRead(TPM_DIRINDEX dirIndex, TPM_DIRVALUE *dirContents)
{
  info("TPM_DirRead()");
  if (dirIndex != 1) return TPM_BADINDEX;
  memcpy(dirContents, &tpmData.permanent.data.DIR, sizeof(TPM_DIRVALUE));
  return TPM_SUCCESS;
}

TPM_RESULT TPM_ChangeAuthAsymStart(  
  TPM_KEY_HANDLE idHandle,
  TPM_NONCE *antiReplay,
  TPM_KEY_PARMS *inTempKey,
  TPM_AUTH *auth1,  
  TPM_CERTIFY_INFO *certifyInfo,
  UINT32 *sigSize,
  BYTE **sig ,
  TPM_KEY_HANDLE *ephHandle,
  TPM_KEY *outTempKey 
)
{
  info("TPM_ChangeAuthAsymStart() not implemented yet");
  /* TODO: implement TPM_ChangeAuthAsymStart() */
  return TPM_FAIL;
}

TPM_RESULT TPM_ChangeAuthAsymFinish(  
  TPM_KEY_HANDLE parentHandle,
  TPM_KEY_HANDLE ephHandle,
  TPM_ENTITY_TYPE entityType,
  TPM_HMAC *newAuthLink,
  UINT32 newAuthSize,
  BYTE *encNewAuth,
  UINT32 encDataSize,
  BYTE *encData,
  TPM_AUTH *auth1,  
  UINT32 *outDataSize,
  BYTE **outData ,
  TPM_NONCE *saltNonce,
  TPM_DIGEST *changeProof 
)
{
  info("TPM_ChangeAuthAsymFinish() not implemented yet");
  /* TODO: implement TPM_ChangeAuthAsymFinish() */
  return TPM_FAIL;
}

TPM_RESULT TPM_Reset()
{
  int i;
  info("TPM_Reset()");
  /* invalidate all authorization sessions */
  for (i = 0; i < TPM_MAX_SESSIONS; i++) {
    TPM_SESSION_DATA *session = &tpmData.stany.data.sessions[i]; 
    if (session->type == TPM_ST_OIAP || session->type == TPM_ST_OSAP)
      memset(session, 0, sizeof(*session));
  }
  /* TODO: invalidate AuthContextSave structures */
  return TPM_SUCCESS;
}

extern TPM_RESULT tpm_sign(TPM_KEY_DATA *key, TPM_AUTH *auth, BOOL isInfo,
  BYTE *areaToSign, UINT32 areaToSignSize, BYTE **sig, UINT32 *sigSize);

TPM_RESULT TPM_CertifySelfTest(TPM_KEY_HANDLE keyHandle, TPM_NONCE *antiReplay,
                               TPM_AUTH *auth1, UINT32 *sigSize, BYTE **sig)
{
  TPM_RESULT res;
  TPM_KEY_DATA *key;
  BYTE buf[35];
  info("TPM_CertifySelfTest()");
  key = tpm_get_key(keyHandle);
  if (key == NULL) return TPM_INVALID_KEYHANDLE;
  /* perform self test */
  res = TPM_SelfTestFull();
  if (res != TPM_SUCCESS) return res;
  /* verify authorization */ 
  if (auth1->authHandle != TPM_INVALID_HANDLE
      || key->authDataUsage != TPM_AUTH_NEVER) {
    res = tpm_verify_auth(auth1, key->usageAuth, keyHandle);
    if (res != TPM_SUCCESS) return res;
  }
  if (key->keyUsage != TPM_KEY_SIGNING && key->keyUsage != TPM_KEY_LEGACY
      && key->keyUsage != TPM_KEY_IDENTITY) return TPM_INVALID_KEYUSAGE;
  /* not neccessary, because a vendor specific signature is allowed
  if (key->sigScheme != TPM_SS_RSASSAPKCS1v15_SHA1)
    return TPM_BAD_SCHEME;
  */
  /* setup and sign result */
  memcpy(&buf, "Test Passed", 11);
  memcpy(&buf[11], antiReplay->nonce, sizeof(TPM_NONCE));
  memcpy(&buf[31], "\x52\x00\x00\x00", 4);
  return tpm_sign(key, auth1, FALSE, buf, sizeof(buf), sig, sigSize);
}

extern TPM_RESULT tpm_get_pubek(TPM_PUBKEY *pubEndorsementKey);

TPM_RESULT TPM_OwnerReadPubek(TPM_AUTH *auth1, TPM_PUBKEY *pubEndorsementKey)
{
  TPM_RESULT res;
  info("TPM_OwnerReadPubek()");
  /* verify authorization */
  res = tpm_verify_auth(auth1, tpmData.permanent.data.ownerAuth, TPM_KH_OWNER);
  if (res != TPM_SUCCESS) return res;
  res = tpm_get_pubek(pubEndorsementKey);
  if (res != TPM_SUCCESS) return res; 
  return TPM_SUCCESS;
}

