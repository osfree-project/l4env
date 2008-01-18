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
 * $Id: tpm_maintenance.c 1 2004-11-03 17:22:56Z mast $
 */

#include "tpm_emulator.h"
#include "tpm_commands.h"

/*
 * Maintenance Functions ([TPM_Part3], Section 12)
 */

TPM_RESULT TPM_CreateMaintenanceArchive(  
  BOOL generateRandom,
  TPM_AUTH *auth1,  
  UINT32 *randomSize,
  BYTE **random ,
  UINT32 *archiveSize,
  BYTE **archive  
)
{
  info("TPM_CreateMaintenanceArchive() not implemented yet");
  /* TODO: implement TPM_CreateMaintenanceArchive() */
  return TPM_FAIL;
}

TPM_RESULT TPM_LoadMaintenanceArchive(  
  UINT32 inArgumentsSize,
  BYTE *inArguments,
  TPM_AUTH *auth1,  
  UINT32 *outArgumentsSize,
  BYTE **outArguments  
)
{
  info("TPM_LoadMaintenanceArchive() not implemented yet");
  /* TODO: implement TPM_LoadMaintenanceArchive() */
  return TPM_FAIL;
}

TPM_RESULT TPM_KillMaintenanceFeature(  
  TPM_AUTH *auth1
)
{
  info("TPM_KillMaintenanceFeature() not implemented yet");
  /* TODO: implement TPM_KillMaintenanceFeature() */
  return TPM_FAIL;
}

TPM_RESULT TPM_LoadManuMaintPub(  
  TPM_NONCE *antiReplay,
  TPM_PUBKEY *pubKey,  
  TPM_DIGEST *checksum 
)
{
  info("TPM_LoadManuMaintPub() not implemented yet");
  /* TODO: implement TPM_LoadManuMaintPub() */
  return TPM_FAIL;
}

TPM_RESULT TPM_ReadManuMaintPub(  
  TPM_NONCE *antiReplay,  
  TPM_DIGEST *checksum 
)
{
  info("TPM_ReadManuMaintPub() not implemented yet");
  /* TODO: implement TPM_ReadManuMaintPub() */
  return TPM_FAIL;
}

