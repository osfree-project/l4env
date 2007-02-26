/* $Id$ */
/**
 * \file	loader/server/src/app.c
 *
 * \date	06/10/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * \brief	application data */

#define DEALLOC_CON

#include "app.h"

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>

#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/l4rm/l4rm.h>
#include <l4/exec/exec.h>
#include <l4/exec/elf.h>
#include <l4/exec/exec-client.h>
#ifdef DEALLOC_CON
#include <l4/con/con-client.h>
#endif
#include <l4/names/libnames.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/generic_ts/generic_ts-client.h>
#include <l4/loader/loader.h>
#include <l4/loader/grub_mb_info.h>
#include <l4/loader/grub_vbe_info.h>
#include <l4/rmgr/librmgr.h>

#include "fprov-if.h"
#include "dm-if.h"
#include "trampoline.h"
#include "pager.h"
#include "debug.h"

#define MAX_APP		32

/** \addtogroup constants Address Space Constants
 * These constants define where some special modules are mapped into
 * the address space of the target application. 
 * \note These pages are \b not direct mapped, even in applications which
 *       where created with the direct_mapped option!! L4Linux 2.2 is not
 *       an issue because it starts at the end of its binary image (_end) for
 *       scanning its address space for RAM. */
/*@{*/
/** position of L4 environment infopage in target application's address space */
#define APP_ENVPAGE	0x00007000
/** position of initial stack in target application's address space */
#define APP_STACK	0x00008000
/** potision of libloader.s.so in target application's address space */
#define APP_LIBLOADER	0x0000E000
/** potision of libloader.s.so in target application's address space
 * obsolete) */
#define APP_LIBL4RM	0x00010000
/*@}*/

#define APP_MCP		255
#define APP_STACK_SIZE	0x00002000

static int app_next_free = 0;			/**< number of next free app_t
						  in app_array */
static app_t app_array[MAX_APP];

static l4_threadid_t ts_id   = L4_INVALID_ID;	/**< task server */
static l4_threadid_t exec_id = L4_INVALID_ID;	/**< ELF interpreter */
static l4_threadid_t con_id  = L4_INVALID_ID;	/**< con server */

/** Return the address of the task struct
 *
 * \param tid		L4 thread ID
 * \return		task descriptor
 * 			NULL if not found */
app_t*
task_to_app(l4_threadid_t tid)
{
  int i;
  for (i=0; i<MAX_APP; i++)
// "if (l4_task_equal(app_array[i].tid, tid))" is not possible here because the
// real task id is still not known after allocating an task number at the
// task server.
    if (app_array[i].tid.id.task == tid.id.task)
      return app_array + i;

  return NULL;
}

/** Dump a message corresponding to an application. If the application
 * has already an task number, print it in hashes after the name of the
 * application. */
void
app_msg(app_t *app, const char *format, ...)
{
  char ftid[8];
  char fname[20];
  const char *c = strrchr(app->fname, '/');
  va_list list;

  if (!c)
    c = app->fname;
  else
    c++;
  strncpy(fname, c, sizeof(fname)-1);
  fname[sizeof(fname)-1] = '\0';

  *ftid = '\0';
  if (!l4_is_invalid_id(app->tid))
    sprintf(ftid, ",#%x", app->tid.id.task);

  va_start(list, format);
  printf("%s%s: ", fname, ftid);
  vprintf(format, list);
  printf("\n");
  va_end(list);
}

/** Debug function to show sections of the L4 environment infopage */
static void
dump_l4env_infopage(app_t *app)
{
  int i, j, k, pos=0;
  l4env_infopage_t *env = app->env;
  
  static char *type_names_on[] = 
    {"r", "w", "x", " reloc", " link", " page", " reserve", " share",
      " beg", " end", " errlink", " startup" };
  static char *type_names_off[] =
    {"-", "-", "-", "", "", "", "", "", "", "", "", "" };
  
  app_msg(app, "Dumping sections of environment page");
  
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
      for (j=1, k=0; j<=L4_DSTYPE_STARTUP; j<<=1, k++)
	{
	  if (l4exc->info.type & j)
	    printf("%s",type_names_on[k]);
	  else
	    printf("%s",type_names_off[k]);
	}
      printf("\n");
    }
}

/** Create a new app_t descriptor.
 *
 * \retval new_app	pointer to new app descriptor
 * \return		0 on success
 * 			-L4_ENOMEM if no descriptors available */
int
create_app_desc(app_t **new_app, const char *name)
{
  int counter;
  
  for (counter=MAX_APP; counter; app_next_free=0)
    {
      while ((app_next_free<MAX_APP) && counter--)
	{
	  if (app_array[app_next_free].env == NULL)
	    {
	      app_t *a = app_array + app_next_free++;
	      
	      a->env = (l4env_infopage_t*)1;	/* mark as allocated */
	      a->flags = 0;
	      a->app_area_next_free = 0;
	      a->tid = L4_INVALID_ID;		/* tid still unknown */
	      a->symbols = 0;			/* has no symbols */
	      a->lines = 0;			/* has no lines */
	      a->last_pf = 0xffffffff;
	      a->last_pf_eip = 0xffffffff;
	      a->fname = name;
	      
	      *new_app = a;
	      return 0;
	    }
	  app_next_free++;
	}
    }
  
  printf("Can't handle more than %d simultaneous applications\n", MAX_APP);
  return -L4_ENOMEM;
}

/** @name Application sections */
/*@{*/
/** Create a new area application descriptor
 *
 * \param app		application descriptor
 * \retval new_app_area	new application area descriptor
 * \return		0 on success
 * 			-L4_ENOMEM on failure */
static int
create_app_area_desc(app_t *app, app_area_t **new_app_area)
{
  app_area_t *aa;
  
  if (app->app_area_next_free >= MAX_APP_AREA)
    {
      printf("Can't handle more than %d sections per application\n", 
	      MAX_APP_AREA);
      return -L4_ENOMEM;
    }

  aa = app->app_area + app->app_area_next_free++;
  aa->type = 0;
  aa->flags = 0;
  aa->beg.here = 0;
  aa->ds = L4DM_INVALID_DATASPACE;

  *new_app_area = aa;
  return 0;
}

/** Check if new app_area overlaps with exisiting areas */
static int
sanity_check_app_area(app_t *app, app_area_t **check_aa)
{
  int i;
  app_area_t *aa;
 
  /* check for overlap */
  for (i=0; i<app->app_area_next_free; i++)
    {
      aa = app->app_area + i;
      if (   (aa->flags & APP_AREA_VALID)
	  && (*check_aa)->beg.app                     <  aa->beg.app+aa->size
	  && (*check_aa)->beg.app+(*check_aa)->size-1 >= aa->beg.app)
	{
	  app_msg(app, 
	          "app_area (%08x-%08x) overlaps:",
	         (*check_aa)->beg.app, (*check_aa)->beg.app+(*check_aa)->size);
	  for (i=0; i<app->app_area_next_free; i++)
	    {
	      aa = app->app_area + i;
	      if (aa->flags & APP_AREA_VALID)
      		app_msg(app, "  %08x-%08x => %08x-%08x",
		       aa->beg.app,  aa->beg.app+aa->size, 
		       aa->beg.here, aa->beg.here+aa->size);
	    }
	  return -L4_EINVAL;
	}
    }

  (*check_aa)->flags |= APP_AREA_VALID;
  return 0;
}

