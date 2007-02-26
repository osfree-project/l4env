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
#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

void crt0_construction(void);
void crt0_sys_destruction(void);
void crt0_dde_construction(void);

void __main(void);

extern l4_uint8_t crt0_stack_low;
extern l4_uint8_t crt0_stack_high;
extern l4_addr_t  crt0_tramppage;
extern void*      crt0_multiboot_info;
extern l4_mword_t crt0_multiboot_flag;
extern void*      crt0_l4env_infopage;

EXTERN_C_END

#endif
