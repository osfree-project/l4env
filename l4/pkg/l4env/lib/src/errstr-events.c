/*!
 * \file   l4env/lib/src/errstr-events.c
 * \brief  Error string handling for events package, which is "below" l4env
 *
 * \date   08/18/2004
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

/* L4 includes */
#include <l4/crtx/ctor.h>
#include <l4/events/events.h>
#include <l4/env/errno.h>

/* symbol that can be referenced to suck in this object */
asm("l4events_err_strings_sym:");

/* L4 IPC errors */
L4ENV_ERR_DESC_STATIC(err_strings,
	L4EVENTS_ERRSTRINGS_DEFINE
);

static void register_ipc_codes(void)
{
  l4env_err_register_desc(&err_strings);
}

L4C_CTOR(register_ipc_codes, L4CTOR_BEFORE_BACKEND);
