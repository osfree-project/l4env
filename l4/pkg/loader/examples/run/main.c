/**
 * \file	loader/examples/run/main.c
 * \anchor	a_run
 *
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * \brief	Small L4 console for controling the L4 Loader
 *
 * 		The program depends on the L4 console. 
 *
 * 		After startup a command prompt appears. There is a help 
 * 		command 'h'. The program is able to start and kill L4 
 * 		programs. All output is buffered and may be backscrolled
 * 		by using SHIFT PgUp and SHIFT PgDn. */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/l4rm/l4rm.h>
#include <l4/rmgr/librmgr.h>
#include <l4/con/l4contxt.h>
#include <l4/names/libnames.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/loader/loader-client.h>
#include <l4/env/env.h>
#include <l4/thread/thread.h>
#include <l4/semaphore/semaphore.h>
#include <l4/exec/exec.h>
#include <l4/log/l4log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char LOG_tag[9] = "run";		/**< tell log library who we are */
l4_ssize_t l4libc_heapsize = -128*1024;
const int l4thread_max_threads = 6;

static l4_threadid_t loader_id;		/**< L4 thread id of L4 loader */
static l4_threadid_t tftp_id;		/**< L4 thread id of TFTP daemon */
static l4_threadid_t dm_id;		/**< L4 thread id of ds manager */

// not exported from oskit10_support_l4env library
extern void my_pc_reset(void);

/** give help */
static void
help(void)
{
  printf(
"  possible command are\n"
"  a ... show information about all loaded applications at loader\n"
"  d ... show all dataspaces of simple_dm\n"
"  f ... set file provider\n"
"  k ... kill an application which was loaded by the loader using the task id\n"
"  l ... load a new application using tftp\n"
"  m ... dump dataspace manager's memory map into to L4 debug console\n"
"  n ... list all registered names at name server\n"
"  p ... dump dataspace manager's pool info to L4 debug console\n"
"  r ... dump rmgr memory info to L4 debug console\n"
"  ^ ... reboot machine\n");
}

#if 0
static void
test_scroll(void)
{
  extern int vtc_cols __attribute__((weak));
  extern int contxt_trygetchar(void) __attribute__((weak));
  
  if (vtc_cols)
    {
      while (contxt_trygetchar() == -1)
	{
	  int i, j;
	  for (j=0; j<50; j++)
	    {
	      for (i=0; i<vtc_cols-2; i++)
		putchar((i & 0x3f) + ' ');

    	      l4thread_sleep(10);
    	    }
	}
    }
}
#endif

/** attach dataspace to our address space */
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

/** detach and close dataspace */
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
      
      printf("  %s ds %3d: %08x-%08x ",
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
dump_task_info(unsigned int task)
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
      printf("Error %d (%s) dumping l4 task %x\n", 
	  error, l4env_errstr(error), task);
      return;
    }

  if ((error = attach_ds(&ds, &addr)))
    return;
  
  printf("\"%s\", #%x\n", fname, task);

  dump_l4env_infopage((l4env_infopage_t*)addr);

  junk_ds(addr);
}

/** show application info */
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
    dump_task_info(*ptr);

  junk_ds(addr);
}

/** show information on dataspace */
static void
show_ds_info(void)
{
  int error;
  long nr;
  static char number[10];
  l4dm_dataspace_t ds;
  l4_addr_t addr;
  l4_threadid_t tid = L4_INVALID_ID;
  static int ihb_init = 0;
  static contxt_ihb_t ihb;

  if (!ihb_init)
    {
      contxt_init_ihb(&ihb, 128, sizeof(number)-1);
      ihb_init = 1;
    }
  
  printf("  List dataspaces of (nr,0=all): ");
  contxt_read(number, sizeof(number), &ihb);
  printf("\n");
  
  nr = strtol(number, 0, 0);

  if (nr == 0)
    tid = L4_INVALID_ID;
  else
    {
      tid.id.task = nr;
      tid.id.lthread = 0;
    }
  
  error = l4dm_ds_dump(dm_id, tid, L4DM_SAME_TASK, &ds);
  if (error)
    {
      printf("Error %d (%s) getting l4 task list\n", 
	  error, l4env_errstr(error));
      return;
    }
  
  if ((error = attach_ds(&ds, &addr)))
    return;

  printf((char*)addr);
  
  junk_ds(addr);
}

