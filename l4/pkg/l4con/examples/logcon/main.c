#include <stdio.h>
#include <string.h>

#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/con/l4contxt.h>
#include <l4/log/l4log.h>
#include <l4/sys/kdebug.h>
#include <l4/thread/thread.h>

#define LOG_BUFFERSIZE 81
#define LOG_NAMESERVER_NAME "stdlogV05"
#define LOG_COMMAND_LOG 0

char LOG_tag[9] = "logcon";
l4_ssize_t l4libc_heapsize = 16*1024;

static char message_buffer[LOG_BUFFERSIZE+5];
static volatile int contxt_init_done;

typedef struct
{
  l4_fpage_t fpage;
  l4_msgdope_t size;
  l4_msgdope_t snd;
  l4_umword_t d0, d1, d2, d3, d4, d5, d6;
  l4_strdope_t string;
  l4_msgdope_t result;
} rcv_message_t;

rcv_message_t message;

//! the sender of the last request
l4_threadid_t msg_sender = L4_INVALID_ID;

static int 
get_message(void)
{
  int err;
  
  message.size.md.strings = 1;
  message.size.md.dwords = 7;
  message.string.rcv_size = LOG_BUFFERSIZE;
  message.string.rcv_str = (l4_umword_t)&message_buffer;
  message.fpage.fpage = 0;

  memset(message_buffer,0,LOG_BUFFERSIZE);
  for (;;)
    {
      if (l4_thread_equal(msg_sender, L4_INVALID_ID))
	{
  	  if ((err=l4_ipc_wait(&msg_sender,
				    &message, &message.d0, &message.d1,
				    L4_IPC_TIMEOUT(0,0,0,0,0,0),
				    &message.result))!=0)
	    return err;
	  break;
	} 
      else 
	{
	  err = l4_ipc_reply_and_wait(
				    msg_sender, NULL, message.d0, 0,
				    &msg_sender,
				    &message, &message.d0, &message.d1,
				    L4_IPC_TIMEOUT(0,1,0,0,0,0),
				    &message.result);
	  if (err & L4_IPC_SETIMEOUT)
	    msg_sender = L4_INVALID_ID;
	  else
	    break;
	}
    }
  return 0;
}

extern int console_puts(const char *s);
static void
my_LOG_outstring(const char *s)
{
  console_puts(s);
}

static void
contxt_init_thread(void *data)
{
  for (;;)
    {
      if (!contxt_init_done)
	{
	  l4_threadid_t con_id;

	  if (names_query_name(CON_NAMES_STR, &con_id))
	    {
	      int err;

	      if ((err = contxt_init(2048, 2000)))
		printf("Error %d opening console\n", err);
	      else
		contxt_init_done = 1;
    	      l4thread_exit();
	    }
	}
      l4thread_sleep(50);
    }
}

int
main(int argc, char**argv)
{
  int err;

  LOG_outstring = my_LOG_outstring;

  l4thread_create(contxt_init_thread, 0, L4THREAD_CREATE_ASYNC);

  if (!names_register(LOG_NAMESERVER_NAME))
    {
      printf("Cannot register at nameserver\n");
      return 1;
    }

  strcpy(message_buffer+LOG_BUFFERSIZE,"...\n");
  for(;;)
    {
      err = get_message();

      if (err == 0 || err == L4_IPC_REMSGCUT)
	{
	  if(message.result.md.fpage_received == 0)
	    {
	      switch(message.d0)
		{
	    	case LOG_COMMAND_LOG:
	  	  if(message.result.md.strings!=1)
		    {
		      message.d0 = -L4_EINVAL;
		      continue;
		    }
		  
		  outstring(message_buffer);
		  if (contxt_init_done)
		    printf("%s", message_buffer);
		  
		  message.d0 = 0;
		  continue;
		}
	    }
	  message.d0 = -L4_EINVAL;
	  continue;
	}

      if (err & 0x40)
	continue;
    
      printf("logcon | Error %#x getting message from %x.%x.\n", 
	  err, msg_sender.id.task, msg_sender.id.lthread);
      msg_sender = L4_INVALID_ID;
    }
}
