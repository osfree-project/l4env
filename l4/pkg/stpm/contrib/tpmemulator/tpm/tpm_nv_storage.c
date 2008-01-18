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
 * $Id: tpm_nv_storage.c 79 2006-01-09 21:41:31Z hstamer $
 */

#include "tpm_emulator.h"
#include "tpm_commands.h"

/*
 * Non-volatile Storage ([TPM_Part3], Section 20)
 * This section handles the allocation and use of the TPM non-volatile storage.
 */
 
TPM_RESULT TPM_NV_DefineSpace(  
  TPM_NV_DATA_PUBLIC *pubInfo,
  TPM_ENCAUTH *encAuth,
  TPM_AUTH *auth1
)
{
  info("TPM_NV_DefineSpace() not implemented yet");
  /* TODO: implement TPM_NV_DefineSpace() */
  return TPM_FAIL;
}

TPM_RESULT TPM_NV_WriteValue(  
  TPM_NV_INDEX nvIndex,
  UINT32 offset,
  TPM_AUTH *auth1
)
{
  info("TPM_NV_WriteValue() not implemented yet");
  /* TODO: implement TPM_NV_WriteValue() */
  return TPM_FAIL;
}

TPM_RESULT TPM_NV_WriteValueAuth(  
  TPM_NV_INDEX nvIndex,
  UINT32 offset,
  UINT32 dataSize,
  BYTE *data,
  TPM_AUTH *auth1
)
{
  info("TPM_NV_WriteValueAuth() not implemented yet");
  /* TODO: implement TPM_NV_WriteValueAuth() */
  return TPM_FAIL;
}

TPM_RESULT TPM_NV_ReadValue(  
  TPM_NV_INDEX nvIndex,
  UINT32 offset,
  UINT32 inDataSize,
  TPM_AUTH *auth1,  
  UINT32 *outDataSize,
  BYTE **data  
)
{
  info("TPM_NV_ReadValue() not implemented yet");
  /* TODO: implement TPM_NV_ReadValue() */
  return TPM_FAIL;
}

TPM_RESULT TPM_NV_ReadValueAuth(  
  TPM_NV_INDEX nvIndex,
  UINT32 offset,
  UINT32 inDataSize,
  TPM_AUTH *auth1,  
  UINT32 *outDataSize,
  BYTE **data  
)
{
  info("TPM_NV_ReadValueAuth() not implemented yet");
  /* TODO: implement TPM_NV_ReadValueAuth() */
  return TPM_FAIL;
}
