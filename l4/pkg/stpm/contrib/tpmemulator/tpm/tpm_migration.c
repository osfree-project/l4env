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
 * $Id: tpm_migration.c 300 2008-10-13 15:59:14Z mast $
 */

#include "tpm_emulator.h"
#include "tpm_commands.h"
#include "tpm_handles.h"
#include "tpm_data.h"
#include "tpm_marshalling.h"
#include "crypto/sha1.h"

/*
 * Migration ([TPM_Part3], Section 11)
 */

extern int tpm_decrypt_private_key(TPM_KEY_DATA *key, BYTE *enc, UINT32 enc_size, 
                                   TPM_STORE_ASYMKEY *store, BYTE **buf);

int tpm_compute_migration_digest(TPM_MIGRATIONKEYAUTH *migrationKeyAuth,
                                 TPM_NONCE *tpmProof, TPM_DIGEST *digest)
{
  tpm_sha1_ctx_t sha1;
  UINT32 len = sizeof_TPM_PUBKEY(migrationKeyAuth->migrationKey);
  BYTE *buf, *ptr, buf2[2];
  buf = ptr = tpm_malloc(len);
  if (buf == NULL
      || tpm_marshal_TPM_PUBKEY(&ptr, &len, &migrationKeyAuth->migrationKey)) {
    tpm_free(buf);
    return -1;
  }
  /* compute SHA1 hash */
  tpm_sha1_init(&sha1);
  tpm_sha1_update(&sha1, buf, sizeof_TPM_PUBKEY(migrationKeyAuth->migrationKey));
  ptr = buf2; len = 2;
  tpm_marshal_UINT16(&ptr, &len, migrationKeyAuth->migrationScheme);
  tpm_sha1_update(&sha1, buf2, 2);
  tpm_sha1_update(&sha1, tpmProof->nonce, sizeof(TPM_NONCE));
  tpm_sha1_final(&sha1, digest->digest);
  tpm_free(buf);
  return 0;
}

int tpm_verify_migration_digest(TPM_MIGRATIONKEYAUTH *migrationKeyAuth,
                                TPM_NONCE *tpmProof)
{
  TPM_DIGEST digest;
  if (tpm_compute_migration_digest(migrationKeyAuth, tpmProof, &digest)) return -1;
  return memcmp(digest.digest, migrationKeyAuth->digest.digest, sizeof(TPM_DIGEST));
}

TPM_RESULT TPM_CreateMigrationBlob(TPM_KEY_HANDLE parentHandle,
                                   TPM_MIGRATE_SCHEME migrationType,
                                   TPM_MIGRATIONKEYAUTH *migrationKeyAuth,
                                   UINT32 encDataSize, BYTE *encData,
                                   TPM_AUTH *auth1, TPM_AUTH *auth2,
                                   UINT32 *randomSize, BYTE **random,
                                   UINT32 *outDataSize, BYTE **outData)
{
  TPM_RESULT res;
  TPM_KEY_DATA *parent;
  TPM_SESSION_DATA *session;
  BYTE *key_buf;
  TPM_STORE_ASYMKEY store;
  TPM_KEY_DATA key;
  info("TPM_CreateMigrationBlob()");
  /* get parent key */
  parent = tpm_get_key(parentHandle);
  if (parent == NULL) return TPM_INVALID_KEYHANDLE;
  /* verify parent authorization */
  res = tpm_verify_auth(auth1, parent->usageAuth, parentHandle);
  if (res != TPM_SUCCESS) return res;
  session = tpm_get_auth(auth2->authHandle);
  if (session == NULL || session->type != TPM_ST_OIAP) TPM_AUTHFAIL;
  /* verify key properties */
  if (parent->keyUsage != TPM_KEY_STORAGE) return TPM_INVALID_KEYUSAGE;
  /* decrypt private key */
  if (tpm_decrypt_private_key(parent, encData, encDataSize, &store, &key_buf)
      || store.payload != TPM_PT_ASYM) {
    tpm_free(key_buf);
    return TPM_DECRYPT_ERROR;
  }
  /* verify migration authorization */
  res = tpm_verify_auth(auth2, store.migrationAuth, TPM_INVALID_HANDLE);
  if (res != TPM_SUCCESS) {
    tpm_free(key_buf);
    return TPM_MIGRATEFAIL;
  }
  if (tpm_verify_migration_digest(migrationKeyAuth,
      &tpmData.permanent.data.tpmProof)) {
    debug("tpm_verify_migration_digest() failed");
    tpm_free(key_buf);
    return TPM_MIGRATEFAIL;
  }
  if (migrationType == TPM_MS_REWRAP) {
    /*
    *encDataSize = key->key.size >> 3;
    *encData = tpm_malloc(*encDataSize);
    
    if (tpm_encrypt_private_key(key, &store, *encData, encDataSize)) {
      tpm_free(*encData);
      tpm_free(key_buf);
      return TPM_ENCRYPT_ERROR;
    }
    random = NULL;
    randomSize = 0;
    tpm_free(key_buf);
    return TPM_SUCCESS;
    */
  } else if (migrationType == TPM_MS_MIGRATE) {
    //TODO: TPM_MS_MIGRATE
  }
  tpm_free(key_buf);
  return TPM_BAD_PARAMETER;
}

