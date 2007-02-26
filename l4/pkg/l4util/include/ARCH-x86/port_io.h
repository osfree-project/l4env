/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4util/include/ARCH-x86/port_io.h
 * \brief  x86 port I/O 
 *
 * \date   06/2003
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef _L4UTIL_PORT_IO_H
#define _L4UTIL_PORT_IO_H

/* L4 includes */
#include <l4/sys/l4int.h>
#include <l4/sys/compiler.h>

/*****************************************************************************
 *** Prototypes
 *****************************************************************************/

/** \defgroup port_io I/O port instructions */

EXTERN_C_BEGIN

/**
 * \brief Read byte from I/O port
 * \ingroup port_io
 *
 * \param  port	   I/O port address
 * \return value
 */
L4_INLINE l4_uint8_t
l4util_in8(l4_uint16_t port);

/**
 * \brief Read 16-bit-value from I/O port
 * \ingroup port_io
 *
 * \param  port	   I/O port address
 * \return value
 */
L4_INLINE l4_uint16_t
l4util_in16(l4_uint16_t port);

/**
 * \brief Read 32-bit-value from I/O port
 * \ingroup port_io
 *
 * \param  port	   I/O port address
 * \return value
 */
L4_INLINE l4_uint32_t
l4util_in32(l4_uint16_t port);

/**
 * \brief Write byte to I/O port
 * \ingroup port_io
 *
 * \param  port	   I/O port address
 * \param  value   value to write
 */
L4_INLINE void
l4util_out8(l4_uint8_t value, l4_uint16_t port);

/**
 * \brief Write 16-bit-value to I/O port
 * \ingroup port_io
 *
 * \param  port	   I/O port address
 * \param  value   value to write
 */
L4_INLINE void
l4util_out16(l4_uint16_t value, l4_uint16_t port);

/**
 * \brief Write 32-bit-value to I/O port
 * \ingroup port_io
 *
 * \param  port	   I/O port address
 * \param  value   value to write
 */
L4_INLINE void
l4util_out32(l4_uint32_t value, l4_uint16_t port);

/**
 * \brief delay I/O port access by writing to port 0x80
 * \ingroup port_io
 */
L4_INLINE void
l4util_iodelay(void);

EXTERN_C_END


/*****************************************************************************
 *** Implementation
 *****************************************************************************/

L4_INLINE l4_uint8_t
l4util_in8(l4_uint16_t port)
{
  l4_uint8_t value;
  asm volatile ("inb %w1, %b0" : "=a" (value) : "Nd" (port));
  return value;
}

L4_INLINE l4_uint16_t
l4util_in16(l4_uint16_t port)
{
  l4_uint16_t value;
  asm volatile ("inw %w1, %w0" : "=a" (value) : "Nd" (port));
  return value;
}

L4_INLINE l4_uint32_t
l4util_in32(l4_uint16_t port)
{
  l4_uint32_t value;
  asm volatile ("inl %w1, %0" : "=a" (value) : "Nd" (port));
  return value;
}

L4_INLINE void
l4util_out8(l4_uint8_t value, l4_uint16_t port)
{
  asm volatile ("outb %b0, %w1" : : "a" (value), "Nd" (port));
}

L4_INLINE void
l4util_out16(l4_uint16_t value, l4_uint16_t port)
{
  asm volatile ("outw %w0, %w1" : : "a" (value), "Nd" (port));
}

L4_INLINE void
l4util_out32(l4_uint32_t value, l4_uint16_t port)
{
  asm volatile ("outl %0, %w1" : : "a" (value), "Nd" (port));
}

L4_INLINE void
l4util_iodelay(void)
{
  asm volatile ("outb %al,$0x80");
}

#endif

