/**
 * \file   libc_backends_l4env/lib/l4env_start_stop/_exit.h
 * \brief  Function definitions for _exit.c
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __LIBC_BACKENDS_L4ENV_LIB_L4ENV_START_STOP_EXIT_H_
#define __LIBC_BACKENDS_L4ENV_LIB_L4ENV_START_STOP_EXIT_H_

void _exit(int code) __attribute__ ((__noreturn__));
void __thread_doexit(int code);

#endif
