/* $Id$ */
/**
 * \file     l4util/include/ARCH-x86/mbi_argv.h
 * \brief    command line handling
 *
 * \date     2003
 * \author   Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef L4UTIL_MBI_ARGV
#define L4UTIL_MBI_ARGV

#include <l4/sys/l4int.h>
#include <l4/util/mb_info.h>
#include <l4/sys/compiler.h>

EXTERN_C_BEGIN

void l4util_mbi_to_argv(l4_mword_t flag, l4util_mb_info_t *mbi);

extern int  l4util_argc;
extern char *l4util_argv[];

EXTERN_C_END

#endif