/** Find a free virtual address range. This is only necessary for
 * old-style applications because in new-style applications, we let
 * the region manager manager decide where a region lives. */
static l4_addr_t
app_find_free_virtual_area(app_t *app, l4_size_t size)
{
  int i;
  app_area_t *aa;
  l4_addr_t addr;
 
  addr = 0x00100000;	/* begin at 1MB */

  /* check for free area */
  for (i=0; i<app->app_area_next_free; i++)
    {
      aa = app->app_area + i;
      if (    (aa->flags & APP_AREA_VALID)
	   && (addr      <  aa->beg.app+aa->size)
	   && (addr+size >= aa->beg.app))
	{
	  /* we have a clash. Set addr to the end of this area. */
	  addr = aa->beg.app + aa->size;

	  /* restart because list of addresses is not sorted */
	  i=0;
	}
    }
  
  /* we went through. Our address may be after the last area. */
  if (addr >= 0xC0000000)
    return 0;
  
  return addr;
}

/** Dump all app_areas for debugging purposes */
void
app_list_addr (l4_threadid_t tid)
{
  int i;
  app_t *app;
  app_area_t *aa;

  if (!(app = task_to_app(tid)))
    {
      printf("Can't list task %x", tid.id.task);
      return;
    }

  app_msg(app, "Dumping app addresses");

  /* Go through all app_areas we page for the application */
  for (i=0; i<app->app_area_next_free; i++)
    {
      aa = app->app_area + i;
      printf("  %08x-%08x => %08x-%08x\n", 
	  aa->beg.app,  aa->beg.app+aa->size, 
	  aa->beg.here, aa->beg.here+aa->size);
    }
}

/** Attach dataspace to pager. Therefore it is accessible by the application.
 *
 * \param app		application
 * \param ds		dataspace to attach
 * \param addr		start address of dataspace inside application.
 * 			If L4_MAX_ADDRESS, then do not make the dataspace
 * 			accessible from the application but only register
 * 			to allow smoothly shutdown of dataspace on exit
 * 			of the application.
 * \param size		size of dataspace
 * \param type		type
 * \param rights	rights for the pager
 * \param dbg_name	name of application area for debugging purposes
 * \retval aa		application area */
static int
app_attach_ds_to_pager(app_t *app, l4dm_dataspace_t *ds,
		       l4_addr_t addr, l4_size_t size, 
		       l4_uint16_t type, l4_uint32_t rights,
		       const char *dbg_name, app_area_t **aa)
{
  int error;
  l4_addr_t sec_end;
  app_area_t *new_aa;
 
  if ((error = create_app_area_desc(app, &new_aa)))
    return error;

  new_aa->beg.app  = l4_trunc_page(addr);
  new_aa->beg.here = 0;
  sec_end          = l4_round_page(addr + size);
  new_aa->size     = sec_end - new_aa->beg.app;
  new_aa->type     = type;
  new_aa->flags    = APP_AREA_PAGE;  /* forward pf to ds */
  new_aa->dbg_name = dbg_name;
  new_aa->ds       = *ds;

  /* if addr == L4_MAX_ADDRESS then we do not have to page the
   * dataspace and therefore don't have to check for overlaps
   * here */
  if (addr != L4_MAX_ADDRESS)
    {
      if ((error = sanity_check_app_area(app, &new_aa)))
	/* app areas overlap */
	return error;

      /* alllow the pager to access the dataspace */
      if ((error = l4dm_share(ds, app_pager_id, rights)))
	{
	  printf("Error %d sharing ds to pager\n", error);
	  return error;
	}
    }
  
  *aa = new_aa;
  
  return 0;
}

/** Attach the sections of an ELF binary which should be paged by the loader.
 * 
 * \param l4exc_start	first l4exec_section
 * \param env		L4 environment infopage
 * \param app		application
 * \return		0 on success
 * 			-L4_ENOMEM if no app descriptor is available */
static int
app_attach_section_to_pager(l4exec_section_t *l4exc_start,
			    l4env_infopage_t *env, app_t *app)
{
  int error;
  l4exec_section_t *l4exc;
  l4exec_section_t *l4exc_stop = env->section + env->section_num;
 
  for (l4exc=l4exc_start; l4exc<l4exc_stop; l4exc++)
    {
      app_area_t *aa;
      l4_uint32_t rights = (l4exc->info.type & L4_DSTYPE_WRITE) 
			? L4DM_RW : L4DM_RO;
      
      /* make section pageable */
      if ((error = app_attach_ds_to_pager(app, &l4exc->ds, 
					  l4exc->addr, l4exc->size,
					  l4exc->info.type, rights, 
					  "program section", &aa)))
	return error;
      
      /* reset flag that section should be attached */
      l4exc->info.type &= ~L4_DSTYPE_PAGEME;

      if (l4exc->info.type & L4_DSTYPE_OBJ_END)
	break;
    }

  return 0;
}

/** Go through all sections of the infopage and try to attach the section to
 * our address space so the pager thread can page the section. We only attach
 * sections which are
 * 
 *   (1) relocated (L4_DSTYPE_RELOCME bit is cleared)
 *   (2) are not yet attached (L4_DSTYPE_PAGEME bit is set)
 *   
 * \param app		application descriptor
 * \return		0 on success 
 * 			-L4_ENOMEM if no app descriptor is available */
static int
app_attach_sections_to_pager(app_t *app)
{
  int error;
  l4env_infopage_t *env = app->env;
  l4exec_section_t *l4exc;
  l4exec_section_t *l4exc_stop = env->section + env->section_num;
  
#ifdef DEBUG_RESERVEAREA
  app_msg(app, "Scanning %d sections for attach", env->section_num);
#endif
      
  for (l4exc = env->section; l4exc<l4exc_stop; l4exc++)
    if (l4exc->info.type & L4_DSTYPE_OBJ_BEGIN)
      if ((!(l4exc->info.type & L4_DSTYPE_RELOCME)) &&
	  (  l4exc->info.type & L4_DSTYPE_PAGEME))
	{
	  if ((error = app_attach_section_to_pager(l4exc, env, app)))
	    return error;
	}
  
  return 0;
}

static int
app_attach_startup_sections_to_pager(app_t *app)
{
  int error;
  l4env_infopage_t *env = app->env;
  l4exec_section_t *l4exc;
  l4exec_section_t *l4exc_stop = env->section + env->section_num;
  
#ifdef DEBUG_RESERVEAREA
  app_msg(app, "Scanning %d sections for attach startup", env->section_num);
#endif
      
  for (l4exc = env->section; l4exc<l4exc_stop; l4exc++)
    {
      if ((l4exc->info.type & L4_DSTYPE_STARTUP) &&
	  (l4exc->info.type & L4_DSTYPE_PAGEME))
	{
	  if (l4exc->info.type & L4_DSTYPE_RELOCME)
	    {
	      app_msg(app, "Startup section %d not yet relocated\n",
			   l4exc-env->section);
	      return -L4_EINVAL;
	    }
	  if ((error = app_attach_section_to_pager(l4exc, env, app)))
	    return error;
	}
    }
  
  return 0;
}

