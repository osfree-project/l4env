/**
 * \file	loader/examples/run/main.c
 *
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * \brief	Small L4 console for controling the L4 Loader
 *
 *		The program depends on the L4 console.
 *
 *		After startup a command prompt appears. There is a help
 *		command 'h'. The program is able to start and kill L4
 *		programs. All output is buffered and may be backscrolled
 *		by using SHIFT PgUp and SHIFT PgDn. */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/l4rm/l4rm.h>
#include <l4/rmgr/librmgr.h>
#include <l4/names/libnames.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/loader/loader-client.h>
#include <l4/loader/loader.h>
#include <l4/env/env.h>
#include <l4/events/events.h>
#include <l4/thread/thread.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/exec/exec.h>
#include <l4/log/l4log.h>
#include <l4/util/reboot.h>
#include <l4/util/parse_cmd.h>
#include <l4/util/l4_macros.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef DIRECT
#include "direct.h"
#include "direct_ihb.h"
#else
#include <l4/l4con/l4contxt.h>
#endif

l4_ssize_t l4libc_heapsize = -256*1024;
const  int l4thread_max_threads = 6;

static l4_threadid_t loader_id;		/**< L4 thread id of L4 loader. */
static l4_threadid_t tftp_id;		/**< L4 thread id of TFTP daemon. */
static l4_threadid_t dm_id;		/**< L4 thread id of ds manager. */

static char load_app_command[80];	/**< for load_app(). */
static contxt_ihb_t load_app_ihb;	/**< for load_app(). */
static char searchpath[]		/**< for load_app(). */
        = "(nd)/tftpboot/" USERNAME;

/** give help. */
static void
help(void)
{
  printf(
"  a ... show information about all loaded applications at loader\n"
"  A ... dump all applications of the task server to L4 debug console\n"
"  c ... close all dataspaces of a specific task\n"
"  d ... show all allocated dataspaces of the default dataspace manager\n"
"  D ... show detail information about a specific dataspace\n"
"  e ... show information about events server to L4 debug console\n"
"  f ... set default file provider\n"
"  k ... kill an application which was loaded by the loader using the task id\n"
"  l ... load a new application using tftp\n"
"  m ... dump dataspace manager's memory map into to L4 debug console\n"
"  n ... list all registered names at name server\n"
"  N ... list really all registered names at name server\n"
"  p ... dump dataspace manager's pool info to L4 debug console\n"
"  r ... dump rmgr memory info to L4 debug console\n"
"  K ... enter kernel debugger\n"
"  Q ... quit\n"
"  ^ ... reboot machine\n");
}

/** attach dataspace to our address space. */
static int
attach_ds(l4dm_dataspace_t *ds, l4_addr_t *addr)
{
  int error;
  l4_size_t size;

  if ((error = l4dm_mem_size(ds, &size)))
    {
      printf("Error %d (%s) getting size of dataspace\n",
	  error, l4env_errstr(error));
      return error;
    }

  if ((error = l4rm_attach(ds, size, 0, L4DM_RO, (void **)addr)))
    {
      printf("Error %d (%s) attaching dataspace\n",
	  error, l4env_errstr(error));
      return error;
    }

  return 0;
}

/** detach and close dataspace. */
static int
junk_ds(l4_addr_t addr)
{
  l4dm_mem_release((void *)addr);
  return 0;
}

static void
dump_l4env_infopage(l4env_infopage_t *env)
{
  int i, j, k, pos=0;

  static char *type_names_on[] =
    {"r", "w", "x", " reloc", " link", " page", " reserve",
      " share", " beg", " end", " errlink" };
  static char *type_names_off[] =
    {"-", "-", "-", "", "", "", "", "", "", "", "" };

  for (i=0; i<env->section_num; i++)
    {
      l4exec_section_t *l4exc = env->section + i;
      char pos_str[8]="   ";

      if (l4exc->info.type & L4_DSTYPE_OBJ_BEGIN)
	{
	  sprintf(pos_str, "%3d", pos);
	  pos++;
	}

      printf("  %s ds %3d: %08lx-%08lx ",
	   pos_str, l4exc->ds.id, l4exc->addr, l4exc->addr+l4exc->size);
      for (j=1, k=0; j<=L4_DSTYPE_ERRLINK; j<<=1, k++)
	{
	  if (l4exc->info.type & j)
	    printf("%s",type_names_on[k]);
	  else
	    printf("%s",type_names_off[k]);
	}
      printf("\n");
    }
}

