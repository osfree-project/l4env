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
 * $Id: gmp_kernel_wrapper.c 159 2006-12-03 10:45:34Z mast $
 */

#include "tpm_emulator_config.h"
#include "gmp.h"

int __gmp_junk;

/* GNU MP assertion and abort functions */

void __attribute__ ((regparm(0))) __gmp_assert_fail(const char *filename, 
  int linenum, const char *expr) 
{
  panic(KERN_CRIT TPM_MODULE_NAME "%s:%d: GNU MP assertion failed: %s\n", 
    filename, linenum, expr);
}

void __attribute__ ((regparm(0))) abort(void)
{
  panic(KERN_CRIT TPM_MODULE_NAME "GNU MP abort() was called\n");
}

void __stack_chk_fail(void)
{
  error("stack-protector: stack is corrupted\n");
}

const unsigned short * __ctype_b_loc (void)
{
  static const unsigned short table[384] = {0};
  return &table[128];
}

/* GNU MP memory management */

void __attribute__ ((regparm(0))) *kernel_allocate(size_t size)
{
  void *ret  = (void*)kmalloc(size, GFP_KERNEL);
  if (!ret) panic(KERN_CRIT TPM_MODULE_NAME 
    "GMP: cannot allocate memory (size=%zd)\n", size);
  return ret;
}

void __attribute__ ((regparm(0))) *kernel_reallocate(void *oldptr, 
  size_t old_size, size_t new_size)
{
  void *ret = (void*)kmalloc(new_size, GFP_KERNEL);
  if (!ret) panic(KERN_CRIT TPM_MODULE_NAME "GMP: Cannot reallocate memory "
    "(old_size=%zd new_size=%zd)\n", old_size, new_size);
  memcpy(ret, oldptr, old_size);
  kfree(oldptr);
  return ret;
}

void __attribute__ ((regparm(0))) kernel_free(void *blk_ptr, size_t blk_size)
{
  /* overwrite used memory */
  if (blk_ptr != NULL) { 
    memset(blk_ptr, 0, blk_size);
    kfree(blk_ptr);
  }
}

void __attribute__ ((regparm(0))) *(*__gmp_allocate_func) 
  __GMP_PROTO ((size_t)) = kernel_allocate;
void __attribute__ ((regparm(0))) *(*__gmp_reallocate_func) 
  __GMP_PROTO ((void *, size_t, size_t)) = kernel_reallocate;
void __attribute__ ((regparm(0))) (*__gmp_free_func) 
  __GMP_PROTO ((void *, size_t)) = kernel_free;

