/* $Id$ */
/**
 * \file	loader/server/src/cfg.c
 * \brief	helper functions for scanning the config script
 *
 * \date 	01/01/2001
 * \author	Frank Mehnert */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include "cfg.h"

#include <stdio.h>
#include <malloc.h>
#include <stdarg.h>
#include <string.h>

#include <l4/env/errno.h>
#include <l4/sys/consts.h>
#include <l4/rmgr/librmgr.h>
#include <l4/sys/kdebug.h>
#include <l4/thread/thread.h>

#include "app.h"
#include "dm-if.h"
#include "fprov-if.h"
#include "exec-if.h"
#include "idl.h"

static cfg_task_t *cfg_task[CFG_MAX_TASK];
static cfg_task_t **cfg_task_nextfree = cfg_task;
static cfg_task_t **cfg_task_current  = cfg_task;
static cfg_task_t **cfg_task_nextout  = cfg_task;

static l4env_infopage_t *cfg_env;
static l4dm_dataspace_t cfg_env_ds;

/** create a pseudo task */
int
cfg_job(unsigned int flag, unsigned int number)
{
  cfg_task_t *ct;

  if (*cfg_task_nextfree || cfg_task_nextfree >= cfg_task+CFG_MAX_TASK)
    {
      /* no free slot */
      printf("No remaining job descriptors\n");
      return -L4_ENOMEM;
    }

  if (!(ct = (cfg_task_t*)malloc(sizeof(cfg_task_t))))
    {
      printf("Out of memory\n");
      return -L4_ENOMEM;
    }

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
cfg_new_task(const char *fname, const char *args)
{
  cfg_task_t *ct;

  if (*cfg_task_nextfree || cfg_task_nextfree >= cfg_task+CFG_MAX_TASK)
    {
      /* no free slot */
      printf("No remaining task descriptors\n");
      return -L4_ENOMEM;
    }

  if (!(ct = (cfg_task_t*)malloc(sizeof(cfg_task_t))))
    {
      printf("Out of memory\n");
      return -L4_ENOMEM;
    }

  ct->flags       = 0;
  ct->prio        = DEFAULT_PRIO;
  ct->ds          = L4DM_INVALID_DATASPACE;
  ct->next_module = ct->module;
  ct->next_mem    = ct->mem;
  ct->task.fname  = fname;
  ct->task.args   = args;

  if (*cfg_task_current)
    cfg_task_current++;
  
  *cfg_task_nextfree++ = ct;

  if (cfg_verbose>0)
    printf("<%s>, args <%s>\n", fname, args);

  return 0;
}

/** create new module for current config task */
int
cfg_new_module(const char *fname, const char *args)
{
  cfg_module_t cm;

  if (!*cfg_task_current)
    {
      /* cfg_new_module before cfg_new_task */
      printf("Add module to which task?\n");
      return -L4_EINVAL;
    }
  
  if ((*cfg_task_current)->next_module >=
      (*cfg_task_current)->module + CFG_MAX_MODULE)
    {
      printf("Can't add another module\n");
      return -L4_ENOMEM;
    }
  
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
    {
      /* cfg_new_mem before cfg_new_task */
      printf("Add memory region to which task?\n");
      return -L4_EINVAL;
    }
  
  if ((*cfg_task_current)->next_mem >=
      (*cfg_task_current)->mem + CFG_MAX_MEM)
    {
      printf("Can't add another memory region\n");
      return -L4_ENOMEM;
    }

  size = l4_round_page(size);

  if (high == 0)
    high = low + size;
  
  cm.size  = size;
  cm.low   = low;
  cm.high  = high;
  cm.flags = flags;

  *((*cfg_task_current)->next_mem++) = cm;
  
  if (cfg_verbose>0)
    printf("  <%s>: mem: %6d kB at %08x-%08x flags %02x\n",
	(*cfg_task_current)->task.fname, size / 1024, low, high, flags);

  return 0;
}


/** set priority of task we currently working on */
int
cfg_new_task_prio(unsigned int prio)
{
  if (!*cfg_task_current)
    /* cfg_set_prio before cfg_new_task */
    {
      printf("Set priority of which task?\n");
      return -L4_EINVAL;
    }

  (*cfg_task_current)->prio = prio;
  
  if (cfg_verbose>0)
    printf("  <%s>: priority: %02x\n", 
	(*cfg_task_current)->task.fname, (*cfg_task_current)->prio);

  return 0;
}


/** set flag of task we currently working on */
int
cfg_new_task_flag(unsigned int flag)
{
  if (!*cfg_task_current)
    /* cfg_set_prio before cfg_new_task */
    {
      printf("Set flag of which task?\n");
      return -L4_EINVAL;
    }

  (*cfg_task_current)->flags |= flag;
  
  if (cfg_verbose>0)
    printf("  <%s>: flag: %02x\n",
	(*cfg_task_current)->task.fname, flag);

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

/** Parse config script */
static int
parse_cfg(l4_addr_t cfg_addr, l4_size_t cfg_size)
{
  cfg_parse_init();
  cfg_setup_input((void*)cfg_addr, cfg_size);

  return cfg_parse();
}

/** load config script using file server
 *
 * \param fname		file name
 * \param fprov_id	id of file provider */
int
load_config_script(const char *fname, l4_threadid_t fprov_id)
{
  int error, parse_error;
  cfg_task_t **ct;
  l4_addr_t cfg_addr;
  l4_size_t cfg_size;
  l4dm_dataspace_t cfg_ds;
 
  /* load config file using file provider fprov_id */
  if ((error = load_file(fname, fprov_id, app_dm_id, 
			 /* use_modpath=*/0, /*contiguous=*/0, 
			 &cfg_addr, &cfg_size, &cfg_ds)))
    return return_error_msg(error, "opening file \"%s\"", fname);

  /* check if user gave us filename of executable */
  if ((error = exec_if_ftype(&cfg_ds, cfg_env)) < 0)
    return return_error_msg(error, "checking file type \"%s\"", fname);

  if (error > 0)
    {
      /* yes => load application directly */
      cfg_task_t ct;
      char *path_end;
      
      ct.flags       = 0;
      ct.prio        = DEFAULT_PRIO;
      ct.ds          = cfg_ds;
      ct.next_module = ct.module;
      ct.next_mem    = ct.mem;
      ct.task.fname  = fname;
      ct.task.args   = NULL;

      printf("\"%s\" is a valid binary image\n", fname);

      /* set libpath according to filename to be able to load shared libs */
      path_end = strrchr(fname, '/');
      if (path_end)
	{
	  l4_size_t size = path_end - fname + 1;
	  if (size > sizeof(cfg_libpath)-1)
	    size = sizeof(cfg_libpath)-1;
	  strncpy(cfg_libpath, fname, size);
	  cfg_libpath[size] = '\0';
	  printf("Setting libpath to %s\n", cfg_libpath);
	}
      
      return app_start(&ct, fprov_id);
    }

  /* no => parse config file */
  parse_error = parse_cfg(cfg_addr, cfg_size);

  if ((error = junk_ds(&cfg_ds, cfg_addr)))
    {
      printf("Error %d junking file \"%s\"\n", error, fname);
      return error;
    }

  if (parse_error)
    {
      printf("Error parsing config file \"%s\"\n", fname);
      return return_error_msg(-L4_EINVAL, "parsing config file \"%s\"", fname);
    }

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

/** Init config stuff before begin with parsing */
int
cfg_parse_init(void)
{
  memset(cfg_task, 0, sizeof(cfg_task));
  cfg_task_nextfree = cfg_task_current = cfg_task_nextout = cfg_task;
  return 0;
}

/** init cfg stuff */
int
cfg_init(void)
{
  int error;
  l4_addr_t addr;

  if ((error = create_ds(app_dm_id, L4_PAGESIZE, &addr, &cfg_env_ds,
			 "cfg infopage")) < 0)
    {
      printf("Error %d creating cfg infopage\n", error);
      return error;
    }

  cfg_env = (l4env_infopage_t*)addr;
  init_infopage(cfg_env);

  return 0;
}

