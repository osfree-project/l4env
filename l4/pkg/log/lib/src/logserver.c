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
#include "log-client.h"

l4_threadid_t log_server;
       int initialized = 0;	// logserver found?
static int server_err = 0;	// error while looking for nameserver?
static int flush_flag = 0;	// flush at the logserver?

void LOG_server_outstring(const char *string);
void LOG_server_setid(l4_threadid_t id);
static void local_outs(const char *);

void (*LOG_outstring)(const char *) __attribute__ ((weak));
void (*LOG_outstring)(const char *) = LOG_server_outstring;

int check_server(void)
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
  int err;
  char *s = (char*)string;
  
  if (strlen(string) + 1 > LOG_BUFFERSIZE)
    s[LOG_BUFFERSIZE] = 0;

  check_server();

  err = 0;
  if (initialized)
    {
      CORBA_Environment env = dice_default_environment;
      log_outstring_call (&log_server, flush_flag, string, &env);
      if (DICE_HAS_EXCEPTION(&env))
        {
	  err = DICE_IPC_ERROR(&env);
        }
    }
  if (!initialized || err)    
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
