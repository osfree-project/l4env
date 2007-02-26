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
extern l4_uint8_t* vis_vmem;
extern l4_uint8_t* vis_offs;

extern l4_uint16_t VESA_XRES, VESA_YRES, VESA_BPL;
extern l4_uint8_t  VESA_BITS, VESA_RES;
extern l4_uint8_t  VESA_RED_OFFS, VESA_GREEN_OFFS, VESA_BLUE_OFFS;
extern l4_uint8_t  VESA_RED_SIZE, VESA_GREEN_SIZE, VESA_BLUE_SIZE;
extern l4_uint8_t  FONT_XRES;
extern l4_uint8_t  FONT_YRES;
extern l4_uint32_t FONT_CHRS;
extern int         panned;
extern l4_umword_t accel_caps;
extern l4_uint32_t pan_offs_x, pan_offs_y;

#endif /* !_GMODE_H */
