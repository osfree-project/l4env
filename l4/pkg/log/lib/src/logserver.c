/*!
 * \file   log/lib/src/logserver.c
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
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/names/libnames.h>
#include "log_comm.h"
#include "internal.h"
#include <l4/log/l4log.h>
#include <l4/env/errno.h>

l4_threadid_t log_server;
static int initialized = 0;	// logserver found?
static int server_err = 0;	// error while looking for nameserver?
static int flush_flag = 0;	// flush at the logserver?

void LOG_server_outstring(const char *string);
void LOG_server_setid(l4_threadid_t id);
static void local_outs(const char *);

void (*LOG_outstring)(const char *) __attribute__ ((weak));
void (*LOG_outstring)(const char *) = LOG_server_outstring;

static int check_server(void)
{
  if (!initialized)
    {
      if (!server_err)
        {
          /* first time: try to resolve logserver name */
          if (names_waitfor_name(LOG_NAMESERVER_NAME, &log_server,
                LOG_TIMEOUT_NAMESERVER) == 0)
            {
              server_err = 1;
            }
          else
            {
              initialized=1;
            }
        }
      else
        {
          /* Maybe logserver registered now ? */
          if (names_query_name(LOG_NAMESERVER_NAME, &log_server) == 1)
            {
              initialized = 1;
            }
        }
    }
  return !initialized;
}

/*!\brief Send a message to the logserver
 *
 * \param string	the string to send
 *
 * Marshalling:
 * - string contains the string
 */
void LOG_server_outstring(const char *string)
{
  struct {
    l4_fpage_t fpage;
    l4_msgdope_t size;
    l4_msgdope_t snd;
    l4_umword_t d0,d1;
    l4_strdope_t string;
  } msg = {
    .fpage  = { .raw     = 0},
    .size   = { .md      = { .strings = 1, .dwords = 2}},
    .snd    = { .md      = { .strings = 1, .dwords = 2}},
    .string = { .snd_str = (l4_umword_t)string }
  };
  l4_msgdope_t result;
  int err;

#ifdef WORKAROUND_L4BUG_LONGIPC_STRINGLENGTH
  char buf[4];
  if (strlen(string) < 4)
    {
      strcpy(buf, string);
      msg.string.snd_str = (l4_umword_t)&buf;
      msg.string.snd_size = sizeof(buf);
    }
  else
#endif
  if ((msg.string.snd_size = strlen(string) + 1) > LOG_BUFFERSIZE)
    msg.string.snd_size = LOG_BUFFERSIZE;

  check_server();

  err = 0;
  if (!initialized ||
#if 1
      ((err = l4_ipc_call(log_server, &msg, LOG_COMMAND_LOG, flush_flag,
		          NULL, &msg.d0, &msg.d1,
		          L4_IPC_NEVER, &result)) != 0)
#else
     ((err = l4_ipc_send(log_server, &msg, LOG_COMMAND_LOG, flush_flag,
                         //L4_IPC_SEND_TIMEOUT_0,
                         L4_IPC_NEVER, &result)) != 0)
#endif
     )
    {
      local_outs(string);
    }
}

/* direct way to set the server-id */
void LOG_server_setid(l4_threadid_t id)
{
  log_server = id;
  initialized = 1;
}

/* fallback function when server is not found */
static void local_outs(const char*s)
{
  /* indicate local output */
  if (*s)
    {
      outchar('*');
      outstring(s);
    }
}

/* Flush the buffered data
 *
 * This flushs the printf-buffer and calls flush at the logserver. As soon as
 * we have file descriptors, only flushing at the server is done.
 */
void LOG_flush(void)
{
  flush_flag++;
  LOG_printf_flush();
  flush_flag--;
}

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
  struct {
    l4_fpage_t fpage;
    l4_msgdope_t size;
    l4_msgdope_t snd;
    l4_umword_t d0,d1, d2, d3, d4;
  } msg = {
      .fpage = page,
      .d2    = 0,
      .d3    = 0,
      .d4    = channel,
      .size  = { .md = { .strings = 0, .dwords = 5}},
      .snd   = { .md = { .strings = 0, .dwords = 5}}
  };
  l4_msgdope_t result;
  int err;

  if (page.fp.size > LOG_LOG2_CHANNEL_BUFFER_SIZE)
    return -L4_ENOMEM;
  if (check_server())
    return -L4_ENOTFOUND;
  if ((err = l4_ipc_call(log_server,
                         (void*)(((l4_umword_t)&msg) | L4_IPC_FPAGE_MASK),
                         0,          // offset
                         page.fpage, // fpage
                         NULL, &msg.d0, &msg.d1,
                         L4_IPC_NEVER, &result)) != 0)
    return -err;
  return msg.d0;
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
  struct {
    l4_fpage_t fpage;
    l4_msgdope_t size;
    l4_msgdope_t snd;
    l4_umword_t d0, d1,d2,d3;
  } msg= {
    .fpage = { .raw = 0},
    .size  = { .md  = { .strings = 0, .dwords = 4}},
    .snd   = { .md  = { .strings = 0, .dwords = 4}},
    .d2    = off,
    .d3    = size
  };
  l4_msgdope_t result;
  int err;

  if (!initialized)
    return -L4_EIPC;

  if ((err = l4_ipc_call(log_server, &msg, LOG_COMMAND_CHANNEL_WRITE, id,
                         NULL, &msg.d0, &msg.d1,
                         L4_IPC_NEVER,&result)) != 0)
    return -err;

  return msg.d0;
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
  unsigned dw0, dw1;
  l4_msgdope_t result;
  int err;

  if (!initialized)
    return -L4_EIPC;

  if ((err = l4_ipc_call(log_server, NULL, LOG_COMMAND_CHANNEL_FLUSH, id,
                         NULL, &dw0, &dw1,
                         L4_IPC_NEVER,&result)) != 0)
    return -err;
  return dw0;
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
  unsigned dw0, dw1;
  l4_msgdope_t result;
  int err;

  if (!initialized)
    return -L4_EIPC;

  if ((err = l4_ipc_call(log_server, NULL, LOG_COMMAND_CHANNEL_CLOSE, id,
                         NULL, &dw0, &dw1,
                         L4_IPC_NEVER,&result)) != 0)
    return -err;
  return dw0;
}
