/*
 * \brief   Fixed-length types definitions
 * \date    2004-08-24
 * \author  Norman Feske <nf2@inf.tu-dresden.de>
 */

/*
 * Copyright (C) 2002-2004  Norman Feske  <nf2@os.inf.tu-dresden.de>
 * Technische Universitaet Dresden, Operating Systems Research Group
 *
 * This file is part of the DOpE package, which is distributed under
 * the  terms  of the  GNU General Public Licence 2.  Please see the
 * COPYING file for details.
 */

#ifndef _NITPICKER_TYPES_H_
#define _NITPICKER_TYPES_H_

#if defined(L4API_l4v2) || defined(L4API_l4x0)

#include <l4/sys/types.h>

typedef l4_uint8_t   u8;
typedef l4_int8_t    s8;
typedef l4_uint16_t u16;
typedef l4_int16_t  s16;
typedef l4_uint32_t u32;
typedef l4_int32_t  s32;
typedef l4_addr_t   adr;

#else

typedef unsigned char   u8;
typedef   signed char   s8;
typedef unsigned short u16;
typedef   signed short s16;
typedef unsigned long  u32;
typedef   signed long  s32;
typedef unsigned long  adr;

#endif

#endif /* _NITPICKER_TYPES_H_ */