static void
show_task_info(unsigned int task)
{
  int error;
  static char fname_buf[1024];
  char *fname = fname_buf;
  l4dm_dataspace_t ds;
  l4_addr_t addr;
  CORBA_Environment env = dice_default_environment;

  if ((error = l4loader_app_info_call(&loader_id, task, 0, &fname,
				      &ds, &env)))
    {
      printf("Error %d (%s) dumping l4 task #%02X\n",
	  error, l4env_errstr(error), task);
      return;
    }

  if ((error = attach_ds(&ds, &addr)))
    return;

  printf("\"%s\", #%02X\n", fname, task);

  dump_l4env_infopage((l4env_infopage_t*)addr);

  junk_ds(addr);
}

/** show application info. */
static void
show_app_info(void)
{
  static char fname_buf[1024];
  int error;
  char *fname = fname_buf;
  l4dm_dataspace_t ds;
  l4_addr_t addr;
  CORBA_Environment env = dice_default_environment;
  unsigned int *ptr;

  if ((error = l4loader_app_info_call(&loader_id, 0, 0, &fname,
				      &ds, &env)))
    {
      printf("Error %d (%s) getting l4 task list\n",
	  error, l4env_errstr(error));
      return;
    }

  if ((error = attach_ds(&ds, &addr)))
    return;

  for (ptr=(unsigned int*)addr; *ptr != 0; ptr++)
    show_task_info(*ptr);

  junk_ds(addr);
}

static void
kill_ds_all(void)
{
  int error;
  char task[10];
  l4_threadid_t tid = L4_INVALID_ID;

  printf("  Kill all dataspaces of task: ");
  contxt_ihb_read(task, sizeof(task), 0);
  printf("\n");

  tid.id.task = strtol(task, 0, 16);

  error = l4dm_close_all(dm_id, tid, L4DM_SAME_TASK);
  if (error)
    printf("Error %d (%s) closing all dataspaces of that task\n",
	error, l4env_errstr(error));
}

/** retrieve information about _every_ dataspace and show how many
 * memory is used by each application */
static void
show_ds_info(void)
{
  l4dm_dataspace_t ds, next_ds;
  l4_size_t size, total_size;
  l4_threadid_t owner, tid = L4_INVALID_ID;
  char name[L4DM_DS_NAME_MAX_LEN+1];
  static int ihb_init = 0;
  static contxt_ihb_t ihb;
  char number[10];
  long nr;

  if (!ihb_init)
    {
      contxt_ihb_init(&ihb, 128, sizeof(number)-1);
      ihb_init = 1;
    }

  printf("  List dataspaces of (nr,0=all): ");
  contxt_ihb_read(number, sizeof(number), &ihb);
  printf("\n");

  nr = strtol(number, 0, 16);

  if (nr == 0)
    tid = L4_INVALID_ID;
  else
    {
      tid.id.task = nr;
      tid.id.lthread = 0;
    }

restart:
  ds         = L4DM_INVALID_DATASPACE;
  ds.manager = dm_id;

  /* retrieve the first dataspace id */
  if (l4dm_mem_info(&ds, &size, &owner, name, &ds.id) == -L4_ENOTFOUND)
    return;

  for (total_size = 0; ds.id != L4DM_INVALID_DATASPACE.id; ds.id = next_ds.id)
    {
      if (l4dm_mem_info(&ds, &size, &owner, name, &next_ds.id) == -L4_ENOTFOUND)
	goto restart;
      if (!l4_is_invalid_id(tid) && !l4_tasknum_equal(owner, tid))
	continue;
      printf("%5u:  size %08x (%7uKB,%4uMB)  owner"
	     l4util_idfmt_adjust"  %s\n",
	  ds.id, size, (size+(1<<9)-1)/(1<<10), (size+(1<<19)-1)/(1<<20),
	  l4util_idstr(owner), name);
      total_size += size;
    }

  if (total_size)
    {
      printf("==========================================================="
	     "=====================\n"
	     "       total %08x (%7uKB,%4uMB)\n",
	     total_size,
	     (total_size+(1<<9 )-1)/(1<<10),
	     (total_size+(1<<19)-1)/(1<<20));
    }
}


