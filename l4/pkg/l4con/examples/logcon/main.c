#include <stdio.h>
#include <string.h>

#include <l4/names/libnames.h>
#include <l4/env/errno.h>
#include <l4/l4con/l4contxt.h>
#include <l4/log/l4log.h>
#include <l4/sys/kdebug.h>
#include <l4/thread/thread.h>
#include <l4/util/l4_macros.h>
#include "log-server.h"

#define LOG_BUFFERSIZE 81
#define LOG_NAMESERVER_NAME "stdlogV05"
#define LOG_COMMAND_LOG 0

char LOG_tag[9] = "logcon";
l4_ssize_t l4libc_heapsize = 16*1024;

static char message_buffer[LOG_BUFFERSIZE+5];
static volatile int init_status;
static char  init_buffer[200*(LOG_BUFFERSIZE+5)];
static char *init_buffer_beg = init_buffer;
static char *init_buffer_ptr = init_buffer;
static int   init_buffer_len;

//! the sender of the last request
l4_threadid_t msg_sender = L4_INVALID_ID;

extern int console_puts(const char *s);

static void
my_LOG_outstring(const char *s)
{
  console_puts(s);
}

static void*
msg_alloc(unsigned long size)
{
  if (size != LOG_BUFFERSIZE)
    return 0;
  return message_buffer;
}

void
log_outstring_component (CORBA_Object _dice_corba_obj,
                         int flush_flag,
			 const char* string,
			 CORBA_Server_Environment *_dice_corba_env)
{
  outstring(string);
  if (init_status == 0)
    {
      /* console still not initialized, use init_buffer as ring buffer */
      char c;
      for (; (c=*string++);)
	{
	  if (init_buffer_ptr >= init_buffer + sizeof(init_buffer))
	    init_buffer_ptr = init_buffer;
	  *init_buffer_ptr++ = c;
	  if (init_buffer_len < sizeof(init_buffer))
	    init_buffer_len++;
	  else
	    init_buffer_beg = init_buffer_ptr;
	}
      return;
    }
    
  /* wait until init buffer is written to text console */
  while (init_status == 1)
    l4thread_sleep(100);

  /* print to L4 console */
  printf("%s", string);
}

int
log_channel_open_component (CORBA_Object _dice_corba_obj,
                            l4_snd_fpage_t page,
                            int channel,
                            CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_EINVAL;
}

int
log_channel_write_component (CORBA_Object _dice_corba_obj,
                             int channel,
                             unsigned int offset,
                             unsigned int size,
                             CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_EINVAL;
}

int
log_channel_flush_component (CORBA_Object _dice_corba_obj,
                             int channel,
                             CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_EINVAL;
}

int
log_channel_close_component (CORBA_Object _dice_corba_obj,
                             int channel,
                             CORBA_Server_Environment *_dice_corba_env)
{
  return -L4_EINVAL;
}

static void
contxt_init_thread(void *data)
{
  for (;;)
    {
      if (init_status == 0)
	{
	  l4_threadid_t con_id;

	  if (names_query_name(CON_NAMES_STR, &con_id))
	    {
	      int err;

	      if ((err = contxt_init(2048, 2000)))
		printf("Error %d opening console\n", err);
	      else
		{
		  init_status = 1;
		  while (init_buffer_len--)
		    {
		      if (init_buffer_beg >= init_buffer + sizeof(init_buffer))
			init_buffer_beg = init_buffer;
		      putchar(*init_buffer_beg++);
		    }
		  init_status = 2;
		}
    	      l4thread_exit();
	    }
	}
      l4thread_sleep(50);
    }
}

int
main(int argc, char**argv)
{
  CORBA_Server_Environment env = dice_default_server_environment;

  LOG_outstring = my_LOG_outstring;

  if (!names_register(LOG_NAMESERVER_NAME))
    {
      printf("Cannot register at nameserver\n");
      return 1;
    }

  l4thread_create(contxt_init_thread, 0, L4THREAD_CREATE_ASYNC);

  env.malloc = msg_alloc;
  env.rcv_fpage.fpage = 0;

  strcpy(message_buffer+LOG_BUFFERSIZE,"...\n");
  log_server_loop (&env);
}