TPM_RESULT TPM_ConvertMigrationBlob(
  TPM_KEY_HANDLE parentHandle,
  UINT32 inDataSize,
  BYTE *inData,
  UINT32 randomSize,
  BYTE *random,
  TPM_AUTH *auth1,
  UINT32 *outDataSize,
  BYTE **outData
)
{
  info("TPM_ConvertMigrationBlob() not implemented yet");
  /* TODO: implement TPM_ConvertMigrationBlob() */
  return TPM_FAIL;
}

TPM_RESULT TPM_AuthorizeMigrationKey(
  TPM_MIGRATE_SCHEME migrateScheme,
  TPM_PUBKEY *migrationKey,
  TPM_AUTH *auth1,
  TPM_MIGRATIONKEYAUTH *outData
)
{
  info("TPM_AuthorizeMigrationKey() not implemented yet");
  /* TODO: implement TPM_AuthorizeMigrationKey() */
  return TPM_FAIL;
}

TPM_RESULT TPM_MigrateKey(
  TPM_KEY_HANDLE maKeyHandle,
  TPM_PUBKEY *pubKey,
  UINT32 inDataSize,
  BYTE *inData,
  TPM_AUTH *auth1,
  UINT32 *outDataSize,
  BYTE **outData
)
{
  info("TPM_MigrateKey() not implemented yet");
  /* TODO: implement TPM_MigrateKey() */
  return TPM_FAIL;
}

TPM_RESULT TPM_CMK_SetRestrictions(
  TPM_CMK_DELEGATE restriction,
  TPM_AUTH *auth1
)
{
  info("TPM_CMK_SetRestrictions() not implemented yet");
  /* TODO: implement TPM_CMK_SetRestrictions() */
  return TPM_FAIL;
}

TPM_RESULT TPM_CMK_ApproveMA(
  TPM_DIGEST *migrationAuthorityDigest,
  TPM_AUTH *auth1,
  TPM_HMAC *outData
)
{
  info("TPM_CMK_ApproveMA() not implemented yet");
  /* TODO: implement TPM_CMK_ApproveMA() */
  return TPM_FAIL;
}

TPM_RESULT TPM_CMK_CreateKey(
  TPM_KEY_HANDLE parentHandle,
  TPM_ENCAUTH *dataUsageAuth,
  TPM_KEY *keyInfo,
  TPM_DIGEST *migrationAuthorityDigest,
  TPM_AUTH *auth1,
  TPM_AUTH *auth2,
  TPM_KEY *wrappedKey
)
{
  info("TPM_CMK_CreateKey() not implemented yet");
  /* TODO: implement TPM_CMK_CreateKey() */
  return TPM_FAIL;
}

TPM_RESULT TPM_CMK_CreateTicket(
  TPM_PUBKEY *verificationKey,
  TPM_DIGEST *signedData,
  UINT32 signatureValueSize,
  BYTE *signatureValue,
  TPM_AUTH *auth1,
  TPM_DIGEST *sigTicket
)
{
  info("TPM_CMK_CreateTicket() not implemented yet");
  /* TODO: implement TPM_CMK_CreateTicket() */
  return TPM_FAIL;
}

TPM_RESULT TPM_CMK_CreateBlob(
  TPM_KEY_HANDLE parentHandle,
  TPM_MIGRATE_SCHEME migrationType,
  TPM_MIGRATIONKEYAUTH *migrationKeyAuth,
  TPM_DIGEST *pubSourceKeyDigest,
  UINT32 restrictTicketSize,
  BYTE *restrictTicket,
  UINT32 sigTicketSize,
  BYTE *sigTicket,
  UINT32 encDataSize,
  BYTE *encData,
  TPM_AUTH *auth1,
  UINT32 *randomSize,
  BYTE **random,
  UINT32 *outDataSize,
  BYTE **outData
)
{
  info("TPM_CMK_CreateBlob() not implemented yet");
  /* TODO: implement TPM_CMK_CreateBlob() */
  return TPM_FAIL;
}

TPM_RESULT TPM_CMK_ConvertMigration(
  TPM_KEY_HANDLE parentHandle,
  TPM_CMK_AUTH *restrictTicket,
  TPM_HMAC *sigTicket,
  TPM_KEY *migratedKey,
  UINT32 msaListSize,
  TPM_MSA_COMPOSITE *msaList,
  UINT32 randomSize,
  BYTE *random,
  TPM_AUTH *auth1,
  UINT32 *outDataSize,
  BYTE **outData
)
{
  info("TPM_CMK_ConvertMigration() not implemented yet");
  /* TODO: implement TPM_CMK_ConvertMigration() */
  return TPM_FAIL;
}