/** Create dataspace of anonymous memory mapped into the new application.
 * 
 * \param app_addr	virtual address of dataspace in application
 *			may be L4_MAX_ADDRESS of area is not pageable to
 *			application (status information an so on)
 * \param size		size of dataspace
 * \param app		pointer to application descriptor
 * \retval ret_aa	filled out app_area
 * \return		0 on success */
static int
app_create_ds(app_t *app, l4_addr_t app_addr, l4_size_t size,
              app_area_t **ret_aa, const char *dbg_name)
{
  int error;
  l4_addr_t here;
  l4dm_dataspace_t ds;

  if ((error = create_ds(app_dm_id, size, &here, &ds, dbg_name)))
    return error;

  /* make dataspace pageable */
  if ((error = app_attach_ds_to_pager(app, &ds, app_addr, size,
				      L4_DSTYPE_READ | L4_DSTYPE_WRITE,
			    	      L4DM_RW, dbg_name, ret_aa)))
    return error;

  (*ret_aa)->beg.here = here;
  
  return 0;
}

/** Create the application's infopage
 *
 * \param app		application descriptor
 * \return		0 on success */
static int
app_create_infopage(app_t *app)
{
  int error;
  app_area_t *aa;
  char dbg_name[32];
  
  strcpy(dbg_name, "info ");
  strncat(dbg_name, app->fname, sizeof(dbg_name)-strlen(dbg_name));
  dbg_name[sizeof(dbg_name)-1] = '\0';

  /* create infopage */
  if ((error = app_create_ds(app, APP_ENVPAGE, L4_PAGESIZE, &aa, dbg_name)))
    return error;

  app->env = (l4env_infopage_t*)aa->beg.here;

  return 0;
}
/*@}*/

/** @name Manipulate Fiasco debug information */
/*@{ */
/** Request symbolic information for an application from the L4 exec layer */
static int
app_get_symbols(app_t *app)
{
  int error;
  l4dm_dataspace_t ds;
  l4_addr_t addr;
  l4_size_t psize;
  sm_exc_t exc;
  
  if ((error = l4exec_bin_get_symbols(exec_id, 
				      (l4exec_envpage_t_slice*)app->env,
				      (l4exec_dataspace_t*)&ds, &exc)))
    {
      app_msg(app, "Error %d (%s) getting symbols", 
	      error, l4env_errstr(error));
      return -L4_EINVAL;
    }

  if (l4dm_is_invalid_ds(ds))
    {
      app_msg(app, "No symbols");
      return -L4_EINVAL;
    }

  if (!l4dm_mem_ds_is_contiguous(&ds))
    {
      app_msg(app, "ds %d not contiguous (bug at exec)!", ds.id);
      l4dm_close(&ds);
      return -L4_EINVAL;
    }
  
  if ((error = l4dm_mem_ds_phys_addr(&ds, 0, L4DM_WHOLE_DS, &addr, &psize)))
    {
      app_msg(app, "Error %d (%s) requesting physical addr of ds %d", 
	      error, l4env_errstr(error), ds.id);
      l4dm_close(&ds);
      return -L4_EINVAL;
    }

  app->symbols_ds = ds;   /* needed for transfering ownership to app when
			   * we know the task_id */
  app->symbols    = addr; /* physical address */

  return 0;
}

/** Request line number information of an application by the L4 exec layer */
static int
app_get_lines(app_t *app)
{
  int error;
  l4dm_dataspace_t ds;
  l4_addr_t addr;
  l4_size_t psize;
  sm_exc_t exc;
  
  if ((error = l4exec_bin_get_lines(exec_id,
				    (l4exec_envpage_t_slice*)app->env,
				    (l4exec_dataspace_t*)&ds, &exc)))
    {
      app_msg(app, "Error %d (%s) getting lines", 
	            error, l4env_errstr(error));
      return -L4_EINVAL;
    }

  if (l4dm_is_invalid_ds(ds))
    {
      app_msg(app, "No lines");
      return -L4_EINVAL;
    }

  if (!l4dm_mem_ds_is_contiguous(&ds))
    {
      app_msg(app, "ds %d not contiguous (bug at exec)!", ds.id);
      l4dm_close(&ds);
      return -L4_EINVAL;
    }

  if ((error = l4dm_mem_ds_phys_addr(&ds, 0, L4DM_WHOLE_DS, &addr, &psize)))
    {
      app_msg(app, "Error %d (%s) requesting physical addr of ds %d", 
	            error, l4env_errstr(error), ds.id);
      l4dm_close(&ds);
      return -L4_EINVAL;
    }

  app->lines_ds = ds;   /* needed for transfering ownership to app when we
			 * know the task_id */
  app->lines    = addr; /* physical address */

  return 0;
}

/** Make symbolic information known by Fiasco */
static inline void
app_publish_symbols(app_t *app)
{
  asm ("int $3 ; cmpb $30,%%al"
       : : "a"(app->symbols), "b"(app->tid.id.task), "c" (1));
}

/** Make debug line number information known by Fiasco */
static inline void
app_publish_lines(app_t *app)
{
  asm ("int $3 ; cmpb $30,%%al"
       : : "a"(app->lines), "b"(app->tid.id.task), "c" (2));
}

static void
app_publish_lines_symbols(app_t *app)
{
  if (app->flags & APP_SYMBOLS)
    {
      if (app_get_symbols(app))
	app->flags &= ~APP_SYMBOLS;
      else
	{
	  /* tell Fiasco where the symbols are. If error, ignore it */
	  app_publish_symbols(app);
	  l4dm_transfer(&app->symbols_ds, app->tid);
	}
    }

  if (app->flags & APP_LINES)
    {
      if (app_get_lines(app))
	app->flags &= ~APP_LINES;
      else
	{
	  /* tell Fiasco where the symbols are. If error, ignore it */
	  app_publish_lines(app);
	  l4dm_transfer(&app->lines_ds, app->tid);
	}
    }
}

/** Retire symbolic debug information of an application from Fiasco */
static inline void
app_unpublish_symbols(app_t *app)
{
  if (app->symbols && !l4_is_invalid_id(app->tid))
    {
      asm ("int $3 ; cmpb $30,%%al"
	  : : "a"(0), "b"(app->tid.id.task), "c" (1));
    }
}

/** Retire symbolic debug information of an application from Fiasco */
static inline void
app_unpublish_lines(app_t *app)
{
  if (app->lines && !l4_is_invalid_id(app->tid))
    {
      asm ("int $3 ; cmpb $30,%%al"
	  : : "a"(0), "b"(app->tid.id.task), "c" (2));
    }
}

/*@} */

/** Initialize the application's environment infopage.
 *
 * \param app		app descriptor
 * \return		0 on success */
static int
app_init_infopage(app_t *app)
{
  l4env_infopage_t *env = app->env;

  /* fill out infopage */
  env->stack_size  = APP_STACK_SIZE;
  env->stack_dm_id = app_stack_dm;
  env->loader_id   = l4_myself();
  env->image_dm_id = app_image_dm;
  env->text_dm_id  = app_text_dm;
  env->data_dm_id  = app_data_dm;

  env->memserv_id  = app_dm_id;
  
  env->ver_info.arch_class = ARCH_ELF_ARCH_CLASS;
  env->ver_info.arch_data  = ARCH_ELF_ARCH_DATA;
  env->ver_info.arch       = ARCH_ELF_ARCH;

  /* set library addresses */
  env->addr_libloader = APP_LIBLOADER;
  env->addr_libl4rm   = APP_LIBL4RM;

  /* set default path for dynamic libraries */
  strcpy(env->binpath, cfg_binpath);
  strcpy(env->libpath, cfg_libpath);

  env->vm_low = 0x1000;
  env->vm_high = 0xC0000000;

  return 0;
}

