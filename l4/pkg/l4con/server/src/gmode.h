/* $Id$ */
/**
 * \file	con/server/src/gmode.h
 * \brief	Graphics mode initialization
 *
 * \date	2005
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef _GMODE_H
#define _GMODE_H

#include <l4/sys/l4int.h>

/** initialize graphics mode. */
void init_gmode(void);

extern l4_uint8_t* gr_vmem;
extern l4_uint8_t* gr_vmem_maxmap;
extern l4_size_t   gr_vmem_size;
extern l4_uint8_t* vis_vmem;            /**< vsbl. mem (>gr_vmem if panned) */
extern l4_offs_t   vis_offs;            /**< vis_vmem - gr_vmem */

extern l4_uint16_t VESA_XRES;           /**< pixels per line */
extern l4_uint16_t VESA_YRES;           /**< pixels per row */
extern l4_uint16_t VESA_YRES_CLIENT;    /**< pixels per row for clients */
extern l4_uint16_t VESA_BPL;            /**< bytes per line */
extern l4_uint8_t  VESA_BITS;           /**< bits per pixel */
extern l4_uint8_t  VESA_RES;            /**< reserved bits */
extern l4_uint8_t  VESA_RED_OFFS;       /**< red bits offset in pixel */
extern l4_uint8_t  VESA_GREEN_OFFS;     /**< green bits offset in pixel */
extern l4_uint8_t  VESA_BLUE_OFFS;      /**< blue bits offset in pixel */
extern l4_uint8_t  VESA_RED_SIZE;       /**< number of red bits per pixel */
extern l4_uint8_t  VESA_GREEN_SIZE;     /**< number of green bits per pixel */
extern l4_uint8_t  VESA_BLUE_SIZE;      /**< number of blue bits per pixel */
extern l4_uint8_t  FONT_XRES;           /**< x-pixels per font character */
extern l4_uint8_t  FONT_YRES;           /**< y-pixels per font character */
extern l4_uint32_t FONT_CHRS;           /**< number of characters in font */
extern int         panned;              /**< display is panned */
extern l4_umword_t accel_caps;
extern l4_uint32_t pan_offs_x;          /**< panned to position x */
extern l4_uint32_t pan_offs_y;          /**< panned to position y */

#endif /* !_GMODE_H */
