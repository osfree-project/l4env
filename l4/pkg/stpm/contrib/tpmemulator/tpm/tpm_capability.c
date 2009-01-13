/* Software-Based Trusted Platform Module (TPM) Emulator for Linux
 * Copyright (C) 2004 Mario Strasser <mast@gmx.net>,
 *                    Swiss Federal Institute of Technology (ETH) Zurich,
 *               2006, 2008 Heiko Stamer <stamer@gaos.org>
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
 * $Id: tpm_capability.c 287 2008-03-22 12:12:47Z mast $
 */

#include "tpm_emulator.h"
#include "tpm_commands.h"
#include "tpm_marshalling.h"
#include "tpm_data.h"
#include "tpm_handles.h"

/*
 * The GetCapability Commands ([TPM_Part3], Section 7)
 * The GetCapability command allows the TPM to report back to the requester 
 * what type of TPM it is dealing with. The request for information requires 
 * the requester to specify which piece of information that is required. 
 */

static inline TPM_RESULT return_UINT32(UINT32 *respSize, BYTE **resp, UINT32 value)
{
  UINT32 len = *respSize = 4;
  BYTE *ptr = *resp = tpm_malloc(*respSize);
  if (ptr == NULL || tpm_marshal_UINT32(&ptr, &len, value)) {
    tpm_free(*resp);
    return TPM_FAIL;
  }
  return TPM_SUCCESS;
}

static inline TPM_RESULT return_BOOL(UINT32 *respSize, BYTE **resp, BOOL value)
{
  UINT32 len = *respSize = 1;
  BYTE *ptr = *resp = tpm_malloc(*respSize);
  if (ptr == NULL || tpm_marshal_BOOL(&ptr, &len, value)) {
    tpm_free(*resp);
    return TPM_FAIL;
  }
  return TPM_SUCCESS;
}

