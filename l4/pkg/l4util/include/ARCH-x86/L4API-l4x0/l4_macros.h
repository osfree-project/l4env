/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4util/include/ARCH-x86/L4API-l4v2/l4_macros.h
 * \brief  Some useful generic macros, L4 x0 version
 *
 * \date   11/12/2002
 * \author Lars Reuther <reuther@os.inf.tu-dresden.de>
 *
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
#ifndef _L4UTIL_L4_MACROS_H
#define _L4UTIL_L4_MACROS_H

/*****************************************************************************
 *** generic macros
 *****************************************************************************/

/* generate L4 thread id printf string */
#ifndef IdStr
#  define IdFmt       "%x.%x"
#  define IdStr(tid)  (tid).id.task,(tid).id.lthread
#endif

#endif /* !_L4UTIL_L4_MACROS_H */
