/*!
 * \file   names/server/src/names.c
 * \brief  names server implementation
 *
 * \date   05/27/2003
 * \author Uwe Dannowski <Uwe.Dannowski@ira.uka.de>
 * \author Jork Loeser <jork.loeser@inf.tu-dresden.de>
 * \author Frank Mehnert <fm3@os.inf.tu-dresden.de>
 * \author Ronald Aigner <ra3@os.inf.tu-dresden.de>
 * \author Adam Lackorzynski <adam@os.inf.tu-dresden.de>
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/ipc.h>
#include <l4/sys/kdebug.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <l4/util/util.h>
#include <l4/util/bitops.h>
#include <l4/util/getopt.h>
#include <l4/util/l4_macros.h>
#include <l4/sigma0/kip.h>

#include <l4/log/l4log.h>
#include <l4/log/server.h>

#include <l4/rmgr/librmgr.h>

#include "config.h"
#include "names.h"

#include "names-server.h"

#if CONFIG_EVENT
#include <l4/events/events.h>
#endif

typedef struct {
  char		name[NAMES_MAX_NAME_LEN + 1];
  l4_threadid_t	id;
  int		weak;
} entry_t;

#if CONFIG_EVENT
static int using_events;
#endif

/* logserver name to compare */
static int use_logserver=1;
static int fiasco_running;
static int register_thread_names;
static l4_threadid_t logserver_id;
static char logserver_name[NAMES_MAX_NAME_LEN + 1] = "stdlogV05";
void (*logsrv_outfunc)(const char*);	/* the original function using the
                                           logserver. We can use this after
                                           the logserver registered. */

void own_output_function(const char*s);
void parse_args(int argc, char* argv[]);
void init_entries(void);

/* this holds the registered names and thread ids */
static entry_t entries[NAMES_MAX_ENTRIES];

/* runtime debug level */
static int verbosity = 0;

#define DEBUGMSG(x) \
if (verbosity >= x)

/* the standard output function, not using the logserver */
void
own_output_function(const char*s)
{
  outstring(s);
}

/* ohoh, hackish: we have to define a names_wait_for_name, which is used
   by the loglib. But we do not use this function. */
int
names_waitfor_name(const char *name, l4_threadid_t *id, const int timeout)
{
  return 0;
}

int
names_query_name(const char* name, l4_threadid_t* id)
{
  return 0;
}

static void
kernel_register_thread_name(l4_threadid_t id, const char *name)
{
  if (register_thread_names)
    fiasco_register_thread_name(id, name);
}

/*!\brief Register a new thread
 *
 * The string in msg->string.rcv_str needs not to be 0-terminated! Its
 * maximum length is NAMES_MAX_NAME_LEN.
 */
l4_int32_t
server_names_register(l4_threadid_t *client,
                      const char *name,
                      const l4_threadid_t *id,
                      int weak)
{
  int i;

  DEBUGMSG(1)
    printf(l4util_idfmt ": %s(" l4util_idfmt " -> \"%.*s\")\n",
           l4util_idstr(*client), __func__, l4util_idstr(*id),
           NAMES_MAX_NAME_LEN, name);

  for (i = 0; i < NAMES_MAX_ENTRIES; i++)
    if (!weak && !entries[i].weak &&
        !strcmp(entries[i].name, name))
      return 0;

  for (i = 0; i < NAMES_MAX_ENTRIES; i++)
    if (l4_is_invalid_id(entries[i].id))
      break;

  if (i == NAMES_MAX_ENTRIES) {
    printf("names: nameserver full\n");
    return 0;
  }

  entries[i].id = *id;
  entries[i].weak = weak;
  strncpy(entries[i].name, name, NAMES_MAX_NAME_LEN);
  entries[i].name[NAMES_MAX_NAME_LEN] = 0;
  kernel_register_thread_name(entries[i].id, name);

  if(use_logserver && !strcmp(entries[i].name, logserver_name)){
    /* use the logserver now */
    logserver_id = entries[i].id;
    LOG_server_setid(logserver_id);
    LOG_outstring = logsrv_outfunc;
  }

  return 1;
}

long
names_register_component (CORBA_Object _dice_corba_obj,
                          const char* name,
                          CORBA_Server_Environment *_dice_corba_env)
{
  return server_names_register(_dice_corba_obj, name, _dice_corba_obj, 0);
}

long
names_register_thread_component (CORBA_Object _dice_corba_obj,
                                 const char* name,
                                 const l4_threadid_t *id,
                                 CORBA_Server_Environment *_dice_corba_env)
{
  return server_names_register(_dice_corba_obj, name, id, 1);
}