static TPM_RESULT cap_property(UINT32 subCapSize, BYTE *subCap, 
                               UINT32 *respSize, BYTE **resp)
{
  UINT32 i, j, property;

  if (tpm_unmarshal_UINT32(&subCap, &subCapSize, &property))
    return TPM_BAD_MODE;
  switch (property) {
    case TPM_CAP_PROP_PCR:
      debug("[TPM_CAP_PROP_PCR]");
      return return_UINT32(respSize, resp, TPM_NUM_PCR);

    case TPM_CAP_PROP_DIR:
      debug("[TPM_CAP_PROP_DIR]");
      return return_UINT32(respSize, resp, 1);

    case TPM_CAP_PROP_MANUFACTURER:
      debug("[TPM_CAP_PROP_MANUFACTURER]");
      return return_UINT32(respSize, resp, TPM_MANUFACTURER);

    case TPM_CAP_PROP_KEYS:
      debug("[TPM_CAP_PROP_KEYS]");
      for (i = 0, j = TPM_MAX_KEYS; i < TPM_MAX_KEYS; i++)
        if (tpmData.permanent.data.keys[i].valid) j--;
      return return_UINT32(respSize, resp, j); 

    case TPM_CAP_PROP_MIN_COUNTER:
      debug("[TPM_CAP_PROP_MIN_COUNTER]");
      return return_UINT32(respSize, resp, 1);

    case TPM_CAP_PROP_AUTHSESS:
      debug("[TPM_CAP_PROP_AUTHSESS]");
      for (i = 0, j = TPM_MAX_SESSIONS; i < TPM_MAX_SESSIONS; i++)
        if (tpmData.stany.data.sessions[i].type != TPM_ST_INVALID) j--;
      return return_UINT32(respSize, resp, j);

    case TPM_CAP_PROP_TRANSESS:
      debug("[TPM_CAP_PROP_TRANSESS]");
      for (i = 0, j = TPM_MAX_SESSIONS; i < TPM_MAX_SESSIONS; i++)
        if (tpmData.stany.data.sessions[i].type != TPM_ST_INVALID) j--;
      return return_UINT32(respSize, resp, j);

    case TPM_CAP_PROP_COUNTERS:
      debug("[TPM_CAP_PROP_COUNTERS]");
      for (i = 0, j = TPM_MAX_COUNTERS; i < TPM_MAX_COUNTERS; i++)
        if (tpmData.permanent.data.counters[i].valid) j--;
      return return_UINT32(respSize, resp, j);

    case TPM_CAP_PROP_MAX_AUTHSESS:
      debug("[TPM_CAP_PROP_MAX_AUTHSESS]");
      return return_UINT32(respSize, resp, TPM_MAX_SESSIONS);

    case TPM_CAP_PROP_MAX_TRANSESS:
      debug("[TPM_CAP_PROP_MAX_TRANSESS]");
      return return_UINT32(respSize, resp, TPM_MAX_SESSIONS);

    case TPM_CAP_PROP_MAX_COUNTERS:
      debug("[TPM_CAP_PROP_MAX_COUNTERS]");
      return return_UINT32(respSize, resp, TPM_MAX_COUNTERS);

    case TPM_CAP_PROP_MAX_KEYS:
      debug("[TPM_CAP_PROP_MAX_KEYS]");
      return return_UINT32(respSize, resp, TPM_MAX_KEYS);

    case TPM_CAP_PROP_OWNER:
      debug("[TPM_CAP_PROP_OWNER]");
      return return_BOOL(respSize, resp, tpmData.permanent.flags.owned);

    case TPM_CAP_PROP_CONTEXT:
      debug("[TPM_CAP_PROP_CONTEXT]");
      for (i = 0, j = 0; i < TPM_MAX_SESSION_LIST; i++)
        if (tpmData.stany.data.contextList[i] == 0) j++;
      return return_UINT32(respSize, resp, j);

    case TPM_CAP_PROP_MAX_CONTEXT:
      debug("[TPM_CAP_PROP_MAX_CONTEXT]");
      return return_UINT32(respSize, resp, TPM_MAX_SESSION_LIST);

    case TPM_CAP_PROP_FAMILYROWS:
      debug("[TPM_CAP_PROP_FAMILYROWS]");
      /* TODO: TPM_CAP_PROP_FAMILYROWS */
      return TPM_FAIL;

    case TPM_CAP_PROP_TIS_TIMEOUT:
      debug("[TPM_CAP_PROP_TIS_TIMEOUT]");
      /* TODO: TPM_CAP_PROP_TIS_TIMEOUT */
      return TPM_FAIL;

    case TPM_CAP_PROP_STARTUP_EFFECT:
      debug("[TPM_CAP_PROP_STARTUP_EFFECT]");
      return return_UINT32(respSize, resp, 0x4f);

    case TPM_CAP_PROP_DELEGATE_ROW:
      debug("[TPM_CAP_PROP_DELEGATE_ROW]");
      /* TODO: TPM_CAP_PROP_DELEGATE_ROW */
      return TPM_FAIL;

    case TPM_CAP_PROP_MAX_DAASESS:
      debug("[TPM_CAP_PROP_MAX_DAASESS]");
      return return_UINT32(respSize, resp, TPM_MAX_SESSIONS_DAA);

    case TPM_CAP_PROP_DAASESS:
      debug("[TPM_CAP_PROP_DAASESS]");
      for (i = 0, j = TPM_MAX_SESSIONS_DAA; i < TPM_MAX_SESSIONS_DAA; i++)
        if (tpmData.stany.data.sessionsDAA[i].type != TPM_ST_INVALID) j--;
      return return_UINT32(respSize, resp, j);

    case TPM_CAP_PROP_CONTEXT_DIST:
      debug("[TPM_CAP_PROP_CONTEXT_DIST]");
      /* TODO: TPM_CAP_PROP_CONTEXT_DIST */
      return TPM_FAIL;

    case TPM_CAP_PROP_DAA_INTERRUPT:
      debug("[TPM_CAP_PROP_DAA_INTERRUPT]");
      /* A value of TRUE indicates that the TPM will accept ANY command 
       * while executing a DAA Join or Sign. A value of FALSE indicates 
       * that the TPM will invalidate the DAA Join or Sign upon the 
       * receipt of any command other than the next join/sign in the 
       * session or a TPM_SaveContext. */
      return return_BOOL(respSize, resp, TRUE);

    case TPM_CAP_PROP_SESSIONS:
      debug("[TPM_CAP_PROP_SESSIONS]");
      for (i = 0, j = TPM_MAX_SESSIONS; i < TPM_MAX_SESSIONS; i++)
        if (tpmData.stany.data.sessions[i].type != TPM_ST_INVALID) j--;
      return return_UINT32(respSize, resp, j);

    case TPM_CAP_PROP_MAX_SESSIONS:
      debug("[TPM_CAP_PROP_MAX_SESSIONS]");
      return return_UINT32(respSize, resp, TPM_MAX_SESSIONS);

    case TPM_CAP_PROP_CMK_RESTRICTION:
      debug("[TPM_CAP_PROP_CMK_RESTRICTION]");
      /* TODO: TPM_CAP_PROP_CMK_RESTRICTION */
      return TPM_FAIL;

    case TPM_CAP_PROP_DURATION:
      debug("[TPM_CAP_PROP_DURATION]");
      /* TODO: TPM_CAP_PROP_DURATION */
      return TPM_FAIL;

    case TPM_CAP_PROP_ACTIVE_COUNTER:
      debug("[TPM_CAP_PROP_ACTIVE_COUNTER]");
      /* TODO: TPM_CAP_PROP_ACTIVE_COUNTER */
      return TPM_FAIL;

    case TPM_CAP_PROP_MAX_NV_AVAILABLE:
      debug("[TPM_CAP_PROP_MAX_NV_AVAILABLE]");
      return return_UINT32(respSize, resp, TPM_MAX_NV_SIZE
                           - tpmData.permanent.data.nvDataSize);

    case TPM_CAP_PROP_INPUT_BUFFER:
      debug("[TPM_CAP_PROP_INPUT_BUFFER]");
      /* TODO: TPM_CAP_PROP_INPUT_BUFFER */
      return TPM_FAIL;

    default:
      return TPM_BAD_MODE;
  }
}

