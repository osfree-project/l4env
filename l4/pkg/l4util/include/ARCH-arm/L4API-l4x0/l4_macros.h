/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4util/include/ARCH-arm/L4API-l4x0/l4_macros.h
 * \brief  Some useful generic macros, L4 x0 version
 *
 * \date   11/12/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de> */

/*
 * Copyright (C) 2000-2002
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

#include_next <l4/util/l4_macros.h>

#ifndef _L4UTIL__ARCH_ARM__L4X0__L4_MACROS_H
#define _L4UTIL__ARCH_ARM__L4X0__L4_MACROS_H

/*****************************************************************************
 *** generic macros
 *****************************************************************************/

/* generate L4 thread id printf string */
#ifndef l4util_idstr
#  define l4util_idfmt         "%x.%02x"
#  define l4util_idfmt_adjust  "%3x.%02x"
#  define l4util_idstr(tid)    (tid).id.task,(tid).id.lthread
#endif

/* generate printf string of the task number of an L4 thread id */
#ifndef l4util_idtskstr
#  define l4util_idtskfmt      "#%x"
#  define l4util_idtskstr(tid) (tid).id.task
#endif

#endif /* !_L4UTIL__ARCH_ARM__L4X0__L4_MACROS_H */
