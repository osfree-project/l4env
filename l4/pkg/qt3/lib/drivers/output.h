/* $Id$ */
/*****************************************************************************/
/**
 * \file   lib/drivers/output.h
 * \brief  Header for framebuffer driver backends for Qt/Embedded.
 *
 * \date   10/24/2004
 * \author Josef Spillner <js177634@inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2004-2005 Technische Universitaet Dresden
 * This file is part of the Qt3 port for L4/DROPS, which is distributed under
 * the terms of the GNU General Public License 2. Please see the COPYING file
 * for details.
 */

#ifndef DROPS_QWS_OUTPUT_H
#define DROPS_QWS_OUTPUT_H

long  drops_qws_set_screen(long width, long height, long depth);

long  drops_qws_get_scr_width(void);
long  drops_qws_get_scr_height(void);
long  drops_qws_get_scr_depth(void);
long  drops_qws_get_scr_line(void);
void *drops_qws_get_scr_adr(void);

void  drops_qws_refresh_screen(void);

#endif