/* changed since v1.2 rev 94: returned version MUST BE 1.1.0.0 */
static TPM_RESULT cap_version(UINT32 *respSize, BYTE **resp)
{
  UINT32 len = *respSize = 4;
  BYTE *ptr = *resp = tpm_malloc(*respSize);
  TPM_STRUCT_VER version;
  version.major = version.minor = 1;
  version.revMajor = version.revMinor = 0;
  if (ptr == NULL || tpm_marshal_TPM_STRUCT_VER(&ptr, &len, &version)) {
    tpm_free(*resp);
    return TPM_FAIL;
  }
  return TPM_SUCCESS;
}

/* manufacturer specific */
static TPM_RESULT cap_mfr(UINT32 subCapSize, BYTE *subCap,
                            UINT32 *respSize, BYTE **resp)
{
  UINT32 len = *respSize = 4, type;
  BYTE *ptr = *resp = tpm_malloc(*respSize);
  
  if (tpm_unmarshal_UINT32(&subCap, &subCapSize, &type))
    return TPM_BAD_MODE;
  
  switch (type) {
    /* TODO */
    
    default:
      if (ptr == NULL || tpm_marshal_TPM_VERSION(&ptr, &len, 
        &tpmData.permanent.data.version)) {
          tpm_free(*resp);
          return TPM_FAIL;
      }
      return TPM_SUCCESS;
  }
}

static TPM_RESULT cap_nv_list(UINT32 *respSize, BYTE **resp)
{
  UINT32 i, len;
  BYTE *ptr = *resp = tpm_malloc(TPM_MAX_NVS * sizeof(TPM_NV_INDEX));
  
  if (ptr == NULL) return TPM_FAIL;
  *respSize = 0;
  for (i = 0; i < TPM_MAX_NVS; i++) {
    if (tpmData.permanent.data.nvStorage[i].valid) {
      len = sizeof(TPM_NV_INDEX);
      ptr = (*resp) + *respSize;
      *respSize += len;
      if (tpm_marshal_UINT32(&ptr, &len, 
          tpmData.permanent.data.nvStorage[i].pubInfo.nvIndex)) {
        tpm_free(*resp);
        return TPM_FAIL;
      }
    }
  }
  return TPM_SUCCESS;
}

static TPM_RESULT cap_nv_index(UINT32 subCapSize, BYTE *subCap,
                               UINT32 *respSize, BYTE **resp)
{
  TPM_NV_INDEX nvIndex;
  TPM_NV_DATA_SENSITIVE *nv;
  UINT32 len;
  BYTE *ptr;

  if (tpm_unmarshal_TPM_NV_INDEX(&subCap, &subCapSize, &nvIndex))
    return TPM_BAD_MODE;
  nv = tpm_get_nvs(nvIndex);
  if (nv == NULL) return TPM_BADINDEX;
  len = *respSize = sizeof_TPM_NV_DATA_PUBLIC(nv->pubInfo);
  ptr = *resp = tpm_malloc(len);
  if (ptr == NULL 
      || tpm_marshal_TPM_NV_DATA_PUBLIC(&ptr, &len, &nv->pubInfo)) {
    tpm_free(*resp);
    return TPM_FAIL;
  }
  *respSize -= len;
  return TPM_SUCCESS;
}

