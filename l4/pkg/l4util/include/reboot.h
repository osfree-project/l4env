/**
 * \file   l4util/include/reboot.h
 * \brief  Machine restarting
 *
 * \date   01/2004
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2000-2004
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/
#ifndef _L4UTIL_REBOOT_H
#define _L4UTIL_REBOOT_H

#include <l4/sys/compiler.h>

/** \defgroup reboot Machine Restarting Function */

/**
 * \brief Machine reboot
 * \ingroup reboot
 */
EXTERN_C_BEGIN
L4_CV void l4util_reboot(void) __attribute__ ((__noreturn__));
EXTERN_C_END

#endif /* ! _L4UTIL_REBOOT_H */
