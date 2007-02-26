/* $Id$ */
/**
 * \file loader/server/src/cfg.c
 *
 * \date 	01/01/2001
 * \author	Frank Mehnert
 *
 * \brief	helper functions for scanning the config script */

#include "cfg.h"

#include <stdio.h>
#include <malloc.h>
#include <stdarg.h>

#include <l4/env/errno.h>
#include <l4/sys/consts.h>
#include <l4/rmgr/librmgr.h>
#include <l4/sys/kdebug.h>
#include <l4/thread/thread.h>

#include "app.h"
#include "dm-if.h"
#include "fprov-if.h"

static cfg_task_t *cfg_task[CFG_MAX_TASK];
static cfg_task_t **cfg_task_nextfree = cfg_task;
static cfg_task_t **cfg_task_current  = cfg_task;
static cfg_task_t **cfg_task_nextout  = cfg_task;

/** create a pseudo task */
int
cfg_job(unsigned int flag, unsigned int number)
{
  cfg_task_t *ct;

  if (*cfg_task_nextfree || cfg_task_nextfree >= cfg_task+CFG_MAX_TASK)
    /* no free slot */
    return -L4_ENOMEM;

  if (!(ct = (cfg_task_t*)malloc(sizeof(cfg_task_t))))
    return -L4_ENOMEM;

  ct->flags       = flag;
  ct->prio        = number;
  ct->next_module = ct->module;
  ct->next_mem    = ct->mem;
  ct->task.fname  = NULL;
  ct->task.args   = NULL;

  if (*cfg_task_current)
    cfg_task_current++;
  
  *cfg_task_nextfree++ = ct;

  if (cfg_verbose>0)
    {
      switch (flag)
	{
	case CFG_F_MEMDUMP:
	  printf("new job: memdump\n");
	  break;
	case CFG_F_SLEEP:
	  printf("new job: sleep for %dms\n", ct->prio);
	  break;
	}
    }

  return 0;
}

/** create new config task */
int
cfg_new_task(const char *fname, const char *args, unsigned int flags)
{
  cfg_task_t *ct;

  if (*cfg_task_nextfree || cfg_task_nextfree >= cfg_task+CFG_MAX_TASK)
    /* no free slot */
    return -L4_ENOMEM;

  if (!(ct = (cfg_task_t*)malloc(sizeof(cfg_task_t))))
    return -L4_ENOMEM;

  ct->flags       = flags;
  ct->prio        = DEFAULT_PRIO;
  ct->next_module = ct->module;
  ct->next_mem    = ct->mem;
  ct->task.fname  = fname;
  ct->task.args   = args;

  if (*cfg_task_current)
    cfg_task_current++;
  
  *cfg_task_nextfree++ = ct;

  if (cfg_verbose>0)
    printf("<%s>, args <%s>, flags %04x\n", fname, args, flags);

  return 0;
}

/** create new module for current config task */
int
cfg_new_module(const char *fname, const char *args)
{
  cfg_module_t cm;

  if (!*cfg_task_current)
    /* cfg_new_module before cfg_new_task */
    return -L4_EINVAL;
  
  if ((*cfg_task_current)->next_module >=
      (*cfg_task_current)->module + CFG_MAX_MODULE)
    return -L4_ENOMEM;
  
  cm.fname = fname;
  cm.args  = args;

  *((*cfg_task_current)->next_module++) = cm;
  
  if (cfg_verbose>0)
    printf("  <%s>: module <%s>, args <%s>\n", 
	(*cfg_task_current)->task.fname, fname, args);

  return 0;
}

/** create new memory region for current config task */
int
cfg_new_mem(unsigned int size, unsigned int low, unsigned int high, 
            unsigned int flags)
{
  cfg_mem_t cm;
  
  if (!*cfg_task_current)
    /* cfg_new_mem before cfg_new_task */
    return -L4_EINVAL;
  
  if ((*cfg_task_current)->next_mem >=
      (*cfg_task_current)->mem + CFG_MAX_MEM)
    return -L4_ENOMEM;

  size = l4_round_page(size);

  if (high == 0)
    high = low + size;
  
  cm.size  = size;
  cm.low   = low;
  cm.high  = high;
  cm.flags = flags;

  *((*cfg_task_current)->next_mem++) = cm;
  
  if (cfg_verbose>0)
    printf("  <%s>: mem: %d kB at %08x-%08x flags %04x\n",
	(*cfg_task_current)->task.fname, size / 1024, low, high, flags);

  return 0;
}