static TPM_RESULT cap_handle(UINT32 subCapSize, BYTE *subCap,
                               UINT32 *respSize, BYTE **resp)
{
  UINT32 i, len, type;
  BYTE *ptr; 
  /* maximum of { TPM_MAX_KEYS, TPM_MAX_SESSIONS } */
  UINT32 list_size =
    (TPM_MAX_KEYS > TPM_MAX_SESSIONS) ? TPM_MAX_KEYS : TPM_MAX_SESSIONS;
  UINT32 handles[list_size];
  TPM_KEY_HANDLE_LIST list = { 0, handles };

  if (tpm_unmarshal_UINT32(&subCap, &subCapSize, &type))
    return TPM_BAD_MODE;
  switch (type) {
    case TPM_RT_KEY:
      debug("[TPM_RT_KEY]");
      for (i = 0; i < TPM_MAX_KEYS; i++)
        if (tpmData.permanent.data.keys[i].valid) {
          list.loaded++;
          list.handle[i] = INDEX_TO_KEY_HANDLE(i);
        }
      break;
    case TPM_RT_AUTH:
      debug("[TPM_RT_AUTH]");
      for (i = 0; i < TPM_MAX_SESSIONS; i++)
        if (tpmData.stany.data.sessions[i].type == TPM_ST_OIAP
            || tpmData.stany.data.sessions[i].type == TPM_ST_OSAP) {
          list.loaded++;
          list.handle[i] = INDEX_TO_AUTH_HANDLE(i);
        }
      break;
    case TPM_RT_TRANS:
      debug("[TPM_RT_TRANS]");
      for (i = 0; i < TPM_MAX_SESSIONS; i++)
        if (tpmData.stany.data.sessions[i].type == TPM_ST_TRANSPORT) {
          list.loaded++;
          list.handle[i] = INDEX_TO_TRANS_HANDLE(i);
        }
      break;
/* removed since v1.2 rev 94
    case TPM_RT_DELEGATE:
      debug("[TPM_RT_DELEGATE]");
      break;
*/
    case TPM_RT_COUNTER:
      debug("[TPM_RT_COUNTER]");
      for (i = 0; i < TPM_MAX_COUNTERS; i++)
        if (tpmData.permanent.data.counters[i].valid) {
          list.loaded++;
          list.handle[i] = INDEX_TO_COUNTER_HANDLE(i);
        }
      break;
    case TPM_RT_CONTEXT:
      /* TODO: implement TPM_CAP_HANDLE for TPM_RT_CONTEXT */
    default:
      return TPM_BAD_MODE;
  }
  /* marshal handle list */
  len = *respSize = 2 + list.loaded * 4;
  ptr = *resp = tpm_malloc(len);
  if (ptr == NULL || tpm_marshal_TPM_KEY_HANDLE_LIST(&ptr, &len, &list)) {
    tpm_free(*resp);
    return TPM_FAIL;
  }
  return TPM_SUCCESS;
}

