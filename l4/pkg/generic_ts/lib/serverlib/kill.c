
/*****************************************************************************/
/**
 * \file   generic_ts/serverlib/kill.c
 * \brief  Reply after a Kill.
 *
 * \date   06/2004
 * \author  <frenzel3@os.inf.tu-dresden.de>
 */
/*****************************************************************************/

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/generic_ts/generic_ts-client.h>
#include <l4/generic_ts/generic_ts_server.h>

void
l4ts_do_kill_reply(l4_threadid_t *src, l4_threadid_t* client)
{
  CORBA_Environment env = dice_default_environment;
  
  l4_ts_do_kill_reply_call(src, client, &env);
}

