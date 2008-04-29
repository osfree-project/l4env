/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_ts/clientlib/src/server.c
 * \brief  Init client library.
 *
 * \date   04/2004
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/log/l4log.h>
#include <l4/env/errno.h>
#include <l4/names/libnames.h>
#include <l4/generic_ts/generic_ts.h>

#include "debug.h"

l4_threadid_t l4ts_server_id;

int
l4ts_connect(void)
{
  static int dont_try_again;

  if (!dont_try_again)
    names_waitfor_name("SIMPLE_TS", &l4ts_server_id, 10000);

  dont_try_again = 1;
  return l4_is_nil_id(l4ts_server_id) ? -L4_ENOTFOUND : 0;
}

l4_threadid_t
l4ts_server(void)
{
  return l4ts_server_id;
}
