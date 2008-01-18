/* Software-Based Trusted Platform Module (TPM) Emulator for Linux
 * Copyright (C) 2004 Mario Strasser <mast@gmx.net>,
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
 * $Id: tpm_emulator.h 142 2006-11-11 21:10:20Z mast $
 */

#ifndef _TPM_EMULATOR_H_
#define _TPM_EMULATOR_H_

#include "tpm_emulator_config.h"

/* emulator version */
#define TPM_STR(s) _TPM_STR(s)
#define _TPM_STR(s) #s
#include "tpm_version.h"

#define TPM_MANUFACTURER 0x4554485A /* 'ETHZ' */        

/**
 * tpm_emulator_init - initialises and starts the TPM emulator
 * @startup: [in] startup mode
 */
void tpm_emulator_init(uint32_t startup);

/**
 * tpm_emulator_shutdown - shuts the TPM emulator down
 */
void tpm_emulator_shutdown(void);

/**
 * tpm_handle_command - handles (i.e., executes) TPM commands
 * @in: [in] incoming TPM command
 * @in_size: [in] total number of input bytes
 * @out: [inout] outgoing TPM result
 * @out_size: [inout] total number of output bytes
 * @Returns: 0 on success, -1 otherwise
 *
 * Description: Handles (i.e., executes) TPM commands. The parameters
 * out and out_size determine the output buffer and its capacity,
 * respectively. If out is NULL, the required memory is allocated
 * internally and has to be released by means of tpm_free() after
 * its usage. In case of an error, all internally allocated memory
 * is released and the the state of out and out_size is unspecified.
 */ 
int tpm_handle_command(const uint8_t *in, uint32_t in_size, uint8_t **out, uint32_t *out_size);

#endif /* _TPM_EMULATOR_H_ */

