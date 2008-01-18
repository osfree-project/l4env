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
 * $Id: hmac.h 139 2006-11-10 16:09:00Z mast $
 */

#ifndef _HMAC_H_
#define _HMAC_H_

#include "tpm_emulator_config.h"
#include "sha1.h"

#define HMAC_PAD_LENGTH 64

typedef struct {
  tpm_sha1_ctx_t ctx;
  uint8_t k_opad[HMAC_PAD_LENGTH];
} tpm_hmac_ctx_t;

void tpm_hmac_init(tpm_hmac_ctx_t *ctx, const uint8_t *key, size_t key_len);

void tpm_hmac_update(tpm_hmac_ctx_t *ctx, const uint8_t *data, size_t length);

void tpm_hmac_final(tpm_hmac_ctx_t *ctx, uint8_t *digest);

#endif /* _HMAC_H_ */

