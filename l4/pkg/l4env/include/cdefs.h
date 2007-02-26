/* $Id$ */
/*****************************************************************************/
/**
 * \file    l4env/include/cdefs.h
 * \brief   L4Env, common defines.
 * \ingroup misc
 *
 * \date    12/30/2000
 * \author  Lars Reuther <reuther@os.inf.tu-dresden.de>
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
#ifndef _L4ENV_CDEFS_H
#define _L4ENV_CDEFS_H

#ifdef __cplusplus

#ifndef __BEGIN_DECLS 
#  define __BEGIN_DECLS  extern "C" {
#endif

#ifndef __END_DECLS
#  define __END_DECLS    }
#endif

#ifndef L4_INLINE
#  define L4_INLINE inline
#endif

#else /*  __cplusplus */

#ifndef __BEGIN_DECLS 
#  define __BEGIN_DECLS
#endif

#ifndef __END_DECLS
#  define __END_DECLS
#endif

#ifndef L4_INLINE
#  ifdef STATIC_L4_INLINE
#   define L4_INLINE static __inline__
#  else
#   define L4_INLINE extern __inline__
#  endif
#endif

#endif /* !__cplusplus */

#endif /* !_L4ENV_CDEFS_H */
