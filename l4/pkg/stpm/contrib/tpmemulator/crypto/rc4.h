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
 * $Id: rc4.h 139 2006-11-10 16:09:00Z mast $
 */
 
#ifndef _RC4_H_
#define _RC4_H_

#include "tpm_emulator_config.h"

typedef struct {
    uint8_t state[256];
    uint8_t x, y;
} tpm_rc4_ctx_t;

void tpm_rc4_init(tpm_rc4_ctx_t *s, uint8_t *key, size_t key_len);

void tpm_rc4_crypt(tpm_rc4_ctx_t *s, uint8_t *in, uint8_t *out, size_t length);

#endif /* _RC4_h_ */
