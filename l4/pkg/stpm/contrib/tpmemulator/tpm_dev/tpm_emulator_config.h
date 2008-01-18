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
 * $Id: tpm_emulator_config.h 158 2006-12-03 10:44:02Z mast $
 */

#ifndef _TPM_EMULATOR_CONFIG_H_
#define _TPM_EMULATOR_CONFIG_H_

#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/random.h>
#include <linux/time.h>
#include <asm/byteorder.h>

/* module settings */

#define TPM_DEVICE_MINOR  224
#define TPM_DEVICE_NAME   "tpm"
#define TPM_MODULE_NAME   "tpm_emulator"

/* TPM emulator configuration */

#undef  TPM_STRONG_PERSISTENCE
#undef  TPM_GENERATE_EK
#undef  TPM_GENERATE_SEED_DAA
#undef  TPM_MEMORY_ALIGNMENT_MANDATORY

/* debug and log output functions */

#ifdef DEBUG
#define debug(fmt, ...) printk(KERN_DEBUG "%s %s:%d: Debug: " fmt "\n", \
                        TPM_MODULE_NAME, __FILE__, __LINE__, ## __VA_ARGS__)
#else
#define debug(fmt, ...) 
#endif
#define info(fmt, ...)  printk(KERN_INFO "%s %s:%d: Info: " fmt "\n", \
                        TPM_MODULE_NAME, __FILE__, __LINE__, ## __VA_ARGS__)
#define error(fmt, ...) printk(KERN_ERR "%s %s:%d: Error: " fmt "\n", \
                        TPM_MODULE_NAME, __FILE__, __LINE__, ## __VA_ARGS__)
#define alert(fmt, ...) printk(KERN_ALERT "%s %s:%d: Alert: " fmt "\n", \
                        TPM_MODULE_NAME, __FILE__, __LINE__, ## __VA_ARGS__)

/* min and max */

#define tpm_min min

/* memory allocation */

static inline void *tpm_malloc(size_t size) 
{
  return kmalloc(size, GFP_KERNEL);  
}

static inline void tpm_free(const void *ptr)
{
  if (ptr != NULL) kfree(ptr);
}

/* random numbers */

static inline void tpm_get_random_bytes(void *buf, size_t nbytes)
{
  get_random_bytes(buf, nbytes);
}

/* usec since last call */

uint64_t tpm_get_ticks(void);

/* file handling */

int tpm_write_to_file(uint8_t *data, size_t data_length);
int tpm_read_from_file(uint8_t **data, size_t *data_length);

/* byte order conversions */

#define CPU_TO_BE64(x) __cpu_to_be64(x)
#define CPU_TO_LE64(x) __cpu_to_le64(x)
#define CPU_TO_BE32(x) __cpu_to_be32(x)
#define CPU_TO_LE32(x) __cpu_to_le32(x)
#define CPU_TO_BE16(x) __cpu_to_be16(x)
#define CPU_TO_LE16(x) __cpu_to_le16(x)

#define BE64_TO_CPU(x) __be64_to_cpu(x)
#define LE64_TO_CPU(x) __le64_to_cpu(x)
#define BE32_TO_CPU(x) __be32_to_cpu(x)
#define LE32_TO_CPU(x) __le32_to_cpu(x)
#define BE16_TO_CPU(x) __be16_to_cpu(x)
#define LE16_TO_CPU(x) __le16_to_cpu(x)

#endif /* _TPM_EMULATOR_CONFIG_H_ */
