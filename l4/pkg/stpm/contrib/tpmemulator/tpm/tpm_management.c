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
 * $Id: tpm_management.c 100 2006-05-09 22:58:41Z hstamer $
 */

#include "tpm_emulator.h"
#include "tpm_commands.h"

/*
 * Administrative Functions ([TPM_Part3], Section 9)
 */

TPM_RESULT TPM_FieldUpgrade()
{
  info("TPM_FieldUpgrade() not implemented yet");
  /* TODO: implement TPM_FieldUpgrade() */
  return TPM_FAIL;
}

TPM_RESULT TPM_SetRedirection(
  TPM_KEY_HANDLE keyHandle,
  TPM_REDIR_COMMAND redirCmd,
  UINT32 inputDataSize,
  BYTE *inputData,
  TPM_AUTH *auth1
)
{
  info("TPM_SetRedirection() not implemented yet");
  /* TODO: implement TPM_SetRedirection() */
  return TPM_FAIL;
}

TPM_RESULT TPM_ResetLockValue(
  TPM_AUTH *auth1
)
{
  info("TPM_ResetLockValue not implemented yet");
  /* TODO: implement TPM_ResetLockValue() */
  return TPM_FAIL;
}
