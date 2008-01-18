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
 * $Id: tpm_data.h 1 2004-11-03 17:22:56Z mast $
 */

#ifndef _TPM_DATA_H_
#define _TPM_DATA_H

#include "tpm_structures.h"

extern TPM_DATA tpmData;

BOOL tpm_get_physical_presence(void);

void tpm_init_data(void);

void tpm_release_data(void);

int tpm_store_permanent_data(void);

int tpm_restore_permanent_data(void);

int tpm_erase_permanent_data(void);

#endif /* _TPM_DATA_H_ */