long
names_unregister_thread_component(CORBA_Object _dice_corba_obj,
                                  const char* name,
                                  const l4_threadid_t *id,
                                  CORBA_Server_Environment *_dice_corba_env)
{
  int i, ret = 0;

  if (use_logserver && l4_thread_equal(logserver_id, *_dice_corba_obj)) {
    /* we must not use the logserver any longer */
    LOG_outstring = own_output_function;
    logserver_id = L4_NIL_ID;
  }

  DEBUGMSG(1)
    printf(l4util_idfmt ": %s(" l4util_idfmt " -> \"%.*s\")\n",
           l4util_idstr(*_dice_corba_obj), __func__, l4util_idstr(*id),
           NAMES_MAX_NAME_LEN, name);

  for (i = 0; i < NAMES_MAX_ENTRIES; i++) {
    if (l4_thread_equal(entries[i].id, *id)
        && (!strcmp(entries[i].name, name))) {

      kernel_register_thread_name(entries[i].id, " (deleted)");
      entries[i].id = L4_INVALID_ID;
      entries[i].name[0] = '\0';
      ret = 1;
    }
  }

  return ret;
}

long
names_query_name_component(CORBA_Object _dice_corba_obj,
                           const char* name,
                           l4_threadid_t *id,
                           CORBA_Server_Environment *_dice_corba_env)
{
  int i, w = -1;

  DEBUGMSG(2)
    printf(l4util_idfmt ": %s(\"%.*s\")\n",
           l4util_idstr(*_dice_corba_obj),
           __func__, NAMES_MAX_NAME_LEN, name);

  if (!name)
    return 0;

  for (i = 0; i < NAMES_MAX_ENTRIES; i++) {
    if (!strcmp(entries[i].name, name)) {
      /* remember weak entry, but look further */
      if (entries[i].weak == 1) {
        if (w == -1)
          w = i;
      } else
        break;
    }
  }
  if (i == NAMES_MAX_ENTRIES) {
    if (w == -1) {
      DEBUGMSG(2)
	printf("%s: name \"%.*s\" not found\n", __func__,
	    NAMES_MAX_NAME_LEN, name);
      return 0;
    }
    /* fall back to first matching weak */
    i = w;
  }

  *id = entries[i].id;

  DEBUGMSG(2)
    printf("%s: found entry at %d with id "l4util_idfmt".\n", __func__,
        i, l4util_idstr(*id));
  return 1;
}

long
names_query_id_component(CORBA_Object _dice_corba_obj,
                         const l4_threadid_t *id,
                         char **name,
                         CORBA_Server_Environment *_dice_corba_env)
{
  int i, w = -1;

  DEBUGMSG(2)
    printf(l4util_idfmt ": %s(" l4util_idfmt ")\n",
           l4util_idstr(*_dice_corba_obj), __func__, l4util_idstr(*id));

  for (i = 0; i < NAMES_MAX_ENTRIES; i++) {
    if (l4_thread_equal(entries[i].id, *id)) {
      /* remember weak entry, but look further */
      if (entries[i].weak == 1) {
        if (w == -1)
          w = i;
      } else
        break;
    }
  }

  if (i == NAMES_MAX_ENTRIES) {
    if (w == -1) {
      *name = 0;
      return 0;
    }
    /* fall back to first matching weak */
    i = w;
  }

  /* Content copied by IDL code */
  *name = entries[i].name;

  return 1;
}

/** Query the entry of the given number.
 *
 * \return	cmd==1 -> entry valid, cmd==0->entry invalid
 */
long
names_query_nr_component (CORBA_Object _dice_corba_obj,
                          int nr,
                          char* *name,
                          l4_threadid_t *id,
                          CORBA_Server_Environment *_dice_corba_env)
{
  DEBUGMSG(2)
    printf(l4util_idfmt ": %s(%d)\n",
           l4util_idstr(*_dice_corba_obj), __func__, nr);

  if (nr >= NAMES_MAX_ENTRIES || l4_is_invalid_id(entries[nr].id))
    return 0;

  *id = entries[nr].id;
  /* Content copied by IDL code */
  *name = entries[nr].name;

  return 1;
}

long
names_unregister_task_component (CORBA_Object _dice_corba_obj,
                                 const l4_threadid_t *id,
                                 CORBA_Server_Environment *_dice_corba_env)
{
  int i;

  DEBUGMSG(1)
    printf(l4util_idfmt ": %s(" l4util_idfmt ")\n",
           l4util_idstr(*_dice_corba_obj), __func__, l4util_idstr(*id));

  for (i = 0; i < NAMES_MAX_ENTRIES; i++)
    // We could/should use l4_task_equal here but often it is not possible
    // for a client to determine the exact task id (especially the field
    // version_id) since often the user simply wants to kill task xyz with
    // xyz specifying the task number using an Integer.
    if (l4_tasknum_equal(entries[i].id, *id)) {
      kernel_register_thread_name(entries[i].id, " (deleted)");
      entries[i].id = L4_INVALID_ID;
      entries[i].name[0] = '\0';
    }

  // always return success
  return 1;
}

