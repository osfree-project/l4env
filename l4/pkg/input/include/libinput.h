/* $Id$ */
/*****************************************************************************/
/**
 * \file   input/include/libinput.h
 * \brief  Input event library (L4INPUT) API
 *
 * \date   11/20/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 *
 * Original copyright notice from include/linux/input.h follows...
 */
/*
 * Copyright (c) 1999-2002 Vojtech Pavlik
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#ifndef __INPUT_INCLUDE_LIBINPUT_H_
#define __INPUT_INCLUDE_LIBINPUT_H_

#include <l4/input/macros.h>

#ifdef __cplusplus
extern "C" {
#endif


struct l4input {
        long long time; ///< unused on bare hardware, used in Fiasco-UX
	unsigned short type;
	unsigned short code;
	int value;
};

/** Initialize input driver library.
 *
 * \param omega0    if unset use RMGR / if set use omega0 for IRQs
 * \param prio      if != L4THREAD_DEFAULT_PRIO use as prio for irq threads
 * \param handler   if !NULL use this function on event occurence as
 *                  callback
 *
 * libinput works in 2 different modes:
 *
 * -# input events are stored in library local ring buffers until
 * l4input_flush() is called.
 *
 * -# on each event occurrence \a handler is called and no local event
 * buffering is done.
 */
int l4input_init(int omega0, int prio, void (*handler)(struct l4input *));

/** Query event status.
 *
 * \return 0 if there are no pending events; !0 otherwise
 */
int l4input_ispending(void);

/** Get events.
 *
 * \param buffer    event return buffer
 * \param count     max number of events to return 
 *
 * Returns up to \a count events into buffer.
 */
int l4input_flush(void *buffer, int count);

/** Program PC speaker
 *
 * \param tone      tone value (0 switches off)
 */
int l4input_pcspkr(int tone);

#ifdef __cplusplus
}
#endif

#endif

