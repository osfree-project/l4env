/*!
 * \file	log/server/src/logserver.c
 * \brief       Log-Server main
 *
 * \date	03/02/2001
 * \author	Jork Loeser <jork.loeser@inf.tu-dresden.de>
 *
 * Buffering issues: We allow buffering of output to speed up logging.
 *
 * Currently, we do not deal with dynamic memory allocation, we use a
 * static array, allocated at compile-time. What would be the use of
 * dynamic allocation? As of today (03/01/2001), malloc() uses a
 * pre-allocated memory area, allocated from simple-dm prior to the
 * start of main(). So, this memory would be allocated anyway. Thus we
 * save the effort and use the static array.
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/rmgr/librmgr.h>
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/util/parse_cmd.h>
#include <l4/util/l4_macros.h>

#include "../include/log_comm.h"
#include "stuff.h"
#include "flusher.h"
#include "tcpip.h"
#include "config.h"
#include "muxed.h"
#include "log-server.h"

int	verbose=0;
unsigned buffer_size, buffer_head, buffer_tail;
#if CONFIG_USE_TCPIP
unsigned buffer_size = 4096;	/* we use a buffer size of 4KB per default
				   when logging to the net. */
#endif

//! A string-buffer containing the received string
static char message_buffer[LOG_BUFFERSIZE+5];

//! Our own logstring
char LOG_tag[9]="log";

int	flush_local=1;
int	flush_serial;
int	serial_esc;

#if CONFIG_USE_TCPIP
bin_conn_t bin_conns[MAX_BIN_CONNS];
int	flush_to_net=0;		// should we actually flush to the net?
int	flush_muxed=1;		/* should we use multiplexed mode on
				   net-output? */
#endif

int prio = 0x20;

char buffer_array[OUTPUT_BUFFER_SIZE];

#define min(a,b) ((a)<(b)?(a):(b))

/* management for buffered output */
static void init_buffer(void){
  if(buffer_size && buffer_size < 10) buffer_size = 10;

  if(buffer_size > sizeof(buffer_array)){
    LOGd(CONFIG_LOG_NOTICE,
	    "requested buffersize of %d, shrinking to %zd\n",
	    buffer_size, sizeof(buffer_array));
    buffer_size = sizeof(buffer_array);
  }
  buffer_head = buffer_tail = 0;
  buffer_array[buffer_head] = 0;
#if CONFIG_USE_TCPIP
  memset(bin_conns, 0, sizeof(bin_conns));
#endif
}

/* If buffering is enabled, try to print a string into the internal buffer.
   If it doesnt fit, flush the buffer and try again. */
static void print_buffered(const char*data){
  int	len;
  int	cur_len;
  int	tail;
  int	free;

  len = strlen(data);
  if(buffer_size){
      while(len>0){
	  tail = buffer_tail;
	  if(tail>buffer_head) free = tail-buffer_head-1;
	  else free = buffer_size-buffer_head+tail - 1;

	  assert(free>=0);
	  LOGd(CONFIG_LOG_RINGBUFFER,
		  "head=%d, tail=%d, size=%d, free=%d, len=%d\n",
		  buffer_head, buffer_tail, buffer_size, free, len);

	  if(free==0){
	      flush_buffer();
	      continue;
	  }

	  if(tail>buffer_head){  // free area is between head and tail
	      cur_len = min(free,len);
	      assert(cur_len>0);
	      LOGd(CONFIG_LOG_RINGBUFFER,
		      "filling %d+%d\n", buffer_head, cur_len);
	      memcpy(buffer_array+buffer_head, data, cur_len);
	  } else {
	      /* head >= tail, free area wraps arround. We only write the
		 topmost bytes and cycle again. We can write at least 1 byte,
		 to the topmost bytes, but we must leave one byte between
		 head and tail! If tail==0, this byte must be the last byte,
		 otherwise somewhere in the beginning of the buffer.
	      */
	      cur_len = buffer_size-buffer_head-(tail==0);
	      cur_len = min(cur_len,len);
	      assert(cur_len>0);
	      LOGd(CONFIG_LOG_RINGBUFFER,
		      "filling %d+%d\n", buffer_head, cur_len);
	      memcpy(buffer_array+buffer_head, data, cur_len);
	  }
	  len-=cur_len;
	  data+=cur_len;
	  if(buffer_head+cur_len == buffer_size) buffer_head = 0;
	  else buffer_head+=cur_len;
	  assert(buffer_head<buffer_size);
      }
  } else {
      if(flush_local){
	  outstring(data);
      }
#if CONFIG_USE_SERIAL
      if(flush_serial) serial_flush(data, strlen(data));
#endif
  }
}

