/**
 * \file    crtx/include/crt0.h
 * \brief   Variables exported by crt0. This is low-level stuff --
 *          applications should not use this information directly.
 *
 * \date    08/2003
 * \author  Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef CRTX_CRT0_H
#define CRTX_CRT0_H

#include <l4/sys/l4int.h>

#ifdef __cplusplus
#define XC "C"
#else
#define XC
#endif

extern XC void crt0_construction(void);
extern XC void crt0_sys_destruction(void);
extern XC void __main(void);

extern XC l4_uint8_t crt0_stack_low;
extern XC l4_uint8_t crt0_stack_high;
extern XC l4_addr_t  crt0_tramppage;
extern XC void*      crt0_multiboot_info;
extern XC l4_mword_t crt0_multiboot_flag;

#undef XC

#endif

