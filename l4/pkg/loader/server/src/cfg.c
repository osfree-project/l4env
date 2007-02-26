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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#ifdef USE_OSKIT
#include <malloc.h>
#endif

#include <l4/env/errno.h>
#include <l4/sys/consts.h>
#include <l4/rmgr/librmgr.h>
#include <l4/sys/kdebug.h>
#include <l4/thread/thread.h>
#include <l4/names/libnames.h>
#include <l4/loader/loader.h>
#include <l4/exec/errno.h>

#include "app.h"
#include "dm-if.h"
#include "fprov-if.h"
#include "exec-if.h"
#include "elf-loader.h"
#include "idl.h"
#include "global.h"

static cfg_task_t *cfg_task[CFG_MAX_TASK];
static cfg_task_t **cfg_task_nextfree = cfg_task;
static cfg_task_t **cfg_task_current  = cfg_task;
static cfg_task_t **cfg_task_nextout  = cfg_task;

static cfg_task_t cfg_task_template;
static cfg_task_t *cfg_task_template_ptr = &cfg_task_template;

static l4env_infopage_t *cfg_env;
static l4dm_dataspace_t cfg_env_ds;

static void
cfg_make_template(void)
{
  cfg_task_t *ct  = cfg_task_template_ptr;

  memset(ct, 0, sizeof(*ct));

  ct->flags       = CFG_F_TEMPLATE;
  ct->prio        = DEFAULT_PRIO;
  ct->mcp         = DEFAULT_MCP;
  ct->ds_image    = L4DM_INVALID_DATASPACE;
  ct->next_module = ct->module;
  ct->next_mem    = ct->mem;
  ct->fprov_id    = L4_INVALID_ID;
  ct->dsm_id      = app_dsm_id;
}

static void
cfg_init_task(cfg_task_t *ct, l4dm_dataspace_t ds, l4_threadid_t fprov_id, 
	      const char *fname)
{
  memset(ct, 0, sizeof(*ct));

  ct->flags       = cfg_task_template.flags & ~CFG_F_TEMPLATE;
  ct->prio        = cfg_task_template.prio;
  ct->mcp         = cfg_task_template.mcp;
  ct->ds_image    = ds;
  ct->next_module = ct->module;
  ct->next_mem    = ct->mem;
  ct->task.fname  = strdup(fname);
  ct->fprov_id    = fprov_id;
  ct->dsm_id      = cfg_task_template.dsm_id;

  if (cfg_task_template.iobitmap)
    {
      ct->iobitmap = (char*)malloc(8192);
      memcpy(ct->iobitmap, cfg_task_template.iobitmap, 8192);
    }
}

/** Create a task template. */
void
cfg_new_task_template(void)
{
  if (*cfg_task_current)
    cfg_task_current++;
  
  *cfg_task_nextfree++ = cfg_task_template_ptr;

  if (cfg_verbose>0)
    printf("<template>\n");
}

/** Create a pseudo task. */
int
cfg_job(l4_umword_t flag, unsigned int number)
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

  memset(ct, 0, sizeof(*ct));

  ct->flags       = flag;
  ct->prio        = number;
  ct->next_module = ct->module;
  ct->next_mem    = ct->mem;
  ct->fprov_id    = L4_INVALID_ID;
  ct->dsm_id      = L4_INVALID_ID;

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

/** Create new config task. */
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

  cfg_init_task(ct, cfg_task_template.ds_image, 
		cfg_task_template.fprov_id, fname);
  ct->task.args = args;

  if (*cfg_task_current)
    cfg_task_current++;
  
  *cfg_task_nextfree++ = ct;

  if (cfg_verbose>0)
    printf("<%s>, args <%s>\n", fname, args);

  return 0;
}

/** Create new module for current config task. */
int
cfg_new_module(const char *fname, const char *args, 
	       l4_addr_t low, l4_addr_t high)
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
  cm.low   = low;
  cm.high  = high;

  *((*cfg_task_current)->next_module++) = cm;
  
  if (cfg_verbose>0)
    printf("  <%s>: module <%s>, args <%s>\n", 
	(*cfg_task_current)->task.fname, fname, args);

  return 0;
}

