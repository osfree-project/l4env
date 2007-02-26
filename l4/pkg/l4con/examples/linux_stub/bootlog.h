/*!
 * \file	con/examples/linux_stub/bootlog.h
 * \brief	
 *
 * \date	01/2002
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 'Technische Universitaet Dresden'
 * This file is part of the con package, which is distributed under
 * the terms of the GNU General Public License 2. Please see the
 * COPYING file for details. */

#ifndef __CON_EXAMPLES_LINUX_STUB_BOOTLOG_H_
#define __CON_EXAMPLES_LINUX_STUB_BOOTLOG_H_

extern unsigned dropscon_bootlog_init_done;
extern char dropscon_bootlog_read(void);
extern void dropscon_bootlog_done(void);

#endif

