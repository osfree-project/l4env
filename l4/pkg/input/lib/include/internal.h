/* $Id$ */
/*****************************************************************************/
/**
 * \file   input/lib/include/internal.h
 * \brief  L4INPUT internal interface
 *
 * \date   11/20/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __INPUT_LIB_INCLUDE_INTERNAL_H_
#define __INPUT_LIB_INCLUDE_INTERNAL_H_

#include <l4/input/libinput.h>

/** SUBSYSTEMS/EMULATION **/
void l4input_internal_irq_init(int omega0, int prio);
void l4input_internal_jiffies_init(void);

/** L4EVDEV input event handler **/
int  l4input_internal_l4evdev_init(void (*cb)(struct l4input *));
void l4input_internal_l4evdev_exit(void);
void l4input_internal_wait_init(void);


/** INPUT SYSTEM **/
int  l4input_internal_atkbd_init(void);
int  l4input_internal_psmouse_init(void);
int  l4input_internal_i8042_init(void);
int  l4input_internal_input_init(void);
int  l4input_internal_serio_init(void);
int  l4input_internal_pcspkr_init(void);

void l4input_internal_atkbd_exit(void);
void l4input_internal_psmouse_exit(void);
void l4input_internal_i8042_exit(void);
void l4input_internal_input_exit(void);
void l4input_internal_serio_exit(void);
void l4input_internal_pcspkr_exit(void);

#endif