static TPM_RESULT cap_ord(UINT32 subCapSize, BYTE *subCap,
                   UINT32 *respSize, BYTE **resp)
{
  TPM_COMMAND_CODE ord;
  if (tpm_unmarshal_TPM_COMMAND_CODE(&subCap, &subCapSize, &ord))
    return TPM_BAD_MODE;
  switch (ord) {
    case TPM_ORD_Init:
    case TPM_ORD_Startup:
    case TPM_ORD_SaveState:
    case TPM_ORD_SelfTestFull:
    case TPM_ORD_ContinueSelfTest:
    case TPM_ORD_GetTestResult:
    case TPM_ORD_SetOwnerInstall:
    case TPM_ORD_OwnerSetDisable:
    case TPM_ORD_PhysicalEnable:
    case TPM_ORD_PhysicalDisable:
    case TPM_ORD_PhysicalSetDeactivated:
    case TPM_ORD_SetTempDeactivated:
    case TPM_ORD_SetOperatorAuth:
    case TPM_ORD_TakeOwnership:
    case TPM_ORD_OwnerClear:
    case TPM_ORD_ForceClear:
    case TPM_ORD_DisableOwnerClear:
    case TPM_ORD_DisableForceClear:
    case TSC_ORD_PhysicalPresence:
    case TSC_ORD_ResetEstablishmentBit:
    case TPM_ORD_GetCapability:
/* WATCH: not yet implemented
    case TPM_ORD_SetCapability:
*/
    case TPM_ORD_GetCapabilityOwner:
    case TPM_ORD_GetAuditDigest:
    case TPM_ORD_GetAuditDigestSigned:
    case TPM_ORD_SetOrdinalAuditStatus:
    case TPM_ORD_FieldUpgrade:
    case TPM_ORD_SetRedirection:
    case TPM_ORD_ResetLockValue:
    case TPM_ORD_Seal:
    case TPM_ORD_Unseal:
    case TPM_ORD_UnBind:
    case TPM_ORD_CreateWrapKey:
    case TPM_ORD_LoadKey2:
    case TPM_ORD_GetPubKey:
    case TPM_ORD_Sealx:
/* WATCH: not yet implemented
    case TPM_ORD_CreateMigrationBlob:
    case TPM_ORD_ConvertMigrationBlob:
    case TPM_ORD_AuthorizeMigrationKey:
    case TPM_ORD_MigrateKey:
    case TPM_ORD_CMK_SetRestrictions:
    case TPM_ORD_CMK_ApproveMA:
    case TPM_ORD_CMK_CreateKey:
    case TPM_ORD_CMK_CreateTicket:
    case TPM_ORD_CMK_CreateBlob:
    case TPM_ORD_CMK_ConvertMigration:
*/
    case TPM_ORD_CreateMaintenanceArchive:
    case TPM_ORD_LoadMaintenanceArchive:
    case TPM_ORD_KillMaintenanceFeature:
    case TPM_ORD_LoadManuMaintPub:
    case TPM_ORD_ReadManuMaintPub:
    case TPM_ORD_SHA1Start:
    case TPM_ORD_SHA1Update:
    case TPM_ORD_SHA1Complete:
    case TPM_ORD_SHA1CompleteExtend:
    case TPM_ORD_Sign:
    case TPM_ORD_GetRandom:
    case TPM_ORD_StirRandom:
    case TPM_ORD_CertifyKey:
    case TPM_ORD_CertifyKey2:
    case TPM_ORD_CreateEndorsementKeyPair:
    case TPM_ORD_CreateRevocableEK:
    case TPM_ORD_RevokeTrust:
    case TPM_ORD_ReadPubek:
    case TPM_ORD_OwnerReadInternalPub:
    case TPM_ORD_MakeIdentity:
    case TPM_ORD_ActivateIdentity:
    case TPM_ORD_Extend:
    case TPM_ORD_PCRRead:
    case TPM_ORD_Quote:
    case TPM_ORD_PCR_Reset:
    case TPM_ORD_Quote2:
    case TPM_ORD_ChangeAuth:
    case TPM_ORD_ChangeAuthOwner:
    case TPM_ORD_OIAP:
    case TPM_ORD_OSAP:
/* WATCH: not yet implemented
    case TPM_ORD_DSAP:
*/
    case TPM_ORD_SetOwnerPointer:
/* WATCH: not yet implemented
    case TPM_ORD_Delegate_Manage:
    case TPM_ORD_Delegate_CreateKeyDelegation:
    case TPM_ORD_Delegate_CreateOwnerDelegation:
    case TPM_ORD_Delegate_LoadOwnerDelegation:
    case TPM_ORD_Delegate_ReadTable:
    case TPM_ORD_Delegate_UpdateVerification:
    case TPM_ORD_Delegate_VerifyDelegation:
*/
    case TPM_ORD_NV_DefineSpace:
    case TPM_ORD_NV_WriteValue:
    case TPM_ORD_NV_WriteValueAuth:
    case TPM_ORD_NV_ReadValue:
    case TPM_ORD_NV_ReadValueAuth:
    case TPM_ORD_KeyControlOwner:
    case TPM_ORD_SaveContext:
    case TPM_ORD_LoadContext:
    case TPM_ORD_FlushSpecific:
    case TPM_ORD_GetTicks:
    case TPM_ORD_TickStampBlob:
    case TPM_ORD_EstablishTransport:
    case TPM_ORD_ExecuteTransport:
    case TPM_ORD_ReleaseTransportSigned:
    case TPM_ORD_CreateCounter:
    case TPM_ORD_IncrementCounter:
    case TPM_ORD_ReadCounter:
    case TPM_ORD_ReleaseCounter:
    case TPM_ORD_ReleaseCounterOwner:
    case TPM_ORD_DAA_Join:
    case TPM_ORD_DAA_Sign:
    /* Deprecated but supported are the following commands */
    case TPM_ORD_EvictKey:
    case TPM_ORD_Terminate_Handle:
    case TPM_ORD_SaveKeyContext:
    case TPM_ORD_LoadKeyContext:
    case TPM_ORD_SaveAuthContext:
    case TPM_ORD_LoadAuthContext:
    case TPM_ORD_DirWriteAuth:
    case TPM_ORD_DirRead:
    case TPM_ORD_ChangeAuthAsymStart:
    case TPM_ORD_ChangeAuthAsymFinish:
    case TPM_ORD_Reset:
    case TPM_ORD_OwnerReadPubek:
    case TPM_ORD_DisablePubekRead:
    case TPM_ORD_LoadKey:
      return return_BOOL(respSize, resp, TRUE);
    default:
      return return_BOOL(respSize, resp, FALSE);
  }
}

