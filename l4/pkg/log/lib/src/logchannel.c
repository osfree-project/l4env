/*!
 * \file   log/lib/src/logchannel.c
 * \brief  Communicate with the logserver, printf output-function
 *
 * \date   1999/09/15
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/sys/types.h>
#include "log_comm.h"
#include "internal.h"
#include <l4/log/l4log.h>
#include <l4/log/server.h>
#include <l4/env/errno.h>
#include "log-client.h"

/*!\brief Open a binary logging connection
 *
 * \param channel	the channel to open
 * \param page		an fpage containing buffer memory used with
 *			LOG_channel_write() later
 *
 * \return \see channel_open()
 *
 * Marshalling:
 * - d0/d1 fpage
 * - d2/d3 0
 * - d4	   channel
 */
int LOG_channel_open(int channel, l4_fpage_t page)
{
  int ret;
  l4_snd_fpage_t f;
  CORBA_Environment env = dice_default_environment;

  if (page.fp.size > LOG_LOG2_CHANNEL_BUFFER_SIZE)
    return -L4_ENOMEM;
  if (check_server())
    return -L4_ENOTFOUND;

  f.snd_base = 0;
  f.fpage = page;
  ret = log_channel_open_call (&log_server, f, channel, &env);
  if (DICE_HAS_EXCEPTION(&env))
    return -DICE_IPC_ERROR(&env);
  
  return ret;
}

/*!\brief Write data to a binary logging connection
 *
 * \param id		connection identifier
 * \param off		offset of data in the connection buffer
 * \param size		size of data to write
 *
 * \return \see channel_write()
 *
 * Marshalling:
 * - d1 channel id
 * - d2 off
 * - d3 length
 */
int LOG_channel_write(int id, unsigned off, unsigned size)
{
  int ret;
  CORBA_Environment env = dice_default_environment;

  if (!initialized)
    return -L4_EIPC;

  ret = log_channel_write_call (&log_server, id, off, size, &env);
  if (DICE_HAS_EXCEPTION(&env))
    return -DICE_IPC_ERROR(&env);

  return ret;
}

/*!\brief Flush a binary logging connection
 *
 * \param id		connection identifier
 *
 * \return \see channel_flush()
 *
 * Marshalling:
 * - d1 channel id
 */
int LOG_channel_flush(int id)
{
  int ret;
  CORBA_Environment env = dice_default_environment;

  if (!initialized)
    return -L4_EIPC;

  ret = log_channel_flush_call (&log_server, id, &env);
  if (DICE_HAS_EXCEPTION(&env))
    return -DICE_IPC_ERROR(&env);
  
  return ret;
}

/*!\brief Close a binary logging connection
 *
 * \param id		connection identifier
 *
 * \return \see channel_close()
 *
 * Marshalling:
 * - d1 channel id
 */
int LOG_channel_close(int id)
{
  int ret;
  CORBA_Environment env = dice_default_environment;

  if (!initialized)
    return -L4_EIPC;

  ret = log_channel_close_call (&log_server, id, &env);
  if (DICE_HAS_EXCEPTION(&env))
    return -DICE_IPC_ERROR(&env);
  
  return ret;
}