/** show all information about a specific dataspace. */
static int
show_one_ds(void)
{
  char number[10];
  long nr;
  l4dm_dataspace_t ds;

  printf("  Show details about a dataspace (nr): ");
  contxt_ihb_read(number, sizeof(number), 0);
  printf("\n");

  nr = strtol(number, 0, 0);
  if (nr)
    {
      ds.manager = dm_id;
      ds.id = nr;

      printf("  (See debug output)\n");
      l4dm_ds_show(&ds);
    }
  else
    printf("  Error: Give a dataspace number (>0).\n");

  return 1;
}

/** start an application. */
static int
load_app(void)
{
  int error = 0;
  CORBA_Environment env = dice_default_environment;
  static char error_msg[1024];
  char *ptr = error_msg;
  l4_taskid_t task_ids[l4loader_MAX_TASK_ID];
  l4dm_dataspace_t dummy_ds = L4DM_INVALID_DATASPACE;
  int i;
  static char cmd_buf[sizeof(searchpath) + sizeof(load_app_command)];
  char *cur_path = searchpath;

  printf("  Load application: ");
  contxt_ihb_read(load_app_command, sizeof(load_app_command), &load_app_ihb);
  printf("\n");

  if (load_app_command[0] == 0)
    return 0;

  while (cur_path)
    {
      if (strchr(load_app_command, '/'))
        {
          strncpy(cmd_buf, load_app_command, sizeof(cmd_buf));
          cur_path = NULL;
        }
      else
        {
          char *colonpos = strchr(cur_path, ':');

          if (colonpos)
            *colonpos = 0;
          snprintf(cmd_buf, sizeof(cmd_buf), "%s/%s",
                   cur_path, load_app_command);
          if (colonpos)
            {
              *colonpos = ':';
              cur_path = colonpos + 1;
            }
          else
            cur_path = NULL;
        }

// Enable the following macro to show the usage of the L4LOADER_STOP flag
// when starting a new application.
// #define BREAK_START
#ifdef BREAK_START
      if ((error = l4loader_app_open_call(&loader_id, &dummy_ds, cmd_buf,
                                          &tftp_id, L4LOADER_STOP, task_ids,
                                          &ptr, &env)))
        {
          if (error == -L4_ENOTFOUND)
            continue;

          printf("  Error %d (%s) loading application\n",
              error, l4env_errstr(error));
          if (*error_msg)
            printf("  (Loader says:'%s')\n", error_msg);
          break;
        }

      printf("  Successfully initialized task%s ",
          l4_is_invalid_id(task_ids[1]) ? "" : "s");

      for (i=0; i<l4loader_MAX_TASK_ID && !l4_is_invalid_id(task_ids[i]); i++)
        printf("%s" l4util_idfmt,
            i>0 ? ", " : "", l4util_idstr(task_ids[i]));

      printf(". Going to start them.\n");

      for (i=0; i<l4loader_MAX_TASK_ID && !l4_is_invalid_id(task_ids[i]); i++)
        {
          if ((error = l4loader_app_cont_call(&loader_id, &task_ids[i], &env)))
            printf("  Cannot continue task "l4util_idfmt", error %d (%s)\n",
                l4util_idstr(task_ids[i]), error, l4env_errstr(error));
          else
            printf("  Task "l4util_idfmt" successfully started.\n",
                l4util_idstr(task_ids[i]));
        }
      break;
#else
      if ((error = l4loader_app_open_call(&loader_id, &dummy_ds, cmd_buf,
                                          &tftp_id, 0, task_ids,
                                          &ptr, &env)))
        {
          if (error == -L4_ENOTFOUND)
            continue;

          printf("  Error %d (%s) loading application\n",
              error, l4env_errstr(error));
          if (*error_msg)
            printf("  (Loader says:'%s')\n", error_msg);
          break;
        }

      printf("  Successfully started task%s ",
          l4_is_invalid_id(task_ids[1]) ? "" : "s");

      for (i=0; i<l4loader_MAX_TASK_ID && !l4_is_invalid_id(task_ids[i]); i++)
        printf("%s" l4util_idfmt,
            i>0 ? ", " : "", l4util_idstr(task_ids[i]));

      printf(".\n");
      break;
#endif
    }

  if (error == -L4_ENOTFOUND)
    printf("run: command not found: %s\n", load_app_command);

  return error;
}