/** allocate space from a "page" _downwards_ */
static int
alloc_from_tramp_page(l4_size_t size, 
		      app_addr_t *tramp_page, l4_addr_t *tramp_page_addr)
{
  l4_addr_t new_pageptr = (*tramp_page_addr - size) & ~3;

  if (new_pageptr < tramp_page->here)
    return 0;

  *tramp_page_addr = new_pageptr;
  return new_pageptr;
}

/** @name Create application resources */
/*@{*/
/** Load boot modules.
 *
 * \param ct		config task descriptor
 * \param app		application descriptor
 * \param app_page	base address in our / application's address space
 * \param page_addr	address in our address space
 * \param mbi		pointer to mbi
 * \param fprov_id	file provider
 * \return		0 on success
 * 			-L4_ENOMEM if not enough space in page */
static int
load_modules(cfg_task_t *ct, app_t *app, l4_threadid_t fprov_id,
	     app_addr_t *tramp_page, l4_addr_t *tramp_page_addr,
	     struct grub_multiboot_info *mbi)
{
  if (ct->next_module > ct->module)
    {
      int mods_count = ct->next_module-ct->module;
      struct grub_mod_list *mod;
      cfg_module_t *ct_mod = ct->module;
      
      if (!(mod = (struct grub_mod_list*)
	    alloc_from_tramp_page(mods_count*sizeof(struct grub_mod_list),
				  tramp_page, tramp_page_addr)))
	{
	  app_msg(app, "No space left for multiboot modules in \
		        trampoline page");
	  return -L4_ENOMEM;
	}

      mbi->mods_addr  = HERE_TO_APP(mod, *tramp_page);
      mbi->mods_count = mods_count;

      for (; mods_count--; mod++, ct_mod++)
	{
	  l4dm_dataspace_t ds;
	  l4_size_t file_size;
	  l4_size_t mod_size;
	  app_area_t *aa;
	  int error;

	  app_msg(app, "Loading module \"%s\"", ct_mod->fname);

	  /* Request module file image from file provider. If application 
	   * should be loaded direct mapped, load the image contiguous. 
	   * Therefore the physical address of the first byte is equal to 
	   * the physical address of the whole dataspace.
	   *
	   * We don't attach the module to our address space since we do
	   * nothing with app modules. These dataspaces get paged by the
	   * application's region manager. */
	  if ((error = load_file(ct_mod->fname, fprov_id, app_dm_id, 
				 /*use_modpath=*/1, /*contiguous=*/ 
				 app->flags & APP_DIRECTMAP ? 1 : 0,
				 0 /*don't attach to our address space*/,
				 &file_size, &ds)))
	    {
	      app_msg(app, "Error %d (%s) loading module \"%s\"", 
			   error, l4env_errstr(error), ct_mod->fname);
	      return error;
	    }

	  /* XXX too restrictive? */
	  mod_size = l4_round_page(file_size);

	  /* if application is direct mapped, obtain the start-address of the
	   * module from dataspace manager (old-style or new-style) */
	  if (app->flags & APP_DIRECTMAP)
	    {
	      /* Application is direct mapped so address of boot module should
	       * be direct mapped too */
	      if ((error = phys_ds(&ds, file_size, 
				   (l4_addr_t*)&mod->mod_start))<0)
		return error;

	      mod->mod_end = mod->mod_start + mod_size;
	    }
	  else if (app->flags & APP_OLDSTYLE)
	    {
	      /* place module somewhere in the virtual address space of
	       * the application */
	      l4_addr_t addr;

	      if (!(addr = app_find_free_virtual_area(app, mod_size)))
		{
		  app_msg(app, "Couldn't find a free vm area of size %08x"
			       " for a module", mod_size);
		  return -L4_EINVAL;
		}
      
	      mod->mod_start = addr;
	      mod->mod_end   = addr + mod_size;
	    }
  
	  if (app->flags & APP_OLDSTYLE)
	    {
	      /* old-style: make module accessible from application
	       * -- paged by application pager.
	       * XXX We could page modules by the applications region manager
	       * but we don't pass the L4env infopage to the application so
	       * the application doesn't know the datatspace id of the 
	       * module. */
	      if ((error = app_attach_ds_to_pager(app, &ds, 
		 			  mod->mod_start, mod_size,
					  L4_DSTYPE_READ | L4_DSTYPE_WRITE,
					  L4DM_RW, "boot module", &aa)))
		return error;
	    }
	  else
	    {
	      /* new-style: make module accessible from application
	       * -- additional program section */
	      l4env_infopage_t *env = app->env;
	      l4exec_section_t *l4exc;

	      if (env->section_num >= L4ENV_MAXSECT)
		return -L4_ENOMEM;

	      l4exc = env->section + env->section_num;
	      env->section_num++;

	      l4exc->size = mod_size;
	      l4exc->info.id = 0;
	      l4exc->info.type = L4_DSTYPE_READ      | L4_DSTYPE_WRITE
			       | L4_DSTYPE_OBJ_BEGIN | L4_DSTYPE_OBJ_END;

	      if (app->flags & APP_DIRECTMAP)
		{
		  /* address known (physical address of ds) */
		  l4exc->info.type |= L4_DSTYPE_PAGEME;
		  l4exc->addr = mod->mod_start;
		}
	      else
		{
		  /* address still unknown -- let the application's region
		   * manager decide */
		  l4exc->info.type |= L4_DSTYPE_RELOCME | L4_DSTYPE_PAGEME;
		  l4exc->addr = 0;
		  /* HACK! The module address is the offset of the program
		   * section inside the L4 environment infopage so the 
		   * libloader knows that the real module address is the
		   * address of the relocated module */
		  mod->mod_start = (l4_addr_t)(l4exc - env->section);
		  mod->mod_end   = 0;
		}

	      l4exc->ds = ds;

	      /* transfer ownership of module to application so the dataspace
	       * gets freed after the application's end */
	      l4dm_transfer(&ds, app->tid);

	      l4exc->info.type |= L4_DSTYPE_APP_IS_OWNER;
	    }

	  /* copy command line arguments */
	  if (ct_mod->args)
	    {
	      char *args;
	      int args_len = strlen(ct_mod->args);
	      if (!(args = (char*)alloc_from_tramp_page(args_len, 
							tramp_page,
						        tramp_page_addr)))
		{
		  app_msg(app, "No space left for command line of module "
			       "\"%s\"\n in trampoline page",
			       ct_mod->args);
		  return -L4_ENOMEM;
		}

	      memcpy(args, ct_mod->args, args_len);
	      mod->cmdline = HERE_TO_APP(args, *tramp_page);
	    }
	}
      mbi->flags |= MB_INFO_MODS;
    }

  return 0;
}

