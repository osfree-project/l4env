/*!
 * \file	loader/server/src/trampoline.h
 * \brief	startup code of a sigma0 application
 *
 * \date	2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __TRAMPOLINE_H_
#define __TRAMPOLINE_H_

void task_trampoline(l4_addr_t entry, void *mbi, void *env);
extern char _task_trampoline_end;

#endif

