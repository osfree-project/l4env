/* $Id$ */

/*!
 * \file	con/include/l4/con/l4con_ev.h
 *
 * \brief	con protocol definitions - input event part
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 * These macros are used as parameters for the IDL functions.
 */

#ifndef _L4CON_L4CON_EV_H
#define _L4CON_L4CON_EV_H

/* we use the original `input' event definitions */
/* except: */

/** Event type */
#define EV_CON		0x15

/** EV_CON event codes */
#define EV_CON_RESERVED    0		/**< invalid request */
#define EV_CON_REDRAW      1		/**< requests client redraw */
#define EV_CON_BACKGROUND  2		/**< tells client that it looses
					     the framebuffer */

#endif

