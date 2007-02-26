#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>

#include <stdio.h>
#include <stdlib.h>

#include <l4/util/util.h>
#include <l4/util/string.h>
#include <l4/util/getopt.h>		/* from libl4util */

#include <l4/log/l4log.h>

#include <names.h>

typedef struct {
  char		name[MAX_NAME_LEN+1];
  l4_threadid_t	id;
} entry_t;

/* logserver name to compare */
static int use_logserver=0;
static l4_threadid_t logserver_id;
static char logserver_name[MAX_NAME_LEN+1];
void (*logsrv_outfunc)(const char*);	/* the original function using the
                                           logserver. We can use this after
                                           the logserver registered. */

void own_output_function(const char*s);
void parse_args(int argc, char* argv[]);
void init_entries(void);

/* this holds the registered names and thread ids */
static entry_t entries[MAX_ENTRIES];

/* runtime debug level */
static int verbosity = 0;

#define DEBUGMSG(x) \
if (verbosity >= x)

/* the standard output function, not using the logserver */
void own_output_function(const char*s){
  while(*s)outchar(*s++);
}

/* ohoh, hackish: we have to define a names_wait_for_name, which is used
   by the loglib. But we do not use this function. */
int names_waitfor_name(const char *name, l4_threadid_t *id, const int timeout)
{
  return 0;
}

int names_query_name(const char* name, l4_threadid_t* id)
{
  return 0;
}

/*!\brief Register a new thread
 *
 * The string in msg->string.rcv_str needs not to be 0-terminated! Its
 * maximum length is MAX_NAME_LEN.
 */
static void
server_names_register(message_t* msg, const l4_threadid_t *who)
{
  int i;

  DEBUGMSG(1)
    printf("%d.%d: %s(%d.%d -> \"%.*s\")\n",
	   who->id.task, who->id.lthread,
	   __func__,
	   msg->id.id.task, msg->id.id.lthread,
	   MAX_NAME_LEN,
	   (char*) msg->string.rcv_str);

  msg->cmd = 0;
  for (i = 0; i < MAX_ENTRIES; i++)
    if (!strcmp(entries[i].name, (char*) msg->string.rcv_str))
      return;
  
  for (i = 0; i < MAX_ENTRIES; i++)
    if (l4_is_invalid_id(entries[i].id))
      break;
  
  if (i == MAX_ENTRIES)
    return;
  
  entries[i].id = msg->id;
  strncpy(entries[i].name, (char*) msg->string.rcv_str, MAX_NAME_LEN);
  entries[i].name[MAX_NAME_LEN] = 0;
  
  if(use_logserver && !strcmp(entries[i].name, logserver_name)){
    /* use the logserver now */
    logserver_id = entries[i].id;
    LOG_server_setid(logserver_id);
    LOG_outstring = logsrv_outfunc;
  }
  msg->cmd = 1;

  return;
};

static void
server_names_unregister(message_t* msg, const l4_threadid_t *who)
{
  int i;

  if(use_logserver && l4_thread_equal(logserver_id, *who)){
    /* we must not use the logserver any longer */
    LOG_outstring = own_output_function;
    logserver_id = L4_NIL_ID;
  }

  DEBUGMSG(1)
    printf("%d.%d: %s(%d.%d -> \"%.*s\")\n",
	   msg->id.id.task, msg->id.id.lthread,
	   __func__,
	   who->id.task, who->id.lthread,
	   MAX_NAME_LEN,
	   (char*) msg->string.rcv_str);

  msg->cmd = 0;
  for (i = 0; i < MAX_ENTRIES; i++)
    if (l4_thread_equal(entries[i].id, msg->id) &&
	(!strcmp(entries[i].name, (char*) msg->string.rcv_str)))
      break;
  
  if (i == MAX_ENTRIES)
    return;
  
  entries[i].id = L4_INVALID_ID;
  entries[i].name[0] = '\0';

  msg->cmd = 1;

  return;
};

static void
server_names_query_name(message_t* msg, const l4_threadid_t *who)
{
  int i;

  DEBUGMSG(2)
    printf("%d.%d: %s(\"%.*s\")\n",
	   who->id.task, who->id.lthread,
	   __func__,
	   MAX_NAME_LEN, (char*) msg->string.rcv_str);

  msg->cmd = 0;
  for (i = 0; i < MAX_ENTRIES; i++)
    if (!strcmp(entries[i].name, (char*) msg->string.rcv_str))
      break;

  if (i == MAX_ENTRIES)
    return;

  msg->id = entries[i].id;
  strncpy((char*) msg->string.rcv_str, entries[i].name, MAX_NAME_LEN);
  msg->cmd = 1;

  return;
};

static void
server_names_query_id(message_t* msg, const l4_threadid_t *who)
{
  int i;

  DEBUGMSG(2)
    printf("%d.%d: %s(%d.%d)\n",
	   who->id.task, who->id.lthread,
	   __func__,
	   msg->id.id.task, msg->id.id.lthread);

  msg->cmd = 0;
  for (i = 0; i < MAX_ENTRIES; i++)
    if (l4_thread_equal(entries[i].id, msg->id))
      break;

  if (i == MAX_ENTRIES)
    return;

  msg->id = entries[i].id;
  strncpy((char*) msg->string.rcv_str, entries[i].name, MAX_NAME_LEN);
  msg->cmd = 1;

  return;
};

/** Query the entry of the given number.
 *
 * \param	the number in msg->id.lh.low
 *
 * \return	the name in msg->string
 * \return	the id in msg->id
 * \return	cmd==1 -> entry valid, cmd==0->entry invalid
 */
