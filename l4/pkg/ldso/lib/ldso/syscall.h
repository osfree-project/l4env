/**
 * \file   ldso/lib/ldso/syscall.h
 * \brief  Fixup for non-PIC direct syscalls in shared libraries.
 *
 * \date   01/2006
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2005 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef _SYSCALL_H_
#define _SYSCALL_H_

int _dl_syscall_fixup(const char *symname, unsigned long *reloc_addr);

#endif