/** Create new memory region for current config task. */
int
cfg_new_mem(l4_size_t size, l4_addr_t low, l4_addr_t high, l4_umword_t flags)
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

  if (flags >> 16 > 7)
    {
      printf("Can't use memory pool %d -- only have 7 pools\n", flags >> 16);
      return -L4_EINVAL;
    }

  size = l4_round_page(size);

  if (high == 0)
    high = low + size;

  cm.size  = size;
  cm.low   = low;
  cm.high  = high;
  cm.flags = flags & 0xffff;
  cm.pool  = flags >> 16;

  *((*cfg_task_current)->next_mem++) = cm;
  
  if (cfg_verbose>0)
    printf("  <%s>: mem: %6d kB at %08x-%08x flags %02x pool %d\n",
	(*cfg_task_current)->task.fname, size / 1024,
	low, high, cm.flags, cm.pool);

  return 0;
}


/** Add I/O ports low..high to I/O permission bitmap. */
int
cfg_new_ioport(int low, int high)
{
  int i;

  if (!*cfg_task_current)
    {
      printf("Set I/O port of which task?\n");
      return -L4_EINVAL;
    }

  if (low > high)
    {
      printf("%d > %d?\n", low, high);
      return -L4_EINVAL;
    }

  if (high > 65536)
    {
      printf("maximum allowed port is 65535\n");
      return -L4_EINVAL;
    }

  if (!(*cfg_task_current)->iobitmap)
    {
      if (!((*cfg_task_current)->iobitmap = (char*)malloc(8192)))
	{
	  printf("Cannot allocate I/O bitmap\n");
	  return -L4_ENOMEM;
	}
      memset((*cfg_task_current)->iobitmap, 0, 8192);
    }

  for (i=low; i<=high; i++)
    (*cfg_task_current)->iobitmap[i/8] |= (1 << (i%8));

  return 0;
}


/** Set priority of task we currently working on. */
int
cfg_task_prio(unsigned int prio)
{
  if (!*cfg_task_current)
    /* cfg_task_prio before cfg_new_task */
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


/** Set priority of task we currently working on. */
int
cfg_task_mcp(unsigned int mcp)
{
  if (!*cfg_task_current)
    /* cfg_task_mcp before cfg_new_task */
    {
      printf("Set priority of which task?\n");
      return -L4_EINVAL;
    }

  (*cfg_task_current)->mcp = mcp;
  
  if (cfg_verbose>0)
    printf("  <%s>: mcp: %02x\n", 
	(*cfg_task_current)->task.fname, (*cfg_task_current)->mcp);

  return 0;
}


/** Set flag of task we currently working on. */
int
cfg_task_flag(l4_umword_t flag)
{
  if (!*cfg_task_current)
    /* cfg_task_flag before cfg_new_task */
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

/** Set task's file provider. */
int
cfg_task_fprov(const char *fprov_name)
{
  if (!*cfg_task_current)
    /* cfg_task_fprov before cfg_new_task */
    {
      printf("Set file provider of which task?\n");
      return -L4_EINVAL;
    }

  if (!names_waitfor_name(fprov_name, &(*cfg_task_current)->fprov_id, 5000))
    {
      printf("File provider \"%s\" not found\n", fprov_name);
      return -L4_ENOTFOUND;
    }

  return 0;
}

/** Set task's dataspace manager. */
int
cfg_task_dsm(const char *dsm_name)
{
  if (!*cfg_task_current)
    /* cfg_task_fprov before cfg_new_task */
    {
      printf("Set dataspace manager of which task?\n");
      return -L4_EINVAL;
    }

  if (!names_waitfor_name(dsm_name, &(*cfg_task_current)->dsm_id, 5000))
    {
      printf("Dataspace manager \"%s\" not found\n", dsm_name);
      return -L4_ENOTFOUND;
    }

  return 0;
}

/** Return next config task. */
cfg_task_t**
cfg_next_task(void)
{
  if (!*cfg_task_nextout || cfg_task_nextout >= cfg_task+CFG_MAX_TASK)
    return NULL;
  
  return cfg_task_nextout++;
}

/** Recycle cfg_task descriptor. */
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
	  ct->next_module->fname = 0;
	  ct->next_module->args  = 0;
	}
      
      free((char*)ct->task.fname);
      free((char*)ct->task.args);
      free(ct->iobitmap);
    }
}

