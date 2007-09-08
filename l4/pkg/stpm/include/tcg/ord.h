/*
 * \brief   Header for TPM ordinals.
 * \date    2004-06-02
 * \author  Bernhard Kauer <kauer@tudos.org>
 */
/*
 * Copyright (C) 2004  Bernhard Kauer <kauer@tudos.org>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the STPM package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef _ORD_H
#define _ORD_H

enum tpm_ords {
	TPM_ORD_OIAP=10,
	TPM_ORD_OSAP=11,
	TPM_ORD_Extend=20,
	TPM_ORD_PcrRead,
	TPM_ORD_Quote,
	TPM_ORD_Seal,
        TPM_ORD_Unseal,
	TPM_ORD_CreateWrapKey=31,
	TPM_ORD_LoadKey=32,
	TPM_ORD_EvictKey=34,
	TPM_ORD_GetRandom = 70,
	TPM_ORD_StirRandom,
        TPM_ORD_SelfTestFull=80,
	TPM_ORD_Reset=90,
	TPM_ORD_OwnerClear=91,
	TPM_ORD_ForceClear=93,
	TPM_ORD_GetCapability=101,
	TPM_ORD_GetCapability_Key_Handle=101,
	TPM_ORD_GetCapability_Keys=101,
	TPM_ORD_GetCapability_Pcrs=101,
	TPM_ORD_GetCapability_Slots=101,
	TPM_ORD_GetCapability_Version=101,
	TPM_ORD_PhysicalEnable = 111,
	TPM_ORD_ReadPubek=124,
	TPM_ORD_Terminate_Handle=150,
	TPM_ORD_SaveState=152,
	TPM_ORD_Startup,
	TPM_ORD_SHA1Start=160,
	TPM_ORD_SHA1Update,
	TPM_ORD_SHA1Complete,
	TPM_ORD_SHA1CompleteExtend,
	TPM_ORD_CreateCounter = 220,
	TPM_ORD_IncrementCounter,
	TPM_ORD_ReadCounter,
	TPM_ORD_ReleaseCounter,
	TPM_ORD_ReleaseCounterOwner,
	TPM_ORD_PhysicalPresence = 0x0A000040,
};

enum tpm_caps {
	TPM_CAP_PROPERTY=5,
	TPM_CAP_VERSION,
	TPM_CAP_KEY_HANDLE,
};

enum tpm_subcaps {
	TPM_CAP_PROP_PCR   = 257,
	TPM_CAP_PROP_SLOTS = 260,
};

enum tpm_subcaps_size {
  TPM_NO_SUBCAP=0,
  TPM_SUBCAP=4,
};


enum tpm_startup_code {
	TPM_ST_CLEAR=1,
	TPM_ST_STATE,
	TPM_ST_DEACTIVATED,
};

#endif
