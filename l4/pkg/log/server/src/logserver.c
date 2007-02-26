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

int	verbose=0;
unsigned buffer_size, buffer_head, buffer_tail;
#if CONFIG_USE_TCPIP
unsigned buffer_size = 4096;	/* we use a buffer size of 4KB per default
				   when logging to the net. */
#endif

//! A string-buffer containing the received string
static char message_buffer[LOG_BUFFERSIZE+5];
//! A structure containing the last received request
rcv_message_t message;

//! the sender of the last request
l4_threadid_t msg_sender = L4_INVALID_ID;

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
	    "requested buffersize of %d, shrinking to %d\n",
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

/* answer the last and get a new message via ipc, we use long ipc.
 *
 * We receive up to 1 flexpage, up to 1 string and up to 6 dwords.
 */
static int get_message(void){
  int err;
  
  message.size.md.strings = 1;
  message.size.md.dwords = 7;
  message.string.rcv_size = LOG_BUFFERSIZE;
  message.string.rcv_str = (l4_umword_t)&message_buffer;
#if CONFIG_USE_TCPIP
  message.fpage = channel_get_next_fpage();
#else
  message.fpage.fpage = 0;
#endif

  memset(message_buffer,0,LOG_BUFFERSIZE);
  while(1){
  	if(l4_thread_equal(msg_sender, L4_INVALID_ID)){
		if((err=l4_ipc_wait(&msg_sender,
                           &message, &message.d0, &message.d1,
			   L4_IPC_NEVER, &message.result))!=0)
		    return err;
		 break;
	} else {
		err = l4_ipc_reply_and_wait(
		        msg_sender, 0, message.d0, 0,
			&msg_sender, &message, &message.d0, &message.d1,
			L4_IPC_SEND_TIMEOUT_0,
			&message.result);
		if(err & L4_IPC_SETIMEOUT){
			msg_sender = L4_INVALID_ID;
		} else
			break;
	}
  }
  LOGd(CONFIG_LOG_REQUESTS,
	  "Request from %x.%x: %d strings, size %d, %d dwords, flexpage: %s\n",
	  msg_sender.id.task, msg_sender.id.lthread,
	  message.result.md.strings,
	  message.string.snd_size,
	  message.result.md.dwords,
	  message.result.md.fpage_received?"yes":"no");
  return 0;
}

static void print_message(void){
  if(verbose && message_buffer[0]){
      char buf[10];
      sprintf(buf, l4util_idfmt_adjust":", l4util_idstr(msg_sender));
      print_buffered(buf);
  }
  print_buffered(message_buffer);
  if(strstr(message_buffer, "***")) flush_buffer();
}

int main(int argc, const char**argv){
  int err;

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

  strcpy(message_buffer+LOG_BUFFERSIZE,"...\n");
  while(1){
    err = get_message();
    if (err == 0    /* no error */
     || err == 0xE0 /* L4_IPC_REMSGCUT: most probably only the logstring
		     * is too long, this case is signaled by the appended
		     * "...\n", see the strcpy line above */
	){
	if(message.result.md.fpage_received == 0){
	    switch(message.d0){
	    case LOG_COMMAND_LOG:
		if(message.result.md.strings!=1){
#if CONFIG_USE_TCPIP
		  // aw9: Hack for kernel tracebuffer
		  //      (no fpage mapped)
		  if ((message.d1 & L4_PAGEMASK) >= 0xc0000000) {
		    message.d0 = channel_open();
		    continue;
		  }
#endif
		    message.d0 = -1;
		    continue;
		}
		print_message();
		if(message.d1) flush_buffer();
		message.d0 = 0;
		continue;
#if CONFIG_USE_TCPIP
	    case LOG_COMMAND_CHANNEL_WRITE:
		if(message.result.md.dwords!=4){
		    message.d0 = -1;
		    continue;
		}
		message.d0 = channel_write();
		continue;
	    case LOG_COMMAND_CHANNEL_FLUSH:
		message.d0 = channel_flush();
		continue;
	    case LOG_COMMAND_CHANNEL_CLOSE:
		message.d0 = channel_close();
		continue;
#endif
	    }
	} else {
#if CONFIG_USE_TCPIP
	    // we received a flexpage. This is an open-request.
	    message.d0 = channel_open();
	    continue;
#endif
	}
	/* We have an invalid request. Signal this to the caller. */
	message.d0 = -1;
	continue;
    } // no error during request receive

    if(err & 0x40){	// ipc aborted, ignore
	continue;
    }
    
    LOG_Error("Error %#x getting message from %x.%x.\n", err,
	  msg_sender.id.task, msg_sender.id.lthread);
    // send no reply
    msg_sender = L4_INVALID_ID;
  }
}
