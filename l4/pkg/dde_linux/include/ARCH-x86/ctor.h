/* $Id$ */
/*****************************************************************************/
/**
 * \file   dde_linux/include/ARCH-x86/ctor.h
 * \brief  Linux DDE Initialization for Linux __initcalls
 *
 * \date   08/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * \author Jork Loeser <jork@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __DDE_LINUX_INCLUDE_ARCH_X86_CTOR_H_
#define __DDE_LINUX_INCLUDE_ARCH_X86_CTOR_H_

#include <l4/sys/compiler.h>
#include <l4/env/cdefs.h>

__BEGIN_DECLS

typedef void (*l4dde_initcall_t)(void);

extern l4dde_initcall_t __DDE_CTOR_LIST__, __DDE_CTOR_END__;

#define l4dde_initcall(fn)	\
	static l4dde_initcall_t \
	  L4_STICKY(__l4dde_initcall_##fn) __l4dde_init_call = fn

#define __l4dde_init_call 	\
	__attribute__ ((__section__ (".l4dde_ctors")))

/** Call the registered initcall functions
 *
 * This function calls all functions that registered using the
 * l4env_initcall macro. Do not forget to link using the main_stat.ld linker
 * script that respects the ".l4env_inicall.init" section.
 */
extern void l4dde_do_initcalls(void);

__END_DECLS

#endif

