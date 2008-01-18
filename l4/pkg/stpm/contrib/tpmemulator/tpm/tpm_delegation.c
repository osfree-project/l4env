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
 * $Id: tpm_delegation.c 1 2004-11-03 17:22:56Z mast $
 */

#include "tpm_emulator.h"
#include "tpm_commands.h"

/*
 * Delegation Commands ([TPM_Part3], Section 19)
 */

TPM_RESULT TPM_Delegate_Manage(  
  TPM_FAMILY_ID familyID,
  TPM_FAMILY_OPERATION opFlag,
  UINT32 opDataSize,
  BYTE *opData,
  TPM_AUTH *auth1,  
  UINT32 *retDataSize,
  BYTE **retData  
)
{
  info("TPM_Delegate_Manage() not implemented yet");
  /* TODO: implement TPM_Delegate_Manage() */
  return TPM_FAIL;
}

TPM_RESULT TPM_Delegate_CreateKeyDelegation(  
  TPM_KEY_HANDLE keyHandle,
  TPM_DELEGATE_PUBLIC *publicInfo,
  TPM_ENCAUTH *delAuth,
  TPM_AUTH *auth1,  
  UINT32 *blobSize,
  TPM_DELEGATE_KEY_BLOB *blob 
)
{
  info("TPM_Delegate_CreateKeyDelegation() not implemented yet");
  /* TODO: implement TPM_Delegate_CreateKeyDelegation() */
  return TPM_FAIL;
}

TPM_RESULT TPM_Delegate_CreateOwnerDelegation(  
  BOOL increment,
  TPM_DELEGATE_PUBLIC *publicInfo,
  TPM_ENCAUTH *delAuth,
  TPM_AUTH *auth1,  
  UINT32 *blobSize,
  TPM_DELEGATE_OWNER_BLOB *blob 
)
{
  info("TPM_Delegate_CreateOwnerDelegation() not implemented yet");
  /* TODO: implement TPM_Delegate_CreateOwnerDelegation() */
  return TPM_FAIL;
}

TPM_RESULT TPM_Delegate_LoadOwnerDelegation(  
  TPM_DELEGATE_INDEX index,
  UINT32 blobSize,
  TPM_AUTH *auth1
)
{
  info("TPM_Delegate_LoadOwnerDelegation() not implemented yet");
  /* TODO: implement TPM_Delegate_LoadOwnerDelegation() */
  return TPM_FAIL;
}

TPM_RESULT TPM_Delegate_ReadTable(  
  UINT32 *familyTableSize,
  BYTE **familyTable ,
  UINT32 *delegateTableSize,
  TPM_DELEGATE_PUBLIC **delegateTable  
)
{
  info("TPM_Delegate_ReadTable() not implemented yet");
  /* TODO: implement TPM_Delegate_ReadTable() */
  return TPM_FAIL;
}

TPM_RESULT TPM_Delegate_UpdateVerification(  
  UINT32 inputSize,
  BYTE *inputData,
  TPM_AUTH *auth1,  
  UINT32 *outputSize,
  BYTE **outputData  
)
{
  info("TPM_Delegate_UpdateVerification() not implemented yet");
  /* TODO: implement TPM_Delegate_UpdateVerification() */
  return TPM_FAIL;
}

TPM_RESULT TPM_Delegate_VerifyDelegation(  
  UINT32 delegateSize,
  BYTE *delegation
)
{
  info("TPM_Delegate_VerifyDelegation() not implemented yet");
  /* TODO: implement TPM_Delegate_VerifyDelegation() */
  return TPM_FAIL;
}

