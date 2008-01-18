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
 * $Id: tpm_migration.c 100 2006-05-09 22:58:41Z hstamer $
 */

#include "tpm_emulator.h"
#include "tpm_commands.h"

/*
 * Migration ([TPM_Part3], Section 11)
 */

TPM_RESULT TPM_CreateMigrationBlob(
  TPM_KEY_HANDLE parentHandle,
  TPM_MIGRATE_SCHEME migrationType,
  TPM_MIGRATIONKEYAUTH *migrationKeyAuth,
  UINT32 encDataSize,
  BYTE *encData,
  TPM_AUTH *auth1,
  TPM_AUTH *auth2,
  UINT32 *randomSize,
  BYTE **random,
  UINT32 *outDataSize,
  BYTE **outData
)
{
  info("TPM_CreateMigrationBlob() not implemented yet");
  /* TODO: implement TPM_CreateMigrationBlob() */
  return TPM_FAIL;
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
