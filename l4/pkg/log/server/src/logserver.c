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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <l4/sys/types.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/rmgr/librmgr.h>
#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/log/l4log.h>
#include <assert.h>

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

#if CONFIG_USE_TCPIP
bin_conn_t bin_conns[MAX_BIN_CONNS];
int	flush_to_net=0;		// should we actually flush to the net?
int	flush_muxed=1;		/* should we use multiplexed mode on
				   net-output? */
#endif

int prio = 0x20;
int flusher_prio = 0x20;

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
      const char *x=data;
      
      while(*x)outchar(*x++);
  }
}

static void parse_args(int argc,char**argv){
  argc--;argv++;
  
  while(argc>0){
    if(!strcmp(argv[0],"--verbose")){
      verbose=1;
      argc--;
      argv++;
    }else if(!strcmp(argv[0],"--net")){
      argc--;
      argv++;
#if CONFIG_USE_TCPIP
      flush_to_net=1;
#else
      LOG_Error("Logging to the network requested by commandline, but not compiled in!\n");
#endif
    }else if(!strcmp(argv[0],"--nonet")){
      argc--;
      argv++;
#if CONFIG_USE_TCPIP
      flush_to_net=0;
#endif
    }else if(!strcmp(argv[0],"--local")){
      argc--;
      argv++;
      flush_local=1;
    }else if(!strcmp(argv[0],"--nolocal")){
      argc--;
      argv++;
      flush_local=0;
    }else if(!strcmp(argv[0],"--buffer")){
      argc--;
      argv++;
      if(argc>0){
	char*eptr;

	buffer_size = strtoul(*argv, &eptr, 0);
	if(*eptr){
	  LOG_Error("invalid numerical argument to --buffer: %s\n",
		*argv);
	  buffer_size = 0;
	}
	argc--;
	argv++;
      } else {
	LOG_Error("no argument to --buffer\n");
      }
    }else if(!strcmp(argv[0],"--muxded")){
      argc--;
      argv++;
#if CONFIG_USE_TCPIP
      flush_muxed=1;
#else
      LOG_Error("Flushing in muxed mode requested by commandline, but not compiled in!\n");
#endif
    }else if(!strcmp(argv[0],"--nomuxded")){
      argc--;
      argv++;
#if CONFIG_USE_TCPIP
      flush_muxed=0;
#endif
    }else if(!strcmp(argv[0],"--flushprio")){
      argc--;
      argv++;
      if(argc>0){
	char*eptr;

	flusher_prio = strtoul(*argv, &eptr, 0);
	if(*eptr){
	  LOG_Error("invalid numerical argument to --flushprio: %s\n",
		*argv);
	  flusher_prio = 0x20;
	}
	argc--;
	argv++;
      } else {
	LOG_Error("no argument to --flushprio\n");
      }
    }else if(!strcmp(argv[0],"--prio")){
      argc--;
      argv++;
      if(argc>0){
	char*eptr;

	prio = strtoul(*argv, &eptr, 0);
	if(*eptr){
	  LOG_Error("invalid numerical argument to --prio: %s\n",
		*argv);
	  prio = 0x20;
	}
	argc--;
	argv++;
      } else {
	LOG_Error("no argument to --prio\n");
      }
    } else if(!strcmp(argv[0],"--ip")){
      argc--;
      argv++;
      if(argc>0){
#if CONFIG_USE_TCPIP
	ip_addr=argv[0];
	if(netmask==NULL) netmask="255.255.255.0";
#else
	LOG_Error("IP-Addr specified on command-line, but no network support compiled in!\n");
#endif
	argc--;
	argv++;
      } else {
	LOG_Error("no argument to --ip\n");
      }
    }else{
      LOG_Error("unknown cmd-arg %s\n",argv[0]);
      argc--;argv++;
    }
  }
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
		if((err=l4_i386_ipc_wait(&msg_sender,
                           &message, &message.d0, &message.d1,
			   L4_IPC_TIMEOUT(0,0,0,0,0,0),
			   &message.result))!=0)
		    return err;
		 break;
	} else {
		err = l4_i386_ipc_reply_and_wait(
			msg_sender, NULL, message.d0, 0,
			&msg_sender, &message, &message.d0, &message.d1,
			L4_IPC_TIMEOUT(0,1,0,0,0,0),
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
      sprintf(buf, "%03X.%02X:",msg_sender.id.task,msg_sender.id.lthread);
      print_buffered(buf);
  }
  print_buffered(message_buffer);
  if(strstr(message_buffer, "***")) flush_buffer();
}

int main(int argc,char**argv){
  int err;

  /*we do not use the log-lib, because this would cause in an additional
    trailer of each line we output from clients. */
  //LOG_init(LOG_TAG);

  parse_args(argc,argv);

  init_buffer();
  
  rmgr_set_prio(l4_myself(), prio);

  if(names_register(LOG_NAMESERVER_NAME)==0){
    LOG_Error("Cannot register at nameserver, falling asleep\n");
    return 1;
  }
  if(verbose){
      printf("Server started and registered as \"%s\"\n",
	      LOG_NAMESERVER_NAME);
#if CONFIG_USE_TCPIP
    printf("Logging to: %s%s\n",
	    flush_local?"console ":"",
	    flush_to_net?"network":""
	);
#else
    printf("Logging to: %s\n", flush_local?"kdebug ":"");
#endif
    printf("Buffersize: %d\n", buffer_size);
  }

  err = flusher_init(flusher_prio);
  if(err){
      LOG_Error("Serious problems initalizing the flusher.\n");
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
		    message.d0 = -L4_EINVAL;
		    continue;
		}
		print_message();
		if(message.d1) flush_buffer();
		message.d0 = 0;
		continue;
#if CONFIG_USE_TCPIP
	    case LOG_COMMAND_CHANNEL_WRITE:
		if(message.result.md.dwords!=4){
		    message.d0 = -L4_EINVAL;
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
	message.d0 = -L4_EINVAL;
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