/** set priority of task we currently working on */
int
cfg_set_prio(unsigned int prio)
{
  if (!*cfg_task_current)
    /* cfg_set_prio before cfg_new_task */
    {
      printf("no task current\n");
    return -L4_EINVAL;
    }

  (*cfg_task_current)->prio = prio;
  
  if (cfg_verbose>0)
    printf("  <%s>: priority: %02x\n", 
	(*cfg_task_current)->task.fname, (*cfg_task_current)->prio);

  return 0;
}


/** return next config task */
cfg_task_t**
cfg_next_task(void)
{
  if (!*cfg_task_nextout || cfg_task_nextout >= cfg_task+CFG_MAX_TASK)
    return NULL;
  
  return cfg_task_nextout++;
}

/** recycle cfg_task descriptor */
static void
cfg_clear_task(cfg_task_t *ct)
{
  if (ct)
    {
      while (ct->next_module > ct->module)
	{
	  ct->next_module--;
	  free((char*)ct->next_module->fname);
	  free((char*)ct->next_module->args);
	  ct->next_module->fname = NULL;
	  ct->next_module->args  = NULL;
	}
      
      free((char*)ct->task.fname);
      free((char*)ct->task.args);
      ct->flags       = 0;
      ct->next_module = ct->module;
      ct->next_mem    = ct->mem;
      ct->task.fname  = NULL;
      ct->task.args   = NULL;
      free(ct);
    }
}

/** Add parameters to task's command line */
int
add_task_arg(cfg_task_t *ct, const char *format,...)
{
  char *new_args;

  if ((new_args = malloc(512)))
    {
      int old_sz = strlen(ct->task.args), new_sz;
      const char *old_args = ct->task.args;
      va_list list;
      
      va_start(list, format);
      vsprintf(new_args, format, list);
      va_end(list);
      new_sz = old_sz + strlen(new_args);
      ct->task.args = (char*)malloc(new_sz);
      strcpy((char*)ct->task.args, old_args);
      strcpy((char*)ct->task.args+old_sz, new_args);
  
      free((char*)new_args);
      free((char*)old_args);

      return 0;
    }

  return -L4_ENOMEM;
}

/** Parse config script */
static int
parse_cfg(const char *cfg_fname, l4_threadid_t fprov_id)
{
  int error, parse_error;
  l4_addr_t cfg_addr;
  l4_size_t cfg_size;
  l4dm_dataspace_t cfg_ds;

  if ((error = load_file(cfg_fname, fprov_id, app_dm_id, 0, 0,
			 &cfg_addr, &cfg_size, &cfg_ds)))
    {
      printf("Error %d opening config file \"%s\"\n", error, cfg_fname);
      return error;
    }

  cfg_init();
  cfg_setup_input((void*)cfg_addr, cfg_size);

  parse_error = cfg_parse();

  if ((error = junk_ds(&cfg_ds, cfg_addr)))
    {
      printf("Error %d junking config file \"%s\"\n", error, cfg_fname);
      return error;
    }

  if (parse_error)
    {
      printf("Error parsing config file \"%s\"\n", cfg_fname);
      return -L4_EINVAL;
    }

  return 0;
}

/** load config script using file server
 *
 * \param fname		file name
 * \param fprov_id	id of file provider */
int
load_config_script(const char *fname, l4_threadid_t fprov_id)
{
  int error;
  cfg_task_t **ct;
 
  /* load config file using file provider fprov_id */
  if ((error = parse_cfg(fname, fprov_id)))
    return error;
	  
  while ((ct = cfg_next_task()))
    {
      if ((*ct)->flags & CFG_F_MEMDUMP)
	{
	  /* specal job: Show rmgr memory dump */
	  rmgr_dump_mem();
	}
      else if ((*ct)->flags & CFG_F_SLEEP)
	{
	  /* special job: Sleep for a while */
	  printf("sleeping for %d ms\n", (*ct)->prio);
	  l4thread_sleep((*ct)->prio);
	}
      else
	{
	  /* start regular task */
	  if ((error = app_start(*ct, fprov_id)))
	    break;
	}
      
      /* recycle cfg_task_t */
      cfg_clear_task(*ct);
      *ct = (cfg_task_t*)NULL;
    }

  /* recycle cfg_task_t's */
  while ((ct = cfg_next_task()))
    {
      cfg_clear_task(*ct);
      *ct = (cfg_task_t*)NULL;
    }

  return error;
}

/** Init config stuff */
int
cfg_init(void)
{
  memset(cfg_task, 0, sizeof(cfg_task));
  cfg_task_nextfree = cfg_task_current = cfg_task_nextout = cfg_task;

  return 0;
}

