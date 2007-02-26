/* $Id$ */
/**
 * \file	con/server/include/con_config.h
 * \brief	con configuration macros
 *
 * \date	2001
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 * 		Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

/* OSKit malloc */
#define CONFIG_MALLOC_MAX_SIZE	(1024*1024)

/* visual --- unused */
#define CONFIG_XRES		640
#define CONFIG_YRES		480
#define CONFIG_MY_BITS		16

/* vc */
#define CONFIG_MAX_VC		5		/* number of virtual consoles */
#define CONFIG_MAX_SBUF_SIZE	(4*256*256)	/* max string buffer */

/* ev */
#define IRQ_HANDLER_STACKSIZE	0x2000