static TPM_RESULT cap_alg(UINT32 subCapSize, BYTE *subCap,
                          UINT32 *respSize, BYTE **resp)
{
  TPM_ALGORITHM_ID id;
  if (tpm_unmarshal_TPM_ALGORITHM_ID(&subCap, &subCapSize, &id))
    return TPM_BAD_MODE;
  switch (id) {
    case TPM_ALG_RSA:
      return return_BOOL(respSize, resp, TRUE);
    default:
      return return_BOOL(respSize, resp, FALSE);
  }
}

static TPM_RESULT cap_pid(UINT32 subCapSize, BYTE *subCap,
                          UINT32 *respSize, BYTE **resp)
{
  TPM_PROTOCOL_ID id;
  if (tpm_unmarshal_TPM_PROTOCOL_ID(&subCap, &subCapSize, &id))
    return TPM_BAD_MODE;
  switch (id) {
    case TPM_PID_OIAP:
    case TPM_PID_OSAP:
    case TPM_PID_ADIP:
    case TPM_PID_ADCP:
    case TPM_PID_OWNER:
/* WATCH: not yet implemented
    case TPM_PID_DSAP:
*/
    case TPM_PID_TRANSPORT:
      return return_BOOL(respSize, resp, TRUE);
    default:
      return return_BOOL(respSize, resp, FALSE);
  }
}

static TPM_RESULT cap_flag(UINT32 subCapSize, BYTE *subCap,
                           UINT32 *respSize, BYTE **resp)
{
  UINT32 type, len;
  BYTE *ptr;
  if (tpm_unmarshal_UINT32(&subCap, &subCapSize, &type)) return TPM_BAD_MODE;
  switch (type) {
    case TPM_CAP_FLAG_PERMANENT:
      debug("[TPM_CAP_FLAG_PERMANENT");
      *respSize = len = sizeof_TPM_PERMANENT_FLAGS(tpmData.permanent.flags);
      *resp = ptr = tpm_malloc(len);
      if (tpm_marshal_TPM_PERMANENT_FLAGS(&ptr, &len, &tpmData.permanent.flags)) {
        tpm_free(*resp);
        return TPM_FAIL;
      }
      return TPM_SUCCESS;
    case TPM_CAP_FLAG_VOLATILE:
      debug("[TPM_CAP_FLAG_VOLATILE]");
      *respSize = len = sizeof_TPM_STCLEAR_FLAGS(tpmData.stclear.flags);
      *resp = ptr = tpm_malloc(len);
      if (tpm_marshal_TPM_STCLEAR_FLAGS(&ptr, &len, &tpmData.stclear.flags)) {
        tpm_free(*resp);
        return TPM_FAIL;
      }
      return TPM_SUCCESS;
/* removed since v1.2 rev 94
    case TPM_CAP_FLAG_STANY:
      debug("[TPM_CAP_FLAG_STANY]");
      *respSize = len = sizeof_TPM_STANY_FLAGS(tpmData.stany.flags);
      *resp = ptr = tpm_malloc(len);
      if (tpm_marshal_TPM_STANY_FLAGS(&ptr, &len, &tpmData.stany.flags)) {
        tpm_free(*resp);
        return TPM_FAIL;
      }
*/
      return TPM_SUCCESS;
    default:
      return TPM_BAD_MODE;
  }
}

static TPM_RESULT cap_loaded(UINT32 subCapSize, BYTE *subCap,
                             UINT32 *respSize, BYTE **resp)
{
  int i;
  BOOL free_space = FALSE;
  TPM_KEY_PARMS parms;
  if (tpm_unmarshal_TPM_KEY_PARMS(&subCap, &subCapSize, &parms))
    return TPM_BAD_MODE;
  for (i = 0; i < TPM_MAX_KEYS; i++) 
    if (!tpmData.permanent.data.keys[i].valid) free_space = TRUE;
  if (free_space
      && parms.algorithmID == TPM_ALG_RSA
      && parms.parms.rsa.keyLength <= 2048
      && parms.parms.rsa.numPrimes == 2) 
    return return_BOOL(respSize, resp, TRUE);
  return return_BOOL(respSize, resp, FALSE);
}