static void
load_vbe_info(app_t *app,
	      app_addr_t *tramp_page, l4_addr_t *tramp_page_addr,
	      struct grub_multiboot_info *_mbi, struct grub_multiboot_info *mbi)
{
  /* copy mb_info->vbe_mode and mb_info->vbe_controller */
  if (mbi->flags & MB_INFO_VIDEO_INFO)
    {
      l4_addr_t tmp_tramp_page_addr = *tramp_page_addr;
      struct vbe_mode *mbi_vbe_mode;
      struct vbe_controller *mbi_vbe_cont;

      if (mbi->vbe_mode_info)
	{
	  if (   (mbi_vbe_mode = (struct vbe_mode*)
		    alloc_from_tramp_page(sizeof(struct vbe_mode),
					  tramp_page, &tmp_tramp_page_addr))
	      && (mbi_vbe_cont = (struct vbe_controller*)
		    alloc_from_tramp_page(sizeof(struct vbe_controller),
					  tramp_page, &tmp_tramp_page_addr))
	      )
	    {
    	      *mbi_vbe_mode = *((struct vbe_mode*)_mbi->vbe_mode_info);
	      *mbi_vbe_cont = *((struct vbe_controller*)_mbi->vbe_control_info);
	      mbi->vbe_mode_info    = HERE_TO_APP(mbi_vbe_mode, *tramp_page);
	      mbi->vbe_control_info = HERE_TO_APP(mbi_vbe_cont, *tramp_page);
	      *tramp_page_addr = tmp_tramp_page_addr;
	    }
	  else
	    {
	      app_msg(app, "Can't pass VBE video info to trampoline page \
			   (needs %d bytes)",
     			   sizeof(struct vbe_controller)+
			   sizeof(struct vbe_mode));
	      mbi->flags &= ~MB_INFO_VIDEO_INFO;
    	    }
	}
    }
}

/** Create multiboot info at stack page. 
 * Load boot modules.
 *
 * \param ct		config task descriptor
 * \param app		application descriptor
 * \param tramp_page	base address in our / application's address space
 * \param tramp_page_addr address in our address space
 * \param mbi		pointer to mbi
 * \param fprov_id	file provider
 * \return		0 on success
 * 			-L4_ENOMEM if not enough space in page */
static int
app_create_mb_info(cfg_task_t *ct, app_t *app, app_addr_t *tramp_page,
		   l4_addr_t *tramp_page_addr, l4_threadid_t fprov_id,
		   struct grub_multiboot_info **mbi)
{
  char *args;
  int args_len;
  struct grub_multiboot_info *mbi1;
  extern struct grub_multiboot_info *_mbi;
  int error;

  /* allocate space for multiboot info */
  if (!(mbi1 = (struct grub_multiboot_info*)
	alloc_from_tramp_page(sizeof(struct grub_multiboot_info),
			      tramp_page, tramp_page_addr)))
    {
      app_msg(app, "No space left for grub_multiboot_info in trampoline page");
      return -L4_ENOMEM;
    }

  /* copy multiboot_info */
  *mbi1 = *_mbi;
  mbi1->flags &= MB_INFO_MEMORY|MB_INFO_BOOTDEV|MB_INFO_VIDEO_INFO;

  /* copy program arguments */
  args_len = strlen(ct->task.fname)+2; /* space between argv[0] and argv[1]
					* and terminating '\0' */
  if (ct->task.args)
    args_len += strlen(ct->task.args);

  if (!(args = (char*)alloc_from_tramp_page(args_len, 
					    tramp_page, tramp_page_addr)))
    {
      app_msg(app, "No space left for command line in trampoline page");
      return -L4_ENOMEM;
    }
  
  mbi1->flags |= MB_INFO_CMDLINE;
  mbi1->cmdline = HERE_TO_APP(args, *tramp_page);
  strcpy(args, ct->task.fname);

  if (ct->task.args)
    {
      strcat(args, " ");
      strcat(args, ct->task.args);
    }

  /* load the VESA information about the current video mode as set by GRUB */
  load_vbe_info(app, tramp_page, tramp_page_addr, _mbi, mbi1);

  /* load applications modules */
  if ((error = load_modules(ct, app, fprov_id, 
			    tramp_page, tramp_page_addr, mbi1)) < 0)
    return error;

  *mbi = mbi1;

  return 0;
}

/** Create physical memory for an application */
static int
app_create_phys_memory(app_t *app, l4_size_t size, int cfg_flags)
{
  int pool, error;
  l4_addr_t addr;
  l4_uint32_t flags;
  app_area_t *aa;
  l4_size_t psize;
  l4dm_dataspace_t ds;
  char ds_name[L4DM_DS_NAME_MAX_LEN];
  const char *c;
  char ds_flags[20];

  if ((c = strrchr(app->fname, '/')))
    c++;
  else
    c = app->fname;
  snprintf(ds_name, sizeof(ds_name)-1, "%s %s", "phys mem", c);
  ds_name[sizeof(ds_name)-1] = '\0';

  if (!app->flags & APP_OLDSTYLE)
    {
      app_msg(app, "Can't create physical memory for new-style application\n");
      return -L4_EINVAL;
    }

  flags = 0;
  if (cfg_flags & CFG_M_DMA_ABLE)
    pool  = L4DM_MEMPHYS_ISA_DMA;
  else
    pool = L4DM_MEMPHYS_DEFAULT;

  sprintf(ds_flags, "pool=%d", pool);

  if (cfg_flags & (CFG_M_CONTIGUOUS | CFG_M_DIRECT_MAPPED | CFG_M_DMA_ABLE))
    {
      /* request contiguous dataspace */
      flags |= L4DM_CONTIGUOUS;
      strcat(ds_flags, ",cont");
    }

  if (size % L4_SUPERPAGESIZE == 0)
    {
      /* request 4MB pages */
      flags |= L4DM_MEMPHYS_4MPAGES;
      strcat(ds_flags, ",4MB");
    }

  if ((error = l4dm_memphys_open(pool, L4DM_MEMPHYS_ANY_ADDR,
				 size, 0, flags, ds_name, &ds)))
    {
      app_msg(app, "Error %d reserving %08x of physical memory (%s)",
		   error, size, ds_flags);
      return error;
    }

  if ((error = l4dm_mem_ds_phys_addr(&ds, 0, L4DM_WHOLE_DS,
				     &addr, &psize)))
    {
      app_msg(app, "Error %d determining address of physmem",
		   error);
      return error;
    }

  if (psize != size)
    {
      app_msg(app, "only %08x/%08x bytes of physmem contiguous",
		   psize, size);
      return -L4_ENOMEM;
    }

  /* make physical memory accessible from application */
  if ((error = app_attach_ds_to_pager(app, &ds, addr, size,
				      L4_DSTYPE_READ | L4_DSTYPE_WRITE,
				      L4DM_RW, "phys memory", &aa)))
    return error;
  
  app_msg(app, "Reserved %08x memory at %08x-%08x",
	      size, aa->beg.app, aa->beg.app+aa->size);

  return 0;
}
/*@}*/

/** @name Start application */
/*@{*/
/** Start app the new way 
 *
 * \param fname		file name of new application
 * \param app		application descriptor
 * \return		0 on success */
