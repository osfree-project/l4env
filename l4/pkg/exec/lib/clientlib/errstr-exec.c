/*!
 * \file   exec/lib/clientlib/errstr-exec.c
 * \brief  Error string handling
 *
 * \date   08/2004
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. 
 */

#include <l4/crtx/ctor.h>
#include <l4/env/errno.h>
#include <l4/exec/errno.h>

#ifndef NULL
#define NULL (void*)0
#endif

asm("l4exec_err_strings_sym:");

L4ENV_ERR_DESC_STATIC(err_strings,
    { L4_EXEC_BADFORMAT,   "EXEC: Invalid file format" },
    { L4_EXEC_BADARCH,     "EXEC: Invalid ELF architecture" },
    { L4_EXEC_CORRUPT,     "EXEC: ELF file corrupt" },
    { L4_EXEC_NOSTANDARD,  "EXEC: Entry point not found" },
    { L4_EXEC_LINK,        "EXEC: Linker error(s)" },
    { L4_EXEC_INTERPRETER, "EXEC: Binary depends on libld-l4.s.so" },
);

static void
register_err_codes(void)
{
  l4env_err_register_desc(&err_strings);
}

L4C_CTOR(register_err_codes, L4CTOR_BEFORE_BACKEND);