static TPM_RESULT cap_auth_encrypt(UINT32 subCapSize, BYTE *subCap,
                                   UINT32 *respSize, BYTE **resp)
{
  TPM_ALGORITHM_ID id;
  if (tpm_unmarshal_TPM_ALGORITHM_ID(&subCap, &subCapSize, &id))
    return TPM_BAD_MODE;
  switch (id) {
    case TPM_ALG_XOR:
      return return_BOOL(respSize, resp, TRUE);
    default:
      return return_BOOL(respSize, resp, FALSE);
  }
}

/* WATCH: added since v1.2 rev 94 */
TPM_RESULT cap_version_val(UINT32 *respSize, BYTE **resp)
{
  UINT32 len;
  BYTE *ptr;
  TPM_CAP_VERSION_INFO version;
  
  version.tag = TPM_TAG_CAP_VERSION_INFO;
  version.version = tpmData.permanent.data.version;
  version.specLevel = 0x0002; /* see [TPM_Part2], Section 21.6 */
  version.errataRev = 0x01;   /* 0x01 = rev 94, 0x02 = rev 103 */
  len = 4, ptr = version.tpmVendorID;
  if (tpm_marshal_UINT32(&ptr, &len, TPM_MANUFACTURER))
    return TPM_FAIL;
  version.vendorSpecificSize = 0;
  version.vendorSpecific = NULL;
  
  len = *respSize = sizeof_TPM_CAP_VERSION_INFO(version);
  ptr = *resp = tpm_malloc(*respSize);
  
  if (ptr == NULL || tpm_marshal_TPM_CAP_VERSION_INFO(&ptr, &len, &version)) {
    tpm_free(*resp);
    return TPM_FAIL;
  }
  return TPM_SUCCESS;
}

TPM_RESULT TPM_GetCapability(TPM_CAPABILITY_AREA capArea, UINT32 subCapSize, 
                             BYTE *subCap, UINT32 *respSize, BYTE **resp)
{
  info("TPM_GetCapability() (not fully implemented yet)");
  switch (capArea) {

    case TPM_CAP_ORD:
      debug("[TPM_CAP_ORD]");
      return cap_ord(subCapSize, subCap, respSize, resp);

    case TPM_CAP_ALG:
      debug("[TPM_CAP_ALG]");
      return cap_alg(subCapSize, subCap, respSize, resp);

    case TPM_CAP_PID:
      debug("[TPM_CAP_PID]");
      return cap_pid(subCapSize, subCap, respSize, resp);

    case TPM_CAP_FLAG:
      debug("[TPM_CAP_FLAG]");
      return cap_flag(subCapSize, subCap, respSize, resp);

    case TPM_CAP_PROPERTY:
      debug("[TPM_CAP_PROPERTY]");
      return cap_property(subCapSize, subCap, respSize, resp);

    case TPM_CAP_VERSION:
      debug("[TPM_CAP_VERSION]");
      return cap_version(respSize, resp);

    case TPM_CAP_KEY_HANDLE:
      debug("[TPM_CAP_KEY_HANDLE]");
      subCapSize = CPU_TO_BE32(TPM_RT_KEY);
      return cap_handle(4, (BYTE*)&subCapSize, respSize, resp);

    case TPM_CAP_CHECK_LOADED:
      debug("[TPM_CAP_CHECK_LOADED]");
      return cap_loaded(subCapSize, subCap, respSize, resp);

    case TPM_CAP_SYM_MODE:
      debug("[TPM_CAP_SYM_MODE]");
      /* TODO: TPM_CAP_SYM_MODE */
      return TPM_FAIL;

    case TPM_CAP_KEY_STATUS:
      debug("[TPM_CAP_KEY_STATUS]");
      /* TODO: TPM_CAP_KEY_STATUS */
      return TPM_FAIL;

    case TPM_CAP_NV_LIST:
      debug("[TPM_CAP_NV_LIST]");
      return cap_nv_list(respSize, resp);

    case TPM_CAP_MFR:
      debug("[TPM_CAP_MFR]");
      return cap_mfr(subCapSize, subCap, respSize, resp);

    case TPM_CAP_NV_INDEX:
      debug("[TPM_CAP_NV_INDEX]");
      return cap_nv_index(subCapSize, subCap, respSize, resp);

    case TPM_CAP_TRANS_ALG:
      debug("[TPM_CAP_TRANS_ALG]");
      /* TODO: TPM_CAP_TRANS_ALG */
      return TPM_FAIL;

    case TPM_CAP_HANDLE:
      debug("[TPM_CAP_HANDLE]");
      return cap_handle(subCapSize, subCap, respSize, resp);

    case TPM_CAP_TRANS_ES:
      debug("[TPM_CAP_TRANS_ES]");
      /* TODO: TPM_CAP_TRANS_ES */
      return TPM_FAIL;

    case TPM_CAP_AUTH_ENCRYPT:
      debug("[TPM_CAP_AUTH_ENCRYPT]");
      return cap_auth_encrypt(subCapSize, subCap, respSize, resp);

    case TPM_CAP_SELECT_SIZE:
      debug("[TPM_CAP_SELECT_SIZE]");
      /* TODO: TPM_CAP_SELECT_SIZE */
      return TPM_FAIL;

    case TPM_CAP_VERSION_VAL:
      debug("[TPM_CAP_VERSION_VAL]");
      return cap_version_val(respSize, resp);

    default:
      return TPM_BAD_MODE;
  }
}

