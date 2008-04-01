/**
 * \file   dietlibc/lib/backends/l4_start_stop/_exit.h
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __DIETLIBC_LIB_BACKENDS_L4_START_STOP__EXIT_H_
#define __DIETLIBC_LIB_BACKENDS_L4_START_STOP__EXIT_H_

void _exit(int code) __attribute__ ((__noreturn__));
void __thread_doexit(int code);

#endif