/** Parse config script. */
static int
parse_cfg(l4_addr_t cfg_addr, l4_size_t cfg_size)
{
  int error;

  cfg_parse_init();
  cfg_setup_input((void*)cfg_addr, cfg_size);
  error = cfg_parse();
  cfg_done();

  return error;
}

int
load_config_script_from_file(const char *fname_and_arg, l4_threadid_t fprov_id,
			     l4_taskid_t owner, l4_uint32_t flags,
			     l4_taskid_t task_ids[])
{
  int error;
  int is_binary = 0;
  l4_addr_t addr;
  l4_size_t size;
  l4dm_dataspace_t ds = L4DM_INVALID_DATASPACE;

  /* optional arguments in fname_and_arg are ignored by load_file() */
  if ((error = load_file(fname_and_arg, fprov_id, app_dsm_id,
			 /*search_path=*/NULL, /*contiguous=*/0,
			 &addr, &size, &ds)))
    return return_error_msg(error, "loading file", fname_and_arg);

  error = load_config_script(fname_and_arg, fprov_id, &ds, addr, size,
			     owner, flags, &is_binary, task_ids);

  /* don't try to deallocate the dataspace if it was a binary */
  if (!is_binary)
    junk_ds(&ds, addr);

  return error;
}

/** Load application directly (without config script).
 *
 * \param fname		file name of the binary image
 * \param fprov_id	file provieder to retrieve the shared libs from
 * \param bin_ds	dataspace containing the binary image
 * \param bin_addr	address the binary is mapped to
 * \param bin_size	size of the dataspace
 * \param owner		owner
 * \param ext_flags	external Flags (see <l4/loader/loader.h>)
 * \param cfg_flags	internal Flags (CFG_F_*)
 * \retval task_ids	IDs of started tasks */
static int
load_without_script(const char *fname_and_arg, l4_size_t fname_len, 
		    l4_threadid_t fprov_id, const l4dm_dataspace_t *bin_ds, 
		    l4_addr_t bin_addr, l4_size_t bin_size, l4_taskid_t owner,
		    l4_uint32_t ext_flags, l4_uint32_t cfg_flags,
		    l4_taskid_t task_ids[])
{
  cfg_task_t ct;
  char *path_end;
  int error;
  char *fname = malloc(fname_len+1);

  if (!fname)
    return -L4_ENOMEM;

  snprintf(fname, fname_len+1, "%s", fname_and_arg);

  cfg_init_task(&ct, *bin_ds, fprov_id, fname);
  ct.image    = bin_addr;
  ct.sz_image = bin_size;

  if (fname_and_arg[fname_len] != '\0')
    ct.task.args = strdup(fname_and_arg + fname_len + 1);

  ct.flags |= cfg_flags;

  if (ext_flags & L4LOADER_STOP)
    ct.flags |= CFG_F_STOP;

  if (ct.flags & CFG_F_INTERPRETER)
    printf("\"%s\" needs %s\n", fname, interp);
  else
    printf("\"%s\" is a valid binary image\n", fname);

  /* set libpath according to filename to be able to load shared libs */
  if ((path_end = strrchr(fname, '/')))
    {
      l4_size_t size = path_end - fname + 1;
      if (size > sizeof(cfg_libpath)-1)
	size = sizeof(cfg_libpath)-1;
      strncpy(cfg_libpath, fname, size);
      cfg_libpath[size] = '\0';
      printf("Setting libpath to %s\n", cfg_libpath);
    }

  if (!(ct.flags & CFG_F_INTERPRETER))
    {
      if ((error = l4rm_detach((void*) bin_addr)))
	printf("Error %d detaching dataspace\n", error);
    }

  error = app_boot(&ct, owner);

  if (task_ids)
    {
      task_ids[0] = ct.task_id;
      task_ids[1] = L4_INVALID_ID;
    }

  cfg_clear_task(&ct);
  free(fname);

  return error;
}

