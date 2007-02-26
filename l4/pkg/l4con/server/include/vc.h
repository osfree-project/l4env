/* $Id$ */
/**
 * \file	con/server/include/vc.h
 * \brief	internals of `con' submodule, thread specific vc stuff
 *
 * \date	2001
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * 		Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef _VC_H
#define _VC_H

#include "con.h"

extern void vc_init(void);
extern void vc_loop(struct l4con_vc *this_vc);
extern void vc_show_id(struct l4con_vc *vc);
extern void vc_show_drops_cscs_logo(void);
extern void vc_clear(struct l4con_vc *vc);
extern int  vc_open(struct l4con_vc *vc, l4_uint32_t mode);
extern int  vc_close(struct l4con_vc *vc);

extern void vc_brightness_contrast(int diff_brightness, int diff_contrast);

extern pslim_copy_fn fg_do_copy;
extern pslim_copy_fn bg_do_copy;
extern pslim_fill_fn fg_do_fill;
extern pslim_fill_fn bg_do_fill;
extern pslim_sync_fn fg_do_sync;
extern pslim_sync_fn bg_do_sync;

extern void*     gr_vmem;
extern l4_addr_t gr_vbase;
extern l4_offs_t gr_voffs;

extern void*     vis_vmem;
extern l4_addr_t vis_vbase;
extern l4_offs_t vis_voffs;

#endif /* !_VC_H */