static int
app_start_libloader(cfg_task_t *ct, app_t *app)
{
  int error;
  l4_msgdope_t result;
  l4_umword_t dw0, dw1;
  l4_addr_t tramp_page_addr;
  app_addr_t tramp_page;
  app_area_t *tramp_page_aa;
  l4_addr_t esp, app_esp, app_eip;
  l4env_infopage_t *env = app->env;
  struct grub_multiboot_info *mbi;
  sm_exc_t exc;
  char dbg_name[32];

  app->flags &= ~APP_OLDSTYLE;

#ifdef DEBUG_DUMP_INFOPAGE
  dump_l4env_infopage(app);
#endif

  /* First stage: attach startup sections which was marked by exec
   * (usually libloader.s containing the region manager). */
  if ((error = app_attach_startup_sections_to_pager(app)))
    {
      app_msg(app, "Error %d (%s) attaching sections", 
		   error, l4env_errstr(error));
      return error;
    }

  /* ask task server to create new task */
  if ((error = l4_ts_allocate(ts_id, (l4_ts_taskid_t*)&app->tid, &exc))
      || exc._type != exc_l4_no_exception)
    {
      printf("Error %d (%s) allocating task at task server (exc=%d)\n",
	      error, l4env_errstr(error), exc._type);
      return error;
    }

  strcpy(dbg_name, "tramp ");
  strncat(dbg_name, app->fname, sizeof(dbg_name)-strlen(dbg_name));
  dbg_name[sizeof(dbg_name)-1] = '\0';

  /* create stack */
  if ((error = app_create_ds(app, APP_STACK, env->stack_size, &tramp_page_aa,
			     dbg_name)))
    return error;

  tramp_page      = tramp_page_aa->beg;
  env->stack_low  = tramp_page.app;
  env->stack_high = tramp_page.app  + env->stack_size;
  tramp_page_addr = tramp_page.here + env->stack_size;
  
  /* create boot info, load boot modules */
  if ((error = app_create_mb_info(ct, app, &tramp_page, &tramp_page_addr,
				  app->env->fprov_id, &mbi)))
    return error;

  env->addr_mb_info = HERE_TO_APP(mbi, tramp_page);
  
  /* allocate stack */
  if (!(esp = alloc_from_tramp_page(0x400, &tramp_page, &tramp_page_addr)))
    {
      app_msg(app, "No space left for stack in trampoline page "
	     "(have %d bytes, need %d bytes)",
	     tramp_page_addr - tramp_page.here, 0x400);
      return -L4_ENOMEM;
    }
  esp += 0x400;

  /* put function parameters for l4loader_init on top of stack */
  *--((l4_umword_t*)esp) = (l4_umword_t)APP_ENVPAGE;
  *--((l4_umword_t*)esp) = (l4_umword_t)0;

  /* set application's stack pointer and instruction pointer */
  app_esp = HERE_TO_APP(esp, tramp_page);
  app_eip = env->entry_1st;

  /* share all program sections not yet transfered to the application */
    {
      l4exec_section_t *l4exc;
      l4exec_section_t *l4exc_stop;
      
      l4exc_stop = app->env->section + app->env->section_num;
      for (l4exc=app->env->section; l4exc<l4exc_stop; l4exc++)
	{
	  if (!(l4exc->info.type & L4_DSTYPE_APP_IS_OWNER))
	    { 
	      l4_uint32_t rights = l4exc->info.type & L4_DSTYPE_WRITE 
				? L4DM_RW : L4DM_RO;
	      l4dm_share(&l4exc->ds, app->tid, rights);
	    }
	}
    }

  app_msg(app, "Starting at l4loader_init (%08x, libloader.s.so)", 
		env->entry_1st);

  if ((error = l4_ts_create(ts_id, (l4_ts_taskid_t*)&app->tid,
			    (l4_umword_t)app_eip, (l4_umword_t)app_esp,
			    APP_MCP, (l4_ts_taskid_t*)&app_pager_id,
			    app->prio, app->fname, 0, &exc))
      || exc._type != exc_l4_no_exception)
    {
      printf("Error %d (%s) creating task %x at task server (exc=%d)\n",
	      error, l4env_errstr(error), app->tid.id.task, exc._type);
      if (l4_ts_free(ts_id, (l4_ts_taskid_t*)&app->tid, &exc)
	  || exc._type != exc_l4_no_exception)
	app_msg(app, "Error freeing task number at task server");
      return error;
    }

  /* wait for message of libloader that we should complete
   * the loading process */
  for (;;)
    {
      l4_i386_ipc_receive(app->tid, L4_IPC_SHORT_MSG, &dw0, &dw1,
			  L4_IPC_NEVER, &result);
      if (dw0 == L4_LOADER_ERROR)
	{
	  app_msg(app, "Error from libloader");
	  return 1;
	}
      else if (dw0 != L4_LOADER_COMPLETE)
	{
	  app_msg(app, "What? Got %08x from started application...", dw0);
	  enter_kdebug("start_app");
	}
      else
	break;
    }

#ifdef DEBUG_DUMP_INFOPAGE
  dump_l4env_infopage(app);
#endif

  if ((error = l4exec_bin_link(exec_id, (l4exec_envpage_t_slice*)env, &exc)))
    {
      app_msg(app, "Error %d (%s) while finish linking",
		   error, l4env_errstr(error));
      return error;
    }

  app_publish_lines_symbols(app);
 
  app_msg(app, "Continue at l4env_init (%08x, libloader.s.so)", 
		env->entry_2nd);

  /* signal the loader that we are ready */
  l4_i386_ipc_send(app->tid, 
		   L4_IPC_SHORT_MSG, L4_LOADER_COMPLETE, APP_ENVPAGE,
		   L4_IPC_NEVER, &result);

  return 0;
}

/** Start application the old way.
 *
 * Create stack page, create a valid multiboot info structure and
 * copy the trampoline code. Then start the new task
 *
 * \param ct		task descriptor
 * \param app		application descriptor
 * \return		0 on success */
