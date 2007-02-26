/* $Id$ */
/*****************************************************************************/
/**
 * \file   lib/drivers/input.h
 * \brief  Header for input driver backends for Qt/Embedded.
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

#ifndef DROPS_QWS_INPUT_H
#define DROPS_QWS_INPUT_H

void drops_qws_init_devices(void);
int  drops_qws_channel_mouse(void);
int  drops_qws_channel_keyboard(void);

int  drops_qws_get_mouse(int &x, int &y, int &button);
int  drops_qws_get_keyboard(int *key, int *press);
int  drops_qws_get_keymodifiers(int *alt, int *shift, int *ctrl, int *caps, int *num);

int  drops_qws_qt_key(int key);
int  drops_qws_qt_keycode(int key, int press);

void drops_qws_notify(void);

#endif