/** kill an application. */
static int
kill_app(void)
{
  char number[10];
  int error;
  long nr;
  static int ihb_init = 0;
  static contxt_ihb_t ihb;

  if (!ihb_init)
    {
      contxt_ihb_init(&ihb, 128, sizeof(number)-1);
      ihb_init = 1;
    }

  printf("  Kill application (hex-nr): ");
  contxt_ihb_read(number, sizeof(number), &ihb);
  printf("\n");

  nr = strtol(number, 0, 16);

  if (nr == l4thread_l4_id(l4thread_myself()).id.task)
    printf("  can't kill myself!\n");

  else if (nr != 0)
    {
      l4_taskid_t taskid;

      if ((error = l4ts_taskno_to_taskid(nr, &taskid)))
	printf("There seems to be no corresponding task ID\n");
      else
	{
	  if ((error = l4ts_kill_task_recursive(taskid)))
	    printf("Error %d (%s) killing task #%02lX\n",
		error, l4env_strerror(-error), nr);
	  else
	    printf("  successfully killed.\n");
	}
    }

  return 0;
}

/** Compare two L4 thread IDs.
 * \param    tid1 Thread ID
 * \param    tid2 Thread ID
 * \return > 0 if tid1 is less than tid2
 * \return   0 if tid1 is equal or greater than tid2 */
static int
less_tid(l4_threadid_t tid1, l4_threadid_t tid2)
{
  return tid1.id.task < tid2.id.task ||
	 (tid1.id.task    == tid2.id.task &&
	  tid1.id.lthread  < tid2.id.lthread);
}

/** list all servers registered at names server. */
static void
show_names_info(int filter)
{
  int i, cnt;
  static char name[60];
  l4_threadid_t tid, tid_next;
  static l4_threadid_t tids[NAMES_MAX_ENTRIES];

  for (i=0; i<sizeof(tids)/sizeof(tids[0]); i++)
    {
      if (names_query_nr(i, name, sizeof(name), &tids[i]) &&
	  (!filter || !strchr(name, '.')))
	continue;
      tids[i] = L4_INVALID_ID;
    }

  for (cnt=0, i=0; i<sizeof(tids)/sizeof(tids[0]); i++)
    if (!l4_is_invalid_id(tids[i]))
      cnt++;

  tid = tids[0];
  for (i=1; i<sizeof(tids)/sizeof(tids[0]); i++)
    if (!l4_is_invalid_id(tids[i]) &&
	(l4_is_invalid_id(tid) || less_tid(tids[i], tid)))
      tid = tids[i];

  while (!l4_is_invalid_id(tid))
    {
      if (names_query_id(tid, name, sizeof(name)))
	printf(l4util_idfmt_adjust"   %s\n", tid.id.task, tid.id.lthread, name);

      tid_next = L4_INVALID_ID;
      for (i=0; i<sizeof(tids)/sizeof(tids[0]); i++)
	{
	  if (!l4_is_invalid_id(tids[i]) && less_tid(tid, tids[i]) &&
	      (l4_is_invalid_id(tid_next) || less_tid(tids[i], tid_next)))
	    tid_next = tids[i];
	}

      tid = tid_next;
    }

}