/** start an application */
static int
load_app(void)
{
  static char command[60];
  int error;
  CORBA_Environment env = dice_default_environment;
  static int ihb_init = 0;
  static contxt_ihb_t ihb;
  static char error_msg[1024];
  char *ptr = error_msg;

  if (!ihb_init)
    {
      contxt_init_ihb(&ihb, 128, sizeof(command)-1);
      ihb_init = 1;
      contxt_add_ihb(&ihb, "(nd)/tftpboot/" USERNAME "/cfg_linux");
    }

  printf("  Load application: ");
  contxt_read(command, sizeof(command), &ihb);
  printf("\n");

  if (command[0] == 0)
    return 0;

  if ((error = l4loader_app_open_call(&loader_id, command,
				      &tftp_id, 0, &ptr, &env)))
    {
      printf("  Error %d (%s) loading application\n", 
	  error, l4env_errstr(error));
      if (*error_msg)
	printf("  (Loader says:'%s')\n", error_msg);
      return error;
    }

  printf("  successfully loaded.\n");

  return 0;
}

/** kill an application */
static int
kill_app(void)
{
  static char number[10];
  int error;
  long nr;
  CORBA_Environment env = dice_default_environment;
  static int ihb_init = 0;
  static contxt_ihb_t ihb;

  if (!ihb_init)
    {
      contxt_init_ihb(&ihb, 128, sizeof(number)-1);
      ihb_init = 1;
    }
  
  printf("  Kill application (nr): ");
  contxt_read(number, sizeof(number), &ihb);
  printf("\n");

  nr = strtol(number, 0, 0);

  if (nr == l4thread_l4_id(l4thread_myself()).id.task)
    {
      printf("  can't kill myself!\n");
    }
  else if (nr != 0)
    {
      if ((error = l4loader_app_kill_call(&loader_id, nr, 0, &env)))
	{
	  printf("Error %d (%s) killing application\n", 
	      error, l4env_errstr(error));
	  return error;
	}
      
      printf("  successfully killed.\n");
    }

  return 0;
}

/** list all servers which are registered at names server */
static void
show_names_info(void)
{
  int i;
  static char name[60];
  l4_threadid_t tid;

  for (i=0; i<NAMES_MAX_ENTRIES; i++)
    if (names_query_nr(i, name, sizeof(name), &tid))
      {
	name[sizeof(name)-1] = '\0';
	printf("%3x.%02x   %s\n", 
	    tid.id.task, tid.id.lthread, name);
      }
}

/** set new file provider */
static void
set_file_provider(void)
{
  int i, j=0, n;
  static char name[60];
  static char threadid[10];
  l4_threadid_t tid;
  static l4_threadid_t fprov[NAMES_MAX_ENTRIES];

  printf("  Choose new file provider from following list:\n");
  for (i=0; i<NAMES_MAX_ENTRIES; i++)
    if (names_query_nr(i, name, sizeof(name), &tid))
      {
	name[sizeof(name)-1] = '\0';
	printf("  %2d: %3x.%02x   %s\n",
	    j, tid.id.task, tid.id.lthread, name);
	fprov[j] = tid;
	j++;
      }
  printf("  select nr: ");
  *threadid = '\0';
  contxt_read(threadid, sizeof(threadid), 0);
  printf("\n");
  if (*threadid)
    {
      n = strtol(threadid, 0, 0);
      if (n < j)
	{
	  printf("  Set file provider to %x.%x\n", 
	      fprov[n].id.task, fprov[n].id.lthread);
	  tftp_id = fprov[n];
	}
      else
	{
	  printf("  Nr out of range!\n");
	}
    }
}

/** basic command input loop */
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
	case 'd': // show information about dataspaces
	  show_ds_info();
	  break;
	case 'f':
	  set_file_provider();
	  break;
	case 'l':
	  load_app();
	  break;
	case 'r':
	  printf("  RMGR memory dump to debugging console\n");
	  rmgr_dump_mem();
	  break;
	case 'k':
	  kill_app();
	  break;
	case 'm':
	  printf("  DM_PHYS memory map to debugging console\n");
	  l4dm_memphys_show_memmap();
	  break;
	case 'n':
	  show_names_info();
	  break;
	case 'p':
	  printf("  DM_PHYS memory pools to debugging console\n");
	  l4dm_memphys_show_pools();
	  l4dm_memphys_show_pool_free(0);
	  break;
#if 0
	case '_':
	  test_scroll();
	  break;
#endif
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
	      my_pc_reset();
	    }
	  printf("Reboot request canceled\n");
	  break;
	default:
	  printf("## invalid command\n");
	}
    }
}

/** main function */
int
main(int argc, char *argv[])
{
  int error;

  if ((error = contxt_init(4096, 200)))
    {
      printf("Error %d opening contxt lib\n", error);
      return error;
    }

  if (!names_waitfor_name("LOADER", &loader_id, 3000))
    {
      printf("LOADER not found");
      return -2;
    }

  if (!names_waitfor_name("TFTP", &tftp_id, 3000))
    {
      printf("TFTP not found");
      return -2;
    }

  dm_id = l4env_get_default_dsm();

  command_loop();

  return 0;
}