static void parse_args(int argc, const char**argv){
    parse_cmdline(&argc, &argv,
		  'v', "verbose", "verbose mode",
		  PARSE_CMD_SWITCH, 1, &verbose,
#if CONFIG_USE_TCPIP
		  ' ', "net", "use TCP/IP network connections",
		  PARSE_CMD_SWITCH, 1, &flush_to_net,
		  ' ', "nonet", "do not use TCP/IP network connections",
		  PARSE_CMD_SWITCH, 0, &flush_to_net,
		  'm', "muxed", "flush in muxed mode to TCP/IP",
		  PARSE_CMD_SWITCH, 1, &flush_muxed,
		  'M', "nomuxed", "do not flush in muxed mode",
		  PARSE_CMD_SWITCH, 0, &flush_muxed,
		  ' ', "ip", "IP address to use",
		  PARSE_CMD_STRING, "", &ip_addr,
#endif
		  'l', "local", "flush to local console",
		  PARSE_CMD_SWITCH, 1, &flush_local,
		  'L' ,"nolocal", "do not flush to local console",
		  PARSE_CMD_SWITCH, 0, &flush_local,
#if CONFIG_USE_SERIAL
		  's' ,"comport", "flush to specified serial interface",
		  PARSE_CMD_INT, 0, &flush_serial,
		  'e', "serial-esc", "enter kdebug on esc on serial",
		  PARSE_CMD_SWITCH, 1, &serial_esc,
#endif
		  'b', "buffer", "buffered mode",
		  PARSE_CMD_INT, 0, &buffer_size,
		  ' ', "flushprio", "priority of flusher thread",
		  PARSE_CMD_INT, 0x20, &flusher_prio,
		  'p', "prio", "priority of main thread",
		  PARSE_CMD_INT, 0x20, &prio,
		  0);

}

void
log_outstring_component (CORBA_Object _dice_corba_obj,
    int flush_flag,
    const char* str,
    CORBA_Server_Environment *_dice_corba_env)
{
    if (verbose && str && str[0])
    {
      char buf[10];
      sprintf(buf, l4util_idfmt_adjust":", l4util_idstr(*_dice_corba_obj));
      print_buffered(buf);
    }
    print_buffered(str);
    if(flush_flag ||
	strstr(message_buffer, "***"))
	flush_buffer();
}

int
log_channel_open_component (CORBA_Object _dice_corba_obj,
    l4_snd_fpage_t page,
    int channel,
    CORBA_Server_Environment *_dice_corba_env)
{
#if CONFIG_USE_TCPIP
    int ret = channel_open(page.fpage, channel);
    _dice_corba_env->rcv_fpage = channel_get_next_fpage();
    return ret;
#else
    return -1;
#endif
}

int
log_channel_write_component (CORBA_Object _dice_corba_obj,
    int channel, 
    unsigned int offset,
    unsigned int size,
    CORBA_Server_Environment *_dice_corba_env)
{
#if CONFIG_USE_TCPIP
    return channel_write(channel, offset, size);
#else
    return -1;
#endif
}

int
log_channel_flush_component (CORBA_Object _dice_corba_obj,
    int channel,
    CORBA_Server_Environment *_dice_corba_env)
{
#if CONFIG_USE_TCPIP
    return channel_flush(channel);
#else
    return -1;
#endif
}

int
log_channel_close_component (CORBA_Object _dice_corba_obj,
    int channel,
    CORBA_Server_Environment *_dice_corba_env)
{
#if CONFIG_USE_TCPIP
    return channel_close(channel);
#else
    return -1;
#endif
}

static void*
msg_alloc(unsigned long size)
{
    if (size != LOG_BUFFERSIZE)
	return 0;
    return message_buffer;
}

int main(int argc, const char**argv){
  int err;
  CORBA_Server_Environment env = dice_default_server_environment;

  parse_args(argc,argv);

  init_buffer();
  
  rmgr_init();
  err = rmgr_set_prio(l4_myself(), prio);
  if(err){
      LOG_Error("rmgr_set_prio(%d): %d", prio, err);
      return 1;
  }

  err = flusher_init();
  if(err){
      LOG_Error("Serious problems initalizing the flusher.\n");
  }

  if(names_register(LOG_NAMESERVER_NAME)==0){
    LOG_Error("Cannot register at nameserver, falling asleep\n");
    return 1;
  }
  if(verbose){
      printf("Server started and registered as \"%s\"\n",
	      LOG_NAMESERVER_NAME);
#if CONFIG_USE_TCPIP
    printf("Logging to: %s%s%s\n",
	   flush_local?"console ":"",
	   flush_to_net?"network ":"",
	   flush_serial?"serial ":""
	);
#else
    printf("Logging to: %s%s\n",
	   flush_local?"kdebug ":"",
	   flush_serial?"serial":"");
#endif
    printf("Buffersize: %d\n", buffer_size);
  }

#if CONFIG_USE_TCPIP
  env.rcv_fpage = channel_get_next_fpage();
#else
  env.rcv_fpage.fpage = 0;
#endif
  env.malloc = msg_alloc;

  strcpy(message_buffer+LOG_BUFFERSIZE,"...\n");
  log_server_loop (&env);
}