static int
app_start_static(cfg_task_t *ct, app_t *app)
{
  int error, task_trampoline_size, i;
  int relink;
  l4_addr_t tramp_page_addr;
  l4_addr_t esp, app_esp, app_eip;
  app_addr_t tramp_page;
  app_area_t *tramp_page_aa;
  l4env_infopage_t *env = app->env;
  struct grub_multiboot_info *mbi;
  cfg_mem_t *ct_mem;
  sm_exc_t exc;
  char dbg_name[32];

  app->flags |= APP_OLDSTYLE;

  /* attach relocated sections (usual the application itself) */
  if ((error = app_attach_sections_to_pager(app)))
    {
      app_msg(app, "Error %d (%s) attaching sections",
		   error, l4env_errstr(error));
      return error;
    }

#ifdef DEBUG_DUMP_INFOPAGE
  dump_l4env_infopage(app);
#endif

  /* first, allocate task number */
  if ((error = l4_ts_allocate(ts_id, (l4_ts_taskid_t*)&app->tid, &exc))
      || exc._type != exc_l4_no_exception)
    {
      printf("Error %d (%s) allocating task at task server (exc=%d)\n",
	  error, l4env_errstr(error), exc._type);
      return error;
    }

  /* check if some section has to be relocated */
  relink = 0;
  for (i=0; i<env->section_num; i++)
    {
      if (env->section[i].info.type & L4_DSTYPE_RELOCME)
	{
	  l4exec_section_t *l4exc = env->section + i;
	  l4_addr_t addr;
	  l4_size_t psize;
	  
	  if ((error = l4dm_mem_ds_phys_addr(&l4exc->ds, 0, L4DM_WHOLE_DS,
					     &addr, &psize)))
	    {
	      app_msg(app, "Error %d requesting physical address of ds %d",
		  	   error, l4exc->ds.id);
	      return -L4_EINVAL;
	    }

	  app_msg(app, "Relocating section %08x-%08x to %08x",
		       l4exc->addr, l4exc->addr+l4exc->size, addr);
	  
	  l4exc->addr = addr;
	  l4exc->info.type &= ~L4_DSTYPE_RELOCME;
	  relink = 1;
	}
    }
  
  if (relink)
    {
      if ((error = l4exec_bin_link(exec_id,
				   (l4exec_envpage_t_slice*)env, &exc)))
	{
	  app_msg(app, "Error %d (%s) while finish linking",
		        error, l4env_errstr(error));
	  return error;
	}
      /* attach relocated sections (usual the application itself) */
      if ((error = app_attach_sections_to_pager(app)))
	{
	  app_msg(app, "Error %d (%s) attaching sections",
   	      error, l4env_errstr(error));
       	  return error;
	}
    }

  app_publish_lines_symbols(app);
  
  strcpy(dbg_name, "tramp ");
  strncat(dbg_name, app->fname, sizeof(dbg_name)-strlen(dbg_name));
  dbg_name[sizeof(dbg_name)-1] = '\0';

  /* create trampoline page */
  if ((error = app_create_ds(app, APP_STACK, env->stack_size, &tramp_page_aa,
			     dbg_name)))
    return error;

  tramp_page      = tramp_page_aa->beg;
  env->stack_low  = tramp_page.app;
  env->stack_high = tramp_page.app  + env->stack_size;
  tramp_page_addr = tramp_page.here + env->stack_size;

  /* reserve physical memory regions */
  for (ct_mem=ct->mem; ct_mem<ct->next_mem; ct_mem++)
    {
      if ((error = app_create_phys_memory(app, ct_mem->size, ct_mem->flags)))
	return error;
    }

  /* create boot info, load boot modules */
  if ((error = app_create_mb_info(ct, app, &tramp_page, &tramp_page_addr, 
				  app->env->fprov_id, &mbi)))
    return error;

  /* allocate stack */
  if (!(esp = alloc_from_tramp_page(0x400, &tramp_page, &tramp_page_addr)))
    {
      app_msg(app, 
	     "No space left for stack in trampoline page "
	     "(have %d bytes, need %d bytes)",
	     tramp_page_addr - tramp_page.here, 0x400);
      return -L4_ENOMEM;
    }
  esp += 0x400;

  /* copy task_trampoline PIC code on top of stack */
  task_trampoline_size = ((unsigned int)&_task_trampoline_end
                        - (unsigned int)task_trampoline);

  /* align stack pointer to dword */
  esp -= ((task_trampoline_size + 3) & ~3);

  memcpy((void*)esp, task_trampoline, task_trampoline_size);

  /* set application's stack pointer and instruction pointer */
  app_eip = HERE_TO_APP(esp, tramp_page);

  /* put function parameters for task_trampoline to stack */
  *--((l4_umword_t*)esp) = HERE_TO_APP(mbi, tramp_page);
  *--((l4_umword_t*)esp) = env->entry_2nd;
  *--((l4_umword_t*)esp) = 0; /* fake return address */

  /* adapt application's stack pointer */
  app_esp = HERE_TO_APP(esp, tramp_page);

  /* request task creating at task server */
  if ((error = l4_ts_create(ts_id, (l4_ts_taskid_t*)&app->tid,
			    (l4_umword_t)app_eip, (l4_umword_t)app_esp,
			    APP_MCP, (l4_ts_taskid_t*)&app_pager_id,
			    app->prio, app->fname, 0, &exc)))
    {
      printf("Error %d (%s) creating task %x (exc=%d)\n",
	      error, l4env_errstr(error), app->tid.id.task, exc._type);
      if (l4_ts_free(ts_id, (l4_ts_taskid_t*)&app->tid, &exc)
	  || exc._type != exc_l4_no_exception)
	app_msg(app, "Error freeing task number at task server");
      return -L4_ENOMEM;
    }

  app_msg(app, "Started");
  
  return 0;
}
/*@}*/

/** @name Task actions */
/*@{*/
/** Walk to all servers requesting them to close all resources of 
 * an application */
static int
app_cleanup(app_t *app)
{
  int error;
  sm_exc_t exc;
  app_area_t *aa;
  l4env_infopage_t *env = app->env;
  l4exec_section_t *l4exc;

  if (!l4_is_invalid_id(app->tid))
    {
      /* kill L4 task at task server */
      if ((error = l4_ts_delete(ts_id, (l4_ts_taskid_t*)&app->tid, &exc))
	  || exc._type != exc_l4_no_exception)
	{
	  app_msg(app, "Error %d (%s) deleting task at task server", 
			error, l4env_errstr(error));
	  return error;
	}

      /* free task number from task */
      if ((error = l4_ts_free(ts_id, (l4_ts_taskid_t*)&app->tid, &exc))
	  || exc._type != exc_l4_no_exception)
        {
	  app_msg(app, "Error %d (%s) freeing task number at task server",
		       error, l4env_errstr(error));
	  return error;
        }

      /* unregister all names of task at name server */
      names_unregister_task(app->tid);

#ifdef DEALLOC_CON
      /* close vc at con */
      if (!l4_is_invalid_id(con_id))
	{
	  if ((error = con_if_close_all(con_id,
					(con_threadid_t*)&app->tid, &exc)))
	    app_msg(app, "Error %d (%s) closing console",
		    error, l4env_errstr(error));
	}
#endif
    }

  /* free program sections at exec server */
  if ((error = l4exec_bin_close(exec_id,
				(l4exec_envpage_t_slice*)app->env, &exc)))
    {
      app_msg(app, "Error %d (%s) deleting task at exec server", 
		   error, l4env_errstr(error));
      return error;
    }

  /* go through the app_area list and omit all envpage program sections
   * from killing ds which are already killed by exec layer. Nevertheless,
   * detach the section from our address space. */
  for (l4exc=env->section; l4exc<env->section+env->section_num; l4exc++)
    {
      for (aa=app->app_area; aa<app->app_area+app->app_area_next_free; aa++)
	{
	  if (l4_trunc_page(aa->beg.app) == l4_trunc_page(l4exc->addr))
	    {
    	      /* don't kill again */
	      aa->ds = L4DM_INVALID_DATASPACE;
	      break;
	    }
	}
    }

  /* free all (pager) app_areas */
  for (aa=app->app_area;
       aa<app->app_area+app->app_area_next_free;
       aa++)
    junk_ds(&aa->ds, aa->beg.here);

  /* tell Fiasco that tasks symbols/lines are no longer valid */
  app_unpublish_symbols(app);
  app_unpublish_lines(app);

  /* free all dataspaces created by application */
  if (!l4_is_invalid_id(app->tid))
    {
      if ((error = l4dm_close_all(app_dm_id, app->tid, L4DM_SAME_TASK)))
	{
	  app_msg(app, "Error %d (%s) deleting all ds", 
		       error, l4env_errstr(error));
	  return error;
	}

      /* release all occupied interrupts at RMGR */
      rmgr_free_irq_all(app->tid);

      /* release all occupied memory pages at RMGR */
      rmgr_free_mem_all(app->tid);

      /* release all occupied tasks at RMGR */
      rmgr_free_task_all(app->tid);
    }

  /* mark app descriptor as free */
  app->tid = L4_INVALID_ID;
  app->env = NULL;
  
  return 0;
}

