/* Software-Based Trusted Platform Module (TPM) Emulator for Linux
 * Copyright (C) 2006 Mario Strasser <mast@gmx.net>,
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
 * $Id: bn.h 152 2006-11-15 17:07:36Z mast $
 */
 
#ifndef _BN_H_
#define _BN_H_

#include "tpm_emulator_config.h"
#include <gmp.h>

typedef mpz_t tpm_bn_t;

void tpm_bn_init(tpm_bn_t a);

void tpm_bn_init2(tpm_bn_t a, size_t nbits);

void tpm_bn_init_set(tpm_bn_t a, tpm_bn_t val);

void tpm_bn_init_set_ui(tpm_bn_t a, uint32_t val);

void tpm_bn_set_ui(tpm_bn_t a, uint32_t val);

void tpm_bn_clear(tpm_bn_t a);

void tpm_bn_swap(tpm_bn_t a, tpm_bn_t b);

uint32_t tpm_bn_bitsize(tpm_bn_t a);

void tpm_bn_import(tpm_bn_t out, size_t count, int order, const void *in);

void tpm_bn_export(void *out, size_t *count, int order, tpm_bn_t in);

int tpm_bn_cmp(tpm_bn_t a, tpm_bn_t b);

int tpm_bn_cmp_ui(tpm_bn_t a, uint32_t b);

int tpm_bn_sgn(tpm_bn_t a);

void tpm_bn_setbit(tpm_bn_t res, uint32_t bit);

void tpm_bn_add(tpm_bn_t res, tpm_bn_t a, tpm_bn_t b);

void tpm_bn_add_ui(tpm_bn_t res, tpm_bn_t a, uint32_t b);

void tpm_bn_sub(tpm_bn_t res, tpm_bn_t a, tpm_bn_t b);

void tpm_bn_sub_ui(tpm_bn_t res, tpm_bn_t a, uint32_t b);

void tpm_bn_mul(tpm_bn_t res, tpm_bn_t a, tpm_bn_t b);

void tpm_bn_mod(tpm_bn_t res, tpm_bn_t a, tpm_bn_t mod);

void tpm_bn_powm(tpm_bn_t res, tpm_bn_t base, tpm_bn_t exp, tpm_bn_t mod);

void tpm_bn_ui_pow_ui(tpm_bn_t res, uint32_t base, uint32_t exp);

void tpm_bn_fdiv_q_2exp(tpm_bn_t res, tpm_bn_t n, uint32_t b);

void tpm_bn_tdiv_q(tpm_bn_t res, tpm_bn_t a, tpm_bn_t b);

void tpm_bn_gcd(tpm_bn_t res, tpm_bn_t a, tpm_bn_t b);

void tpm_bn_invert(tpm_bn_t res, tpm_bn_t a, tpm_bn_t b);

void tpm_bn_nextprime(tpm_bn_t res, tpm_bn_t a);

#endif /* _BN_H_ */
