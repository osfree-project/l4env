/* $Id$ */
/**
 * \file	con/server/src/gmode.c
 * \brief	graphics mode initialization
 *
 * \date	2005
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2005 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#include "gmode.h"

l4_uint16_t VESA_XRES, VESA_YRES, VESA_YRES_CLIENT, VESA_BPL;
l4_uint8_t  VESA_BITS, VESA_RES;
l4_uint8_t  VESA_RED_OFFS, VESA_GREEN_OFFS, VESA_BLUE_OFFS;
l4_uint8_t  VESA_RED_SIZE, VESA_GREEN_SIZE, VESA_BLUE_SIZE;
l4_uint8_t  FONT_XRES, FONT_YRES;
l4_uint32_t FONT_CHRS;

l4_umword_t accel_caps  = 0;

int         panned;		/**< Display already panned? */
l4_uint32_t pan_offs_x;		/**< x offset for panning */
l4_uint32_t pan_offs_y;		/**< y offset for panning */

l4_uint8_t  *gr_vmem;		/**< Linear video framebuffer. */
l4_uint8_t  *gr_vmem_maxmap;    /**< don't map fb beyond this address. */
l4_size_t    gr_vmem_size;      /**< Size of video framebuffer. */
l4_uint8_t  *vis_vmem;		/**< Visible video framebuffer. */
l4_offs_t    vis_offs;		/**< vis_vmem - gr_vmem. */