/** Start the application. 
 *
 * \param ct		task descriptor 
 * \param fprov_id	file provider
 * \return		0 on success */
int
app_start(cfg_task_t *ct, l4_threadid_t fprov_id)
{
  int error, open_flags;
  app_t *app;
  sm_exc_t exc;
  l4env_infopage_t *env;

  if (  (error = create_app_desc(&app, ct->task.fname))
      ||(error = app_create_infopage(app))
      ||(error = app_init_infopage(app)))
    {
      app_cleanup(app);
      return error;
    }
  
  env = app->env;
  env->fprov_id = fprov_id;
  app->prio     = ct->prio;
  app->fname    = strdup(ct->task.fname);

  /* translate flags */
  open_flags = 0;
  app->flags = 0;
  if (ct->flags & CFG_F_DIRECT_MAPPED)
    {
      app->flags |= APP_DIRECTMAP;
      open_flags |= L4EXEC_DIRECT_MAP;
    }
  if (ct->flags & CFG_F_REBOOT_ABLE)
    app->flags |= APP_REBOOTABLE;
  if (ct->flags & CFG_F_NO_VGA)
    app->flags |= APP_NOVGA;
  if (ct->flags & CFG_F_NO_SIGMA_NULL)
    app->flags |= APP_NOSIGMA0;
  if (cfg_fiasco_symbols)
    {
      app->flags |= APP_SYMBOLS;
      open_flags |= L4EXEC_LOAD_SYMBOLS;
    }
  if (cfg_fiasco_lines)
    {
      app->flags |= APP_LINES;
      open_flags |= L4EXEC_LOAD_LINES;
    }

  /* open file image and separate sections */
  if ((error = l4exec_bin_open(exec_id, ct->task.fname,
			       (l4exec_envpage_t_slice*)env, open_flags, &exc)))
    {
      app_msg(app, "Error %d (%s) while loading",
		  error, l4env_errstr(error));
      return error;
    }

  if (env->entry_1st == L4_MAX_ADDRESS)
    {
      /* Application is not linked against libloader.s -- Plan B */
      app_msg(app, "Starting old-style application");
      error = app_start_static(ct, app);
    }
  else
    {
      /* libloader.s entry point found -- Plan A */
      app_msg(app, "Starting new-style application");
      error = app_start_libloader(ct, app);
    }

  /* Check for errors */
  if (error)
    {
      if (!app_cleanup(app))
	printf("==> App successfully purged\n");
      else
	printf("==> App not fully purged!\n");
    }

  return error;
}

/** Kill application
 *
 * \param task_id	L4 task id or 0 to kill all tasks
 * \return		0 on success
 * 			-L4_ENOTFOUND if no proper task was not found */
int
app_kill(unsigned long task_id)
{
  int i;

  for (i=0; i<MAX_APP; i++)
    {
      if (   (task_id == 0 && (app_array[i].env != NULL))
  	  || (task_id == app_array[i].tid.id.task))
    	{
	  app_t *app = app_array + i;
	  
  	  app_msg(app, "Killing");
  	  return app_cleanup(app);
    	}
    }
  
  if (task_id)
    {
      printf("Task 0x%02lx not found\n", task_id);
      return -L4_ENOTFOUND;
    }

  return 0;
}

/** Dump application to standard output
 *
 * \param task_id	L4 task id or 0 to dump all tasks
 * \return		0 on success
 * 			-L4_ENOTFOUND if no proper task was not found */
int
app_dump(unsigned long task_id)
{
  int i;

  for (i=0; i<MAX_APP; i++)
    {
      if (   (task_id == 0 && (app_array[i].env != NULL))
	  || (task_id == app_array[i].tid.id.task))
	{
	  dump_l4env_infopage(app_array + i);
	  if (task_id)
	    return 0;
	}
    }

  if (task_id)
    {
      printf("Task 0x%02lx not found\n", task_id);
      return -L4_ENOTFOUND;
    }

  return 0;
}

/** Deliver application info to flexpage
 *
 * \param task_id	L4 task id
 * \retval l4env_ds	dataspace containing l4env infopage information
 * \retval fname	application name
 * \return		0 on success
 * 			-L4_ENOTFOUND if no proper task was not found */
int
app_info(unsigned long task_id, l4dm_dataspace_t *l4env_ds, 
	l4_threadid_t client, char **fname)
{
  int i, error;

  if (task_id == 0)
    {
      unsigned int *a;
      l4_threadid_t tid;
      l4_addr_t l4env_addr;

      /* get list of all available tasks */
      if ((error = create_ds(app_dm_id, L4_PAGESIZE,
			     &l4env_addr, l4env_ds,
			     "l4loader_info ds")))
	{
	  printf("Error %d (%s) creating 4k dataspace for l4env\n",
		 error, l4env_errstr(error));
	  return error;
	}

      a = (unsigned int*)l4env_addr;
      for (i=0; i<MAX_APP; i++)
	{
	  if (   (!l4_is_invalid_id(tid = app_array[i].tid))
	      && (app_array[i].env != 0)
	      && (a - (unsigned int*)l4env_addr < 1023))
	    *a++ = tid.id.task;
	}

      /* terminate list */
      *a++ = 0;

      *fname = "";

      /* detach dataspace */
      l4rm_detach((void *)l4env_addr);

      /* transfer ownership of dataspace to client */
      l4dm_transfer(l4env_ds, client);
      
      return 0;
    }
  else
    {
      /* get info for a specific task */
      for (i=0; i<MAX_APP; i++)
	{
	  if (task_id == app_array[i].tid.id.task)
	    {
	      l4_addr_t l4env_addr;
	      app_t *app = app_array + i;
	      
	      if ((error = create_ds(app_dm_id, L4_PAGESIZE,
				     &l4env_addr, l4env_ds,
				     "l4loader_info ds")))
		{
		  printf("Error %d creating 4k dataspace for l4env\n", error);
		  return error;
		}
	      
	      /* copy application's L4 infopage information */
	      memcpy((void*)l4env_addr, app->env, sizeof(l4env_infopage_t));
	      *fname = (char*)app->fname;
	      
	      /* detach dataspace */
	      l4rm_detach((void *)l4env_addr);
	      
	      /* transfer ownership of dataspace to client */
	      l4dm_transfer(l4env_ds, client);

	      return 0;
	    }
	}
    }

  printf("Task 0x%02lx not found\n", task_id);
  return -L4_ENOTFOUND;
}

/** Init application stuff */
int
app_init(void)
{
  l4_threadid_t id;

  /* task server */
  if (!names_waitfor_name("SIMPLE_TS", &id, 5000))
    {
      printf("SIMPLE_TS not found\n");
      return -L4_ENOTFOUND;
    }
  ts_id = id;
  
  /* ELF loader */
  if (!names_waitfor_name("EXEC", &id, 5000))
    {
      printf("EXEC not found\n");
      return -1;
    }
  exec_id = id;

  /* CON server */
  if (!names_waitfor_name("con", &id, 3000))
    /* ignore errors */
    printf("Warning: CON not found\n");

  con_id = id;

  return 0;
}
/*@}*/