/** set new file provider. */
static void
set_file_provider(void)
{
  int i, j=0, n;
  static char name[60];
  char threadid[10];
  l4_threadid_t tid;
  static l4_threadid_t fprov[NAMES_MAX_ENTRIES];

  printf("  Choose new file provider from following list:\n");
  for (i=0; i<NAMES_MAX_ENTRIES; i++)
    if (names_query_nr(i, name, sizeof(name), &tid))
      {
	name[sizeof(name)-1] = '\0';
	if (!strchr(name, '.'))
	  {
	    printf("  %2d: "l4util_idfmt_adjust"   %s\n",
	       	j, tid.id.task, tid.id.lthread, name);
	    fprov[j] = tid;
	    j++;
	  }
      }
  printf("  select nr: ");
  *threadid = '\0';
  contxt_ihb_read(threadid, sizeof(threadid), 0);
  printf("\n");
  if (*threadid)
    {
      n = strtol(threadid, 0, 0);
      if (n < j)
	{
	  printf("  Set file provider to "l4util_idfmt"\n",
	      l4util_idstr(fprov[n]));
	  tftp_id = fprov[n];
	}
      else
	printf("  Nr out of range!\n");
    }
}

/** retrieve information about _every_ dataspace and show how many
 * memory is used by each application */
static void
show_prog_memory(void)
{
  l4dm_dataspace_t ds;
  struct
    {
      l4_uint32_t task;
      l4_size_t   size;
    } t[128];
  int i, j, k, found;
  l4_size_t size, total_size;
  l4_threadid_t owner;
  char name[L4DM_DS_NAME_MAX_LEN+1];

restart:
  memset(t, 0, sizeof(t));
  ds         = L4DM_INVALID_DATASPACE;
  ds.manager = dm_id;

  /* retrieve the first dataspace id */
  if (l4dm_mem_info(&ds, &size, &owner, name, &ds.id) == -L4_ENOTFOUND)
    return;

  for (;;)
    {
      if (l4dm_mem_info(&ds, &size, &owner, name, &ds.id) == -L4_ENOTFOUND)
	goto restart;

      found = 0;
      for (i=0; i<sizeof(t)/sizeof(t[0]) && t[i].task; i++)
	if (t[i].task == owner.id.task)
	  {
	    t[i].size += size;
	    found = 1;
	    break;
	  }
      if (!found && i<sizeof(t)/sizeof(t[0]))
	{
	  t[i].task = owner.id.task;
	  t[i].size = size;
	}
      if (ds.id == L4DM_INVALID_DATASPACE.id)
	break;
    }

  for (i=0, total_size=0;; )
    {
      for (k=0, found=0, j=999999; k<sizeof(t)/sizeof(t[0]); k++)
	if (t[k].task < j && t[k].task > i)
	  {
	    j = t[k].task;
	    found = 1;
	  }
      if (!found)
	break;
      for (k=0, size=0; k<sizeof(t)/sizeof(t[0]); k++)
	if (t[k].task == j)
	  size += t[k].size;
      printf("  %3X  %7uKB  (%4uMB)  \n",
	  j, (size+(1<<9)-1)/(1<<10), (size+(1<<19)-1)/(1<<20));
      total_size += size;
      i = j;
    }
  if (t[0].task)
    {
      printf("===========================\n"
	     " total%8uKB  (%4uMB)\n",
	     (total_size+(1<<9)-1)/(1<<10), (total_size+(1<<19)-1)/(1<<20));
    }
}

static void
show_events_info(void)
{
  l4events_dump();
}