TPM_RESULT TPM_SetCapability(TPM_CAPABILITY_AREA capArea, UINT32 subCapSize, 
                             BYTE *subCap, UINT32 setValueSize, BYTE *setValue)
{
  info("TPM_SetCapability() not implemented yet");
  /* TODO: implement TPM_SetCapability() */
  return TPM_FAIL;
}

TPM_RESULT TPM_GetCapabilityOwner(TPM_VERSION *version, 
                                  UINT32 *non_volatile_flags, 
                                  UINT32 *volatile_flags, 
                                  TPM_AUTH *auth1)
{
  TPM_RESULT res;
  
  info("TPM_GetCapabilityOwner()");
  
  /* Verify owner authorization */
  res = tpm_verify_auth(auth1, tpmData.permanent.data.ownerAuth, TPM_KH_OWNER);
  if (res != TPM_SUCCESS) return res;
  
  /* initialize */
  *version = tpmData.permanent.data.version;
  *non_volatile_flags = *volatile_flags = 0;
  
  /* set non-volatile flags */
  if (tpmData.permanent.flags.disable)
    *non_volatile_flags |= (1 <<  0);
  if (tpmData.permanent.flags.ownership)
    *non_volatile_flags |= (1 <<  1);
  if (tpmData.permanent.flags.deactivated)
    *non_volatile_flags |= (1 <<  2);
  if (tpmData.permanent.flags.readPubek)
    *non_volatile_flags |= (1 <<  3);
  if (tpmData.permanent.flags.disableOwnerClear)
    *non_volatile_flags |= (1 <<  4);
  if (tpmData.permanent.flags.allowMaintenance)
    *non_volatile_flags |= (1 <<  5);
  if (tpmData.permanent.flags.physicalPresenceLifetimeLock)
    *non_volatile_flags |= (1 <<  6);
  if (tpmData.permanent.flags.physicalPresenceHWEnable)
    *non_volatile_flags |= (1 <<  7);
  if (tpmData.permanent.flags.physicalPresenceCMDEnable)
    *non_volatile_flags |= (1 <<  8);
  if (tpmData.permanent.flags.CEKPUsed)
    *non_volatile_flags |= (1 <<  9);
  if (tpmData.permanent.flags.TPMpost)
    *non_volatile_flags |= (1 << 10);
  if (tpmData.permanent.flags.TPMpostLock)
    *non_volatile_flags |= (1 << 11);
  if (tpmData.permanent.flags.FIPS)
    *non_volatile_flags |= (1 << 12);
  if (tpmData.permanent.flags.operator)
    *non_volatile_flags |= (1 << 13);
  if (tpmData.permanent.flags.enableRevokeEK)
    *non_volatile_flags |= (1 << 14);
  if (tpmData.permanent.flags.nvLocked)
    *non_volatile_flags |= (1 << 15);
  if (tpmData.permanent.flags.readSRKPub)
    *non_volatile_flags |= (1 << 16);
  if (tpmData.permanent.flags.tpmEstablished)
    *non_volatile_flags |= (1 << 17);
  if (tpmData.permanent.flags.maintenanceDone)
    *non_volatile_flags |= (1 << 18);
  
  /* set volatile flags */
  if (tpmData.stclear.flags.deactivated)
    *volatile_flags |= (1 <<  0);
  if (tpmData.stclear.flags.disableForceClear)
    *volatile_flags |= (1 <<  1);
  if (tpmData.stclear.flags.physicalPresence)
    *volatile_flags |= (1 <<  2);
  if (tpmData.stclear.flags.physicalPresenceLock)
    *volatile_flags |= (1 <<  3);
  if (tpmData.stclear.flags.bGlobalLock)
    *volatile_flags |= (1 <<  4);
  
  return TPM_SUCCESS;
}