/** Load config script from dataspace image. Before loading, the dataspace
 *  is sent to the exec server which for ELF check. If it is a valid ELF
 *  binary, it is directly loaded.
 *
 * \param fname_and_arg	file name of the config script
 * \param fprov_id	file provider to retrieve the config script from
 * \param cfg_ds	dataspace of the config script (optionally)
 * \param cfg_addr	address the dataspace is attached to
 * \param cfg_size	size of the dataspace
 * \param owner		owner of the new application
 * \param ext_flags	external Flags (see <l4/loader/loader.h>
 * \param is_binary	is that an ELF binary?
 * \retval task_ids	IDs of started tasks */
int
load_config_script(const char *fname_and_arg, l4_threadid_t fprov_id,
		   const l4dm_dataspace_t *cfg_ds, l4_addr_t cfg_addr,
		   l4_size_t cfg_size, l4_taskid_t owner,
		   l4_uint32_t ext_flags, int *is_binary,
		   l4_taskid_t task_ids[])
{
  int error, parse_error;
  cfg_task_t **ct;
  int nr = 0;
  l4_uint32_t cfg_flags = 0;
  l4_size_t fname_len = strlen(fname_and_arg);
  const char *o;

  if ((o = strchr(fname_and_arg, ' ')))
    fname_len = o-fname_and_arg;

  /* check if user gave us a filename of an executable */
  error = exec_if_ftype(cfg_ds, cfg_env);
  if (error == -L4_EINVAL)
    return return_error_msg(error, "checking file type of \"%s\"",
	fname_and_arg);

  *is_binary = (error ==  0 || error == -L4_EXEC_INTERPRETER);

  if (error == -L4_EXEC_INTERPRETER)
    cfg_flags |= CFG_F_INTERPRETER;

  if (*is_binary)
    return load_without_script(fname_and_arg, fname_len, fprov_id, cfg_ds, 
			       cfg_addr, cfg_size, owner, ext_flags, cfg_flags,
			       task_ids);

  parse_error = parse_cfg(cfg_addr, cfg_size);

  if (parse_error)
    {
      printf("Error parsing config \"%s\"\n", fname_and_arg);
      return return_error_msg(-L4_EINVAL, "parsing config file \"%s\"",
			      fname_and_arg);
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
      else if (!((*ct)->flags & CFG_F_TEMPLATE))
	{
	  /* start regular task */
	  (*ct)->flags |= ext_flags & L4LOADER_STOP ? CFG_F_STOP : 0;
	  if (l4_is_invalid_id((*ct)->fprov_id))
	    (*ct)->fprov_id = fprov_id;
	  if (l4_is_invalid_id((*ct)->dsm_id))
	    (*ct)->dsm_id = app_dsm_id;

	  if ((error = app_boot(*ct, owner)))
      	    break;

	  if (task_ids)
	    task_ids[nr++] = (*ct)->task_id;
	}

      if (!((*ct)->flags & CFG_F_TEMPLATE))
	{
	  cfg_clear_task(*ct);
	  free(*ct);
	}
      *ct = (cfg_task_t*)0;
    }

  if (task_ids && nr < l4loader_MAX_TASK_ID)
    task_ids[nr++] = L4_INVALID_ID;

  /* recycle cfg_task_t's */
  while ((ct = cfg_next_task()))
    {
      if (!((*ct)->flags & CFG_F_TEMPLATE))
	{
	  cfg_clear_task(*ct);
	  free(*ct);
	}
      *ct = (cfg_task_t*)0;
    }

  return error;
}

/** Init config stuff before begin with parsing. */
int
cfg_parse_init(void)
{
  memset(cfg_task, 0, sizeof(cfg_task));
  cfg_task_nextfree = cfg_task_current = cfg_task_nextout = cfg_task;
  return 0;
}

/** Init cfg stuff. */
int
cfg_init(void)
{
  int error;
  l4_addr_t addr;

  if ((error = create_ds(app_dsm_id, L4_PAGESIZE, &addr, &cfg_env_ds,
			 "cfg infopage")) < 0)
    {
      printf("Error %d creating cfg infopage\n", error);
      return error;
    }

  cfg_env = (l4env_infopage_t*)addr;
  init_infopage(cfg_env);
  cfg_make_template();

  return 0;
}
