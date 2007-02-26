/*!
 * \file   dietlibc/lib/backends/l4env_base/_exit.h
 * \brief  
 *
 * \date   08/19/2003
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __DIETLIBC_LIB_BACKENDS_L4ENV_BASE__EXIT_H_
#define __DIETLIBC_LIB_BACKENDS_L4ENV_BASE__EXIT_H_

void _exit(int code) __attribute__ ((__noreturn__));
void __thread_doexit(int code);

#endif
