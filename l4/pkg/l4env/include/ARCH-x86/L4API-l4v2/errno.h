/*!
 * \file   l4env/include/ARCH-x86/L4API-l4v2/errno.h
 * \brief  Architecture-dependent include file to slurp in L4 IPC error strings
 *
 * \date   08/06/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __L4ENV_INCLUDE_ARCH_X86_L4API_L4V2_ERRNO_H_
#define __L4ENV_INCLUDE_ARCH_X86_L4API_L4V2_ERRNO_H_

asm(".globl l4env_err_ipcstrings; .type l4env_err_ipcstrings,@object");

#endif

#include_next <l4/env/errno.h>