#define TASK_SHIFT	7	/* number of thread bits */
void
names_dump_component(CORBA_Object _dice_corba_obj,
                     CORBA_Server_Environment *_dice_corba_env)
{
  int i;
  /* 
   * We need to keep track of all the names we already found, so we don't
   * display them twice.
   */
  l4_uint32_t found_bits[NAMES_MAX_ENTRIES / 8 + 1];
  l4_threadid_t t = L4_NIL_ID;
  int idx;

  memset(found_bits, 0, NAMES_MAX_ENTRIES / 8 + 1);
  printf("dumping names server:\n");

  do {
	  int dist = (1 << 20);
	  idx = -1;
	  /* For all entries (except 1st), determine the distance to the last found entry. */
	  for (i = 0; i < NAMES_MAX_ENTRIES; ++i) {
		  /* Skip entries, if they are
		   *   - invalid (trivial), or
		   *   - we already found this entry. This may occur if we get a
		   *   distance of 0, because a thread registered multiple names.
		   */
		  if (!l4_is_invalid_id(entries[i].id) && !l4util_test_bit32(i, found_bits)) {
			  /* Ensure that task IDs have a larger weight than thread IDs 
			   * so that e.g., the distance of 4.3 to 4.2 is smaller than 
			   * the distance of 5.2 to 4.2 */
			  int d = ((entries[i].id.id.task - t.id.task) << TASK_SHIFT) 
			         + (entries[i].id.id.lthread - t.id.lthread);
			  /* Ignore negative distances. These point to entries lying 
			   * before the current one. */
			  if (d >= 0 && d < dist) {
				  idx = i;
				  dist = d;
			  }
		  }
	  }
	  if (idx >= 0) {
		  t = entries[idx].id;
		  l4util_set_bit32(idx, found_bits);
		  printf("taskid "l4util_idfmt" name %s\n",
				 l4util_idstr(entries[idx].id), entries[idx].name);
	  }
  } while (idx > -1);
}

void
init_entries(void)
{
  int i = NAMES_MAX_ENTRIES;
  while (i--) {
    entries[i].id = L4_INVALID_ID;
    entries[i].name[0] = '\0';
  };
}

void
parse_args(int argc, char* argv[])
{

  int longoptind;
  static int longoptcheck;
  signed char c;
  static struct option longopts[] =
  {
    {"verbose", required_argument, &longoptcheck, 1},
    {"logsrv",  required_argument, &longoptcheck, 2},
#if CONFIG_EVENT
    {"events",  0,                 &longoptcheck, 3},
#endif
    {"nofiasco", 0,                &longoptcheck, 4},
    {0,         0,                 0,             0},
  };

  #if 0
  printf("argc=%d, argv at %p\n",argc,argv);
  for(longoptind=0;longoptind<argc;longoptind++){
    printf("option %d: %s\n",longoptind, argv[longoptind]);
  }
  enter_kdebug("!");
  #endif

  optind = 0;
  longoptind = 0;
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
	             NAMES_MAX_NAME_LEN-1, optarg);
	      strncpy(logserver_name, optarg, NAMES_MAX_NAME_LEN);
	      logserver_name[NAMES_MAX_NAME_LEN] = 0;
	      use_logserver = 1;	// potentially use it
	      break;
#if CONFIG_EVENT
	    case 3:
	      using_events = 1;
	      break;
#endif
	    case 4:
	      fiasco_running = 0;
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
	}
    }

  if (optind < argc) {
    for(; optind < argc; optind++){
      printf("unrecognized option `%s'\n", argv[optind]);
    }
  }
}

int
main(int argc, char* argv[])
{
  /* first: use the own output function, because we do not know who is the
            logserver. */
  logsrv_outfunc = LOG_outstring;
  LOG_outstring	= own_output_function;

  l4sigma0_kip_map(L4_INVALID_ID);
  if (l4sigma0_kip_version() == L4SIGMA0_KIP_VERSION_FIASCO)
    {
      fiasco_running = 1;
      register_thread_names = l4sigma0_kip_kernel_has_feature("thread_names");
    }

  parse_args(argc, argv);

  init_entries();

  rmgr_init();

  if (preregister() == 0)
      LOG_Error("Error preregistering threads. Continuing anyway.");

#if CONFIG_EVENT
  if (using_events)
    {
      printf("Starting thread listening for `exit' events\n");
      l4events_init();
    }
#endif

  DEBUGMSG(2)
    printf("Entering server loop.\n");
  names_server_loop(NULL);

  return 0;
}