static void
server_names_query_nr(message_t* msg, const l4_threadid_t *who)
{
  unsigned i = msg->id.lh.low;

  DEBUGMSG(2)
    printf("%d.%d: %s(%d)\n",
	   who->id.task, who->id.lthread,
	   __func__, i);

  msg->cmd = 0;
  if(i>=MAX_ENTRIES) return;

  if (l4_is_invalid_id(entries[i].id)) return;

  msg->id = entries[i].id;
  strncpy((char*) msg->string.rcv_str, entries[i].name, MAX_NAME_LEN);
  msg->cmd = 1;

  return;
};

static void
server_names_unregister_task(message_t* msg, const l4_threadid_t *who)
{
  int i;

  DEBUGMSG(1)
    printf("%d.%d: %s(%d.%d)\n",
	   who->id.task, who->id.lthread,
	   __func__,
	   msg->id.id.task, msg->id.id.lthread);

  msg->cmd = 0;
  for (i = 0; i < MAX_ENTRIES; i++)
    if (l4_task_equal(entries[i].id, msg->id))
      {
	entries[i].id = L4_INVALID_ID;
	entries[i].name[0] = '\0';
	msg->cmd = 1;
	break;
      }

  return;
}

void
init_entries(void)
{
  int i = MAX_ENTRIES;
  while(i--)
    {
      entries[i].id = L4_INVALID_ID;
      entries[i].name[0] = '\0';
    };
};

void
parse_args(int argc, char* argv[])
{

  int longoptind;
  static int longoptcheck;
  signed char c;
  static struct option longopts[] =
  {
    {"verbose",required_argument,&longoptcheck,1},
    {"logsrv",required_argument, &longoptcheck,2},
    {0,0,0,0},
  };

  #if 0
  printf("argc=%d, argv at %p\n",argc,argv);
  for(longoptind=0;longoptind<argc;longoptind++){
    printf("option %d: %s\n",longoptind, argv[longoptind]);
  }
  enter_kdebug("!");
  #endif
  
  optind = 0;
  longoptind=0;
  while (1)
    {
      c = getopt_long_only(argc, argv, 
			   "", longopts, &longoptind);
      if (c == -1)
	break;
      switch (c){
	case 0: /* long option */
	  switch (longoptcheck) {
	    case 1:
	      printf("Enabling verbose mode\n");
	      verbosity = atol(optarg);
	      break;
	    case 2:
	      printf("Preparing for logserver named \"%.*s\".\n",
	             MAX_NAME_LEN-1,optarg);
	      strncpy(logserver_name, optarg, MAX_NAME_LEN);
	      logserver_name[MAX_NAME_LEN]=0;
	      use_logserver = 1;	// potentially use it
	      break;
	    default:
	      printf("un-option set\n");
	      break;
	    };
	  break;
	case '?':
	  printf("unrecognized/ambiguous option %s.\n", argv[optind-1]);
	  break;
	case ':':
	  printf("missing parameter\n");
	  break;
	default:
	  printf("unknown return val: %#x\n", c);
	};
    };
  
  if (optind < argc) {
    for(;optind<argc;optind++){
      printf("unrecognized option `%s'\n", argv[optind]);
    }
  }
};

int
main(int argc, char* argv[])
{
  message_t	message;
  l4_msgdope_t	result;
  l4_threadid_t	client;
  
  unsigned char	buffer[MAX_NAME_LEN+1];
  unsigned	error;

  /* first: use the own output function, because we do not know who is the
            logserver. */
  logsrv_outfunc = LOG_outstring;
  LOG_outstring	= own_output_function;
  LOG_init("names");

  parse_args(argc, argv);
  
  init_entries();

  /* register ourself, just for fun */
  entries[0].id = l4_myself();
  strncpy(entries[0].name, "names", MAX_NAME_LEN);

  message.fpage.fpage = 0;
  
  message.size_dope = L4_IPC_DOPE(3,1);
  message.send_dope = L4_IPC_DOPE(3,1);
  
  message.string.rcv_str = (l4_umword_t) buffer;
  message.string.snd_str = (l4_umword_t) buffer;
  message.string.rcv_size = MAX_NAME_LEN;
  message.string.snd_size = MAX_NAME_LEN;

  error = l4_i386_ipc_wait(&client, &message,
			   &message.id.lh.low, &message.id.lh.high,
			   L4_IPC_NEVER, &result);
  while (4711)
    {
      if (error)
	while (1410)
	  {
	    outhex32(result.msgdope);
	    enter_kdebug("names   | "__FILE__": main: IPC error");
	  };	

      if(l4_thread_equal(L4_NIL_ID, client)){
      	printf("got a message from %x.%x!\n", 
	    client.id.task, client.id.lthread);
      }

      switch(message.cmd)
	{
	case NAMES_REGISTER:
	  server_names_register(&message, &client);
	  break;
	case NAMES_UNREGISTER:
	  server_names_unregister(&message, &client);
	  break;
	case NAMES_QUERY_NAME:
	  server_names_query_name(&message, &client);
	  break;
	case NAMES_QUERY_ID:
	  server_names_query_id(&message, &client);
	  break;
	case NAMES_QUERY_NR:
	  server_names_query_nr(&message, &client);
	  break;
	case NAMES_UNREGISTER_TASK:
	  server_names_unregister_task(&message, &client);
	  break;
	default:
	  message.cmd = 0;
	  break;
	};

      error = l4_i386_ipc_reply_and_wait(client, &message,
					 message.id.lh.low, message.id.lh.high,
					 &client, &message,
					 &message.id.lh.low, &message.id.lh.high,
					 L4_IPC_NEVER, &result);
    };
  
while (4711)
    enter_kdebug("names   | "__FILE__": exit");
  
  return 0;
};