/** basic command input loop. */
static void
command_loop(void)
{
  for (;;)
    {
      int c;

      printf("\n<cmd>: ");
      do
	{
	  c = getchar();
	  if (c == 0xd)
	    printf("\n<cmd>: ");
	} while (c < 0x20);

      putchar(c);

      printf("\n");
      switch (c)
	{
	case 'a': // show information about loaded applications
	  show_app_info();
	  break;
	case 'A':
#ifndef DIRECT
	  printf("  SIMPLE_TS task dump to debugging console\n");
#endif
	  l4ts_dump_tasks();
	  break;
	case 'c':
	  kill_ds_all();
	  break;
	case 'd': // show information about dataspaces
	  show_ds_info();
	  break;
	case 'D':
	  show_one_ds();
	  break;
	case 'e':
	  show_events_info();
	  break;
	case 'f':
	  set_file_provider();
	  break;
	case 'l':
	  load_app();
	  break;
	case 'r':
#ifndef DIRECT
	  printf("  RMGR memory dump to debugging console\n");
#endif
	  rmgr_dump_mem();
	  break;
	case 'k':
	  kill_app();
	  break;
	case 'm':
#ifndef DIRECT
	  printf("  DM_PHYS memory map to debugging console\n");
#endif
	  l4dm_memphys_show_memmap();
	  break;
	case 'M':
	  show_prog_memory();
	  break;
	case 'n':
	  show_names_info(1);
	  break;
	case 'N':
	  show_names_info(0);
	  break;
	case 'p':
#ifndef DIRECT
	  printf("  DM_PHYS memory pools to debugging console\n");
#endif
	  l4dm_memphys_show_pools();
	  l4dm_memphys_show_pool_free(0);
	  break;
	case ' ':
	  contxt_clrscr();
	  break;
	case '?':
	case 'h':
	  help();
	  break;
	case '^':
	  printf("  Really reboot? Press '^' again ... ");
	  if (getchar() == '^')
	    {
	      printf("Rebooting ... ");
	      l4util_reboot();
	    }
	  printf("Reboot request canceled\n");
	  break;
	case 'K':
	  printf("  Entering kernel debugger\n");
	  enter_kdebug("Enter from run");
	  break;
	case 'Q':
	  printf("  Really quit? Press 'y'...");
	  if (getchar() == 'y')
	    exit(0);
	  else
	    printf(" go on.\n");
	  break;
	default:
	  printf("## invalid command\n");
	}
    }
}

static void
add_to_load_app_ihb(int id, const char *string, int num)
{
  contxt_ihb_add(&load_app_ihb, string);
}

static int
set_fprov(const char *fprov_name)
{
  if (!fprov_name || !*fprov_name)
    {
      l4env_infopage_t *l4env_infopage = l4env_get_infopage();
      if (!l4env_infopage)
	fprov_name = "TFTP"; /* Fall back to TFTP */
      else
	tftp_id = l4env_infopage->fprov_id;
    }

  if (fprov_name && *fprov_name)
    {
      if (!names_waitfor_name(fprov_name, &tftp_id, 30000))
	{
	  LOG("File provider \"%s\" not found -- terminating", fprov_name);
	  return -2;
	}

      printf("File provider is \"%s\"", fprov_name);
    }
  return 0;
}

/** main function. */
int
main(int argc, const char *argv[])
{
  int error;
  const char *fprov_name;

  contxt_ihb_init(&load_app_ihb, 64, sizeof(load_app_command)-1);
  contxt_ihb_add(&load_app_ihb, "(nd)/tftpboot/" USERNAME "/cfg_linux");

  if ((error = parse_cmdline(&argc, &argv,
		'f', "fprov", "specify file provider",
		PARSE_CMD_STRING, "", &fprov_name,
		'i', "ihb", "add entry to input history of [l]oad",
		PARSE_CMD_FN_ARG, 0, &add_to_load_app_ihb,
		0)))
    {
      switch (error)
	{
	case -1: LOG("Bad parameter for parse_cmdline()"); break;
	case -2: LOG("Out of memory in parse_cmdline()"); break;
	case -3:
	case -4: return 1;
	default: LOG("Error %d in parse_cmdline()", error); break;
	}
      return 1;
    }

  if ((error = contxt_init(4096, 1000)))
    {
      LOG("Error %d opening contxt lib -- terminating", error);
      return error;
    }

  if (!names_waitfor_name("LOADER", &loader_id, 30000))
    {
      LOG("Dynamic loader LOADER not found -- terminating");
      return -2;
    }

  if ((error = set_fprov(fprov_name)))
    return error;

  dm_id = l4env_get_default_dsm();

#ifdef DIRECT
  key_init();
#endif

  command_loop();

  return 0;
}
