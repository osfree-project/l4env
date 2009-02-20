/* $Id$ */
/**
 * \file	loader/server/src/app.c
 * \brief	start, stop, kill, etc. applications
 *
 * \date	06/10/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003-2007 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#include "app.h"

#include <stdio.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>

#include <l4/sys/consts.h>
#include <l4/env/errno.h>
#include <l4/sys/kdebug.h>
#include <l4/sys/syscalls.h>
#include <l4/sys/cache.h>
#include <l4/l4rm/l4rm.h>
#include <l4/log/l4log.h>
#include <l4/env/mb_info.h>
#include <l4/util/util.h>
#include <l4/util/stack.h>
#include <l4/util/macros.h>
#include <l4/util/memdesc.h>
#include <l4/dm_mem/dm_mem.h>
#include <l4/dm_phys/dm_phys.h>
#include <l4/generic_ts/generic_ts.h>
#include <l4/loader/loader.h>
#include <l4/util/elf.h>
#include <l4/ipcmon/ipcmon.h>
#ifdef USE_TASKLIB
#include <l4/task/task_server.h>
#include "events.h"
#endif

#include "elf-loader.h"
#include "fprov-if.h"
#include "dm-if.h"
#include "trampoline.h"
#include "pager.h"
#ifdef USE_INTEGRITY
#include "integrity.h"
#endif
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
/** position of L4 environment infopage in target app's address space. */
#define APP_ADDR_ENVPAGE	0x00007000
/** position of initial stack in target application's address space. */
#define APP_ADDR_STACK		0x00009000
/** position of libloader.s.so in target application's address space. */
#define APP_ADDR_LIBLOADER	0x00010000
/** position of libld-l4.s.so in target application's address space. */
#define APP_ADDR_LDSO		0x00010000
/*@}*/

/** must be set to PAGESIZE since startup code in l4env/lib/src/startup.c
 * depends on it */
#define APP_TRAMP_SIZE		L4_PAGESIZE

static int app_next_free = 0;			/**< number of next free app_t
						  in app_array. */
static app_t app_array[MAX_APP];
extern int use_events;				/**< use events server. */

/** Return the address of the task struct.
 *
 * \param tid		L4 thread ID
 * \return		task descriptor
 *			NULL if not found */
app_t*
task_to_app(l4_threadid_t tid)
{
  int i;
  for (i=0; i<MAX_APP; i++)
    if (l4_task_equal(app_array[i].tid, tid))
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
  va_list list;

  *ftid = '\0';
  if (!l4_is_invalid_id(app->tid))
    sprintf(ftid, ",#%x", app->tid.id.task);

  va_start(list, format);
  if (!l4_is_invalid_id(app->tid) && app->hi_first_msg)
    printf("\033[36m");
  printf("%*.*s%s: ", 0, 20, app->name, ftid);
  vprintf(format, list);
  if (!l4_is_invalid_id(app->tid) && app->hi_first_msg)
    {
      printf("\033[m");
      app->hi_first_msg = 0;
    }
  putchar('\n');
  va_end(list);
}

/** Debug function to show sections of the L4 environment infopage. */
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

      printf("  %s ds %3d: %08lx-%08lx ",
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
 *			-L4_ENOMEM if no descriptors available */
int
create_app_desc(app_t **new_app)
{
  int counter;

  for (counter=MAX_APP; counter; app_next_free=0)
    {
      while ((app_next_free<MAX_APP) && counter--)
	{
	  if (app_array[app_next_free].env == NULL)
	    {
	      app_t *a = app_array + app_next_free++;

	      memset(a, 0, sizeof(*a));
	      a->env          = (l4env_infopage_t*)1; /* mark as allocated */
	      a->tid          = L4_INVALID_ID;	/* tid still unknown */
	      a->taskno       = 0;		/* tid not configured */
	      a->hi_first_msg = is_fiasco();	/* highlight first message */
	      a->last_pf      = ~0U;
	      a->last_pf_eip  = ~0U;
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
/** Create a new area application descriptor.
 *
 * \param app		application descriptor
 * \retval new_app_area	new application area descriptor
 * \return		0 on success
 *			-L4_ENOMEM on failure */
static app_area_t*
create_app_area_desc(app_t *app)
{
  app_area_t *aa;

  if (app->app_area_next_free >= MAX_APP_AREA)
    {
      printf("Can't handle more than %d sections per application\n",
	      MAX_APP_AREA);
      return 0;
    }

  aa = app->app_area + app->app_area_next_free++;
  memset(aa, 0, sizeof(*aa));
  aa->ds = L4DM_INVALID_DATASPACE;
  return aa;
}

static void
list_app_areas(app_t *app)
{
  app_area_t *aa;
  int i;

  for (i=0; i<app->app_area_next_free; i++)
    {
      aa = app->app_area + i;
      if (aa->flags & APP_AREA_VALID)
	app_msg(app, "  %08lx-%08lx => %08lx-%08lx",
		aa->beg.app,  aa->beg.app+aa->size,
		aa->beg.here, aa->beg.here+aa->size);
    }
}

/** Check if new app_area overlaps with exisiting areas. */
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
	  app_msg(app, "app_area (%08lx-%08lx) overlaps:",
	         (*check_aa)->beg.app, (*check_aa)->beg.app+(*check_aa)->size);
	  list_app_areas(app);
	  return -L4_EINVAL;
	}
    }

  (*check_aa)->flags |= APP_AREA_VALID;
  return 0;
}

/** Find a free virtual address range. This is only necessary for
 * sigma0 applications because in l4env-style applications, we let
 * the region manager manager decide where a region lives. */
static l4_addr_t
app_find_free_virtual_area(app_t *app, l4_size_t size,
			   l4_addr_t low, l4_addr_t high)
{
  int i;
  app_area_t *aa;
  l4_addr_t addr;

  addr = 0x00100000;	/* begin at 1MB */
  if (low > addr)
    addr = low;

  /* check for free area */
  for (i=0; i<app->app_area_next_free; i++)
    {
      aa = app->app_area + i;
      if ((aa->flags & APP_AREA_VALID) &&
	   addr < aa->beg.app+aa->size && addr+size >= aa->beg.app)
	{
	  /* we have a clash. Set addr to the end of this area. */
	  addr = aa->beg.app + aa->size;

	  /* restart because list of addresses is not sorted */
	  i=0;
	}
    }

  /* we went through. Our address may be after the last area. */
  if (addr > l4util_memdesc_vm_high() || addr >= high)
    return 0;

  return addr;
}

/** Dump all app_areas for debugging purposes. */
void
app_list_addr (app_t *app)
{
  int i;
  app_area_t *aa;

  app_msg(app, "Dumping app addresses");

  /* Go through all app_areas we page for the application */
  for (i=0; i<app->app_area_next_free; i++)
    {
      aa = app->app_area + i;
      printf("  %08lx-%08lx => %08lx-%08lx (%s)\n",
	  aa->beg.app,  aa->beg.app+aa->size,
	  aa->beg.here, aa->beg.here+aa->size, aa->dbg_name);
    }
}

/** Attach dataspace to pager. Therefore it is accessible by the application.
 *
 * \param app		application
 * \param ds		dataspace to attach
 * \param addr		start address of dataspace inside application.
 *			If L4_MAX_ADDRESS, then do not make the dataspace
 *			accessible from the application but only register
 *			to allow smoothly shutdown of dataspace on exit
 *			of the application.
 * \param size		size of dataspace
 * \param type		type
 * \param rights	rights for the pager
 * \param dbg_name	name of application area for debugging purposes
 * \retval aa		application area */
int
app_attach_ds_to_pager(app_t *app, l4dm_dataspace_t *ds, l4_addr_t addr,
		       l4_size_t size, l4_uint16_t type, l4_uint32_t rights,
		       const char *dbg_name, app_area_t **aa)
{
  int error;
  l4_addr_t sec_end;
  app_area_t *new_aa;

  if (!(new_aa = create_app_area_desc(app)))
    return -L4_ENOMEM;

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
    }

  *aa = new_aa;

  return 0;
}

/** Attach a section which should be paged by the loader pager.
 *
 * \param l4exc_start	first l4exec_section
 * \param env		L4 environment infopage
 * \param app		application
 * \return		0 on success
 *			-L4_ENOMEM if no app descriptor is available */
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
      if ((error = app_attach_ds_to_pager(app, &l4exc->ds, l4exc->addr,
					  l4exc->size, l4exc->info.type,
					  rights, "program section", &aa)))
	return error;

      if (app->flags & APP_NOSUPER)
	aa->flags |= APP_AREA_NOSUP;

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
 *			-L4_ENOMEM if no app descriptor is available */
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

/** Create dataspace of anonymous memory mapped into the new application.
 * The dataspace is also mapped into this application.
 *
 * \param app		application descriptor
 * \param app_addr	virtual address of dataspace in application
 *			may be L4_MAX_ADDRESS of area is not pageable to
 *			application (status information an so on)
 * \param size		size of dataspace
 * \retval ret_aa	filled out app_area
 * \param dbg_name	name of dataspace for debugging purposes
 * \return		0 on success */
static int
app_create_ds(app_t *app, l4_addr_t app_addr, l4_size_t size,
              app_area_t **ret_aa, const char *dbg_name)
{
  int error;
  l4_addr_t here;
  l4dm_dataspace_t ds;

  if ((error = create_ds(app_dsm_id, size, &here, &ds, dbg_name)))
    return error;

  /* make dataspace pageable */
  if ((error = app_attach_ds_to_pager(app, &ds, app_addr, size,
				      L4_DSTYPE_READ | L4_DSTYPE_WRITE,
				      L4DM_RW, dbg_name, ret_aa)))
    return error;

  (*ret_aa)->beg.here = here;

  return 0;
}

/** Share all sections which are not exclusiv owned by the applications
 * to the application. Modules have the L4_DSTYPE_APP_IS_OWNER bit set
 * which means that their ownership is already transfered to the application */
void
app_share_sections_with_client(app_t *app, l4_threadid_t client)
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
	  l4dm_share(&l4exc->ds, client, rights);
	}
    }
}


/*@}*/

/** Make symbolic information known by Fiasco. */
static inline void
app_publish_symbols(app_t *app)
{
#if defined(ARCH_x86) || defined(ARCH_amd64)
  fiasco_register_symbols (app->tid, app->symbols, app->sz_symbols);
#endif
}

/** Make debug line number information known by Fiasco. */
static inline void
app_publish_lines(app_t *app)
{
#if defined(ARCH_x86) || defined(ARCH_amd64)
  fiasco_register_lines (app->tid, app->lines, app->sz_lines);
#endif
}

static void
app_publish_lines_symbols(app_t *app)
{
  /* XXX-Todo: this function and the two above are just for sigma0-style
   * applications, lines/symbol loading does not work for sigma0 apps right
   * now as we would have to redo the work previously done in the exec
   * server, something similar exists in the ldso lib
   */
  if (app->flags & APP_SYMBOLS)
    {
      if (app->symbols)
        {
          app_publish_symbols(app);
          l4dm_transfer(&app->ds_symbols, app->tid);
        }
    }

  if (app->flags & APP_LINES)
    {
      if (app->lines)
        {
          app_publish_lines(app);
          l4dm_transfer(&app->ds_lines, app->tid);
        }
    }
}

/** Retire symbolic debug information of an application from Fiasco. */
static inline void
app_unpublish_symbols(app_t *app)
{
#if defined(ARCH_x86) || defined(ARCH_amd64)
  if (app->symbols && !l4_is_invalid_id(app->tid))
    fiasco_register_symbols(app->tid, 0, 0);
#endif
}

/** Retire symbolic debug information of an application from Fiasco. */
static inline void
app_unpublish_lines(app_t *app)
{
#if defined(ARCH_x86) || defined(ARCH_amd64)
  if (app->lines && !l4_is_invalid_id(app->tid))
    fiasco_register_lines(app->tid, 0, 0);
#endif
}

/*@} */

/** Create the application's infopage.
 *
 * \param app		application descriptor
 * \return		0 on success */
static int
app_create_infopage(app_t *app)
{
  int error;
  app_area_t *aa;

  /* create infopage */
  if ((error = app_create_ds(app, APP_ADDR_ENVPAGE, L4_PAGESIZE,
			     &aa, "infopage")))
    return error;

  app->env = (l4env_infopage_t*)aa->beg.here;
  return 0;
}

/** Create I/O permission bitmap.
 *
 * \param app		application descriptor
 * \param ct		config task descriptor
 * \return		0 on success */
static int
app_create_iobitmap(app_t *app, cfg_task_t *ct)
{
  if (ct->iobitmap)
    {
      int error;
      app_area_t *aa;

      if (!(aa = create_app_area_desc(app)))
	return -L4_ENOMEM;

      aa->dbg_name = "iobitmap";

      if ((error = create_ds(app_dsm_id, 8192, &aa->beg.here,
			     &aa->ds, aa->dbg_name)))
	{
	  app_msg(app, "Error %d creating I/O permission bitmap", error);
	  return error;
	}

      app->iobitmap = (char*)aa->beg.here;
      memcpy(app->iobitmap, ct->iobitmap, 8192);
    }

  return 0;
}

/** Initialize the application's environment infopage.
 *
 * \param env		pointer to L4env infopage
 * \return		0 on success */
int
init_infopage(l4env_infopage_t *env)
{
  /* zero out */
  memset(env, 0, sizeof(l4env_infopage_t));

  /* fill out infopage */
  env->stack_size  = APP_TRAMP_SIZE;
  env->stack_dm_id = app_stack_dsm;
  env->loader_id   = l4_myself();
  env->image_dm_id = app_image_dsm;
  env->text_dm_id  = app_text_dsm;
  env->data_dm_id  = app_data_dsm;
#ifdef USE_TASKLIB
  env->parent_id   = l4task_get_server();
#endif

  env->ver_info.arch_class = ARCH_ELF_ARCH_CLASS;
  env->ver_info.arch_data  = ARCH_ELF_ARCH_DATA;
  env->ver_info.arch       = ARCH_ELF_ARCH;

  /* set library addresses */
  env->addr_libloader = APP_ADDR_LIBLOADER;

  /* set default path for dynamic libraries */
  strcpy(env->binpath, cfg_binpath);
  strcpy(env->libpath, cfg_libpath);

  env->vm_low  = 0x1000;
  env->vm_high = l4util_memdesc_vm_high();

  return 0;
}

/** allocate space from a "page" _downwards_. */
static l4_addr_t
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
 * \param fprov_id	file provider
 * \param tramp_page	address of trampoline page in our / application's
 *			address space
 * \param tramp_page_addr address in our address space
 * \param mbi		pointer to mbi
 * \return		0 on success
 *			-L4_ENOMEM if not enough space in page */
static int
load_modules(cfg_task_t *ct, app_t *app, l4_threadid_t fprov_id,
	     app_addr_t *tramp_page, l4_addr_t *tramp_page_addr,
	     l4util_mb_info_t *mbi)
{
  if (ct->next_module > ct->module)
    {
      int mods_count = ct->next_module-ct->module;
      l4util_mb_mod_t *mod;
      cfg_module_t *ct_mod = ct->module;

      if (!(mod = (l4util_mb_mod_t*)
	    alloc_from_tramp_page(mods_count*sizeof(l4util_mb_mod_t),
				  tramp_page, tramp_page_addr)))
	{
	  app_msg(app, "No space left for multiboot modules in "
		       "trampoline page");
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
          l4_addr_t map_addr;
	  int error;

	  app_msg(app, "Loading module \"%s\"", ct_mod->fname);

	  /* Request module file image from file provider. If application
	   * should be loaded direct mapped, load the image contiguous.
	   * Therefore the physical address of the first byte is equal to
	   * the physical address of the whole dataspace.
	   *
	   * We don't attach the module to our address space since we do
	   * nothing with app modules, unless we need have to hash them to do
           * the integrity measurements. Later, these dataspaces get paged
           * by the application's region manager. */
	  if ((error = load_file(ct_mod->fname, fprov_id,
				 app->env->memserv_id,
				 cfg_modpath, /*contiguous=*/
				 app->flags & APP_DIRECTMAP ? 1 : 0,
				 ct->flags & CFG_F_HASH_MODULES ? &map_addr : 0,
				 &file_size, &ds)))
	    {
	      app_msg(app, "Error %d (%s) loading module \"%s\"",
			   error, l4env_errstr(error), ct_mod->fname);
	      return error;
	    }

          if (ct->flags & CFG_F_HASH_MODULES)
            {
#ifdef USE_INTEGRITY
              integrity_hash_data(app, ct_mod->fname, (void *)map_addr, file_size);
#endif
              l4rm_detach((void *)map_addr);
            }

	  /* XXX too restrictive? */
	  mod_size = l4_round_page(file_size);

	  if (ct_mod->low != 0 || ct_mod->high != L4_MAX_ADDRESS)
	    {
	      /* User-specified constraints overwrite everything. Module is
	       * not necessary mapped one-by-one. */
	      l4_addr_t addr;

	      if (!(addr = app_find_free_virtual_area(app, mod_size,
						ct_mod->low, ct_mod->high)))
		{
		  app_msg(app, "Couldn't find a free vm area of size %08zx"
			       " in [%08lx-%08lx] for module \"%s\"",
			       mod_size, ct_mod->low, ct_mod->high,
			       ct_mod->fname);
		  app_list_addr(app);
		  return -L4_EINVAL;
		}

	      mod->mod_start = addr;
	      mod->mod_end   = addr + mod_size;
	    }
	  else if (app->flags & APP_DIRECTMAP)
	    {
	      /* Application is direct mapped so address of boot module should
	       * be direct mapped too. Obtain the start-address of the module
	       * from the dataspace manager. */
	      if ((error = phys_ds(&ds, file_size,
				   (l4_addr_t*)&mod->mod_start))<0)
		return error;

	      mod->mod_end = mod->mod_start + mod_size;
	    }
	  else if (app->flags & APP_MODE_SIGMA0)
	    {
	      /* place module somewhere in the virtual address space of
	       * the application */
	      l4_addr_t addr;

	      if (!(addr = app_find_free_virtual_area(app, mod_size,
						      0, L4_MAX_ADDRESS)))
		{
		  app_msg(app, "Couldn't find a free vm area of size %08zx"
			       " for module \"%s\"", mod_size, ct_mod->fname);
		  app_list_addr(app);
		  return -L4_EINVAL;
		}

	      mod->mod_start = addr;
	      mod->mod_end   = addr + mod_size;
	    }

	  app_msg(app, "Mapped module to %08x-%08x",
	      mod->mod_start, mod->mod_end);

	  if (app->flags & APP_MODE_SIGMA0)
	    {
	      /* sigma0-style: make module accessible from application
	       * -- paged by application pager.
	       * XXX We could page modules by the applications region manager
	       * but we don't pass the L4env infopage to the application so
	       * the application doesn't know the datatspace id of the
	       * module. */
	      if ((error =
		    app_attach_ds_to_pager(app, &ds, mod->mod_start, mod_size,
					   L4_DSTYPE_READ | L4_DSTYPE_WRITE,
					   L4DM_RW, "boot module", &aa)))
		return error;
	    }
	  else
	    {
	      /* l4env-style: make module accessible from application
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
	      int args_len = strlen(ct_mod->args) + 1;
	      if (!(args = (char*)alloc_from_tramp_page(args_len,
							tramp_page,
						        tramp_page_addr)))
		{
		  app_msg(app, "No space left for cmdline of module "
			       "\"%s\"\n in trampoline page",
			       ct_mod->args);
		  return -L4_ENOMEM;
		}

	      memcpy(args, ct_mod->args, args_len);
	      mod->cmdline = HERE_TO_APP(args, *tramp_page);
	    }
	  else
	    {
	      /* mod->cmdline = "" */
	      char *args;
	      if (!(args = (char*)alloc_from_tramp_page(1,
							tramp_page,
							tramp_page_addr)))
		{
		  app_msg(app, "No space left for cmdline \"\" of module "
			       "\"%s\"\n in trampoline page",
			       ct_mod->args);
		  return -L4_ENOMEM;
		}

	      *args = '\0';
	      mod->cmdline = HERE_TO_APP(args, *tramp_page);
	    }
	}
      mbi->flags |= L4UTIL_MB_MODS;
    }

  return 0;
}

static void
load_vbe_info(app_t *app,
	      app_addr_t *tramp_page, l4_addr_t *tramp_page_addr,
	      l4util_mb_info_t *mbi)
{
  /* copy mb_info->vbe_mode and mb_info->vbe_ctrl */
  if (l4env_multiboot_info->flags & L4UTIL_MB_VIDEO_INFO)
    {
      l4_addr_t tmp_tramp_page_addr = *tramp_page_addr;
      l4util_mb_vbe_mode_t *vbe_mode;
      l4util_mb_vbe_ctrl_t *vbe_ctrl;

      if (l4env_multiboot_info->vbe_mode_info)
	{
	  if (   (vbe_mode = (l4util_mb_vbe_mode_t*)
		    alloc_from_tramp_page(sizeof(l4util_mb_vbe_mode_t),
					  tramp_page, &tmp_tramp_page_addr))
	      && (vbe_ctrl = (l4util_mb_vbe_ctrl_t*)
		    alloc_from_tramp_page(sizeof(l4util_mb_vbe_ctrl_t),
					  tramp_page, &tmp_tramp_page_addr))
	      )
	    {
              /* XXX ensure this structure is located below 4GB */
	      *vbe_mode = *((l4util_mb_vbe_mode_t*)
			     (l4_addr_t)l4env_multiboot_info->vbe_mode_info);
              /* XXX ensure this structure is located below 4GB */
	      *vbe_ctrl = *((l4util_mb_vbe_ctrl_t*)
			     (l4_addr_t)l4env_multiboot_info->vbe_ctrl_info);
	      mbi->vbe_mode_info = HERE_TO_APP(vbe_mode, *tramp_page);
	      mbi->vbe_ctrl_info = HERE_TO_APP(vbe_ctrl, *tramp_page);
	      mbi->flags        |= L4UTIL_MB_VIDEO_INFO;
	      *tramp_page_addr   = tmp_tramp_page_addr;
	    }
	  else
	    {
	      app_msg(app, "Can't pass VBE video info to trampoline page \
			   (needs %zd bytes)",
			   sizeof(l4util_mb_vbe_ctrl_t)+
			   sizeof(l4util_mb_vbe_mode_t));
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
 * \param fprov_id	file provider
 * \param multiboot_info pointer to mbi
 * \return		0 on success
 *			-L4_ENOMEM if not enough space in page */
static int
app_create_mb_info(cfg_task_t *ct, app_t *app, app_addr_t *tramp_page,
		   l4_addr_t *tramp_page_addr, l4_threadid_t fprov_id,
		   l4util_mb_info_t **multiboot_info)
{
  char *args;
  int error, args_len;
  l4util_mb_info_t *mbi;

  /* allocate space for multiboot info */
  if (!(mbi = (l4util_mb_info_t*)
		    alloc_from_tramp_page(sizeof(l4util_mb_info_t),
					  tramp_page, tramp_page_addr)))
    {
      app_msg(app, "No space left for multiboot_info in trampoline page");
      return -L4_ENOMEM;
    }

  memset(mbi, 0, sizeof(l4util_mb_info_t));

  /* copy multiboot_info */
  if (l4env_multiboot_info->flags & L4UTIL_MB_MEMORY)
    {
      mbi->flags    |= L4UTIL_MB_MEMORY;
      mbi->mem_lower = l4env_multiboot_info->mem_lower;
      mbi->mem_upper = l4env_multiboot_info->mem_upper;
    }
  if (l4env_multiboot_info->flags & L4UTIL_MB_BOOTDEV)
    {
      mbi->flags      |= L4UTIL_MB_BOOTDEV;
      mbi->boot_device = l4env_multiboot_info->boot_device;
    }

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

  mbi->flags  |= L4UTIL_MB_CMDLINE;
  mbi->cmdline = HERE_TO_APP(args, *tramp_page);
  strcpy(args, ct->task.fname);

  if (ct->task.args)
    {
      strcat(args, " ");
      strcat(args, ct->task.args);
    }

  /* load the VESA information about the current video mode as set by GRUB */
  load_vbe_info(app, tramp_page, tramp_page_addr, mbi);

  /* load applications modules */
  if ((error = load_modules(ct, app, fprov_id,
			    tramp_page, tramp_page_addr, mbi)) < 0)
    return error;

  *multiboot_info = mbi;

  return 0;
}

/** Create physical memory for an application. */
static int
app_create_phys_memory(app_t *app, l4_size_t size, int cfg_flags, int pool)
{
  int error;
  l4_addr_t addr;
  l4_uint32_t flags;
  app_area_t *aa;
  l4_size_t psize;
  l4dm_dataspace_t ds;
  char ds_name[L4DM_DS_NAME_MAX_LEN];
  char ds_flags[20];

  if (!(app->flags & APP_MODE_SIGMA0))
    {
      app_msg(app, "Can't create physical memory for l4env-style application");
      return -L4_EINVAL;
    }

  flags = 0;

  if (!pool)
    {
      if (cfg_flags & CFG_M_DMA_ABLE)
	pool = L4DM_MEMPHYS_ISA_DMA;
      else
	pool = L4DM_MEMPHYS_DEFAULT;
    }

  sprintf(ds_flags, "pool=%d", pool);

  if (cfg_flags & (CFG_M_CONTIGUOUS | CFG_M_DIRECT_MAPPED | CFG_M_DMA_ABLE))
    {
      /* request contiguous dataspace */
      flags |= L4DM_CONTIGUOUS;
      strcat(ds_flags, ",cont");
    }

  if ((size % L4_SUPERPAGESIZE) == 0 && (cfg_flags & CFG_M_NOSUPERPAGES) == 0)
    {
      /* request 4MB pages */
      flags |= L4DM_MEMPHYS_SUPERPAGES;
      strcat(ds_flags, ",4MB");
    }

  snprintf(ds_name, sizeof(ds_name), "phys mem %s", app->name);
  if ((error = l4dm_memphys_open(pool, L4DM_MEMPHYS_ANY_ADDR,
				 size, 0, flags, ds_name, &ds)))
    {
      app_msg(app, "Error %d reserving %08zx of physical memory (%s)",
		   error, size, ds_flags);
      return error;
    }

  if ((error = l4dm_mem_ds_phys_addr(&ds, 0, L4DM_WHOLE_DS, &addr, &psize)))
    {
      app_msg(app, "Error %d determining address of physmem",
		   error);
      return error;
    }

  if (psize != size)
    {
      app_msg(app, "only %08zx/%08zx bytes of physmem contiguous",
		   psize, size);
      return -L4_ENOMEM;
    }

  /* make physical memory accessible from application */
  if ((error = app_attach_ds_to_pager(app, &ds, addr, size,
				      L4_DSTYPE_READ | L4_DSTYPE_WRITE,
				      L4DM_RW, "phys memory", &aa)))
    return error;

  if (cfg_flags & CFG_M_NOSUPERPAGES)
    aa->flags |= APP_AREA_NOSUP;

  app_msg(app, "Reserved %08zx memory at %08lx-%08lx",
	      size, aa->beg.app, aa->beg.app+aa->size);

  return 0;
}
/*@}*/

/** @name Start application */
/*@{*/

/** Allocate the task number.
 *
 * \param app		application descriptor
 * \return		0 on success */
static int
app_create_tid(app_t *app)
{
  int error;

  /* ask task server to create new task */
  if ((error = l4ts_allocate_task(app->taskno, &app->tid)))
    {
      app_msg(app, "Error %d (%s) allocating task",
		   error, l4env_errstr(error));
      return error;
    }

  return 0;
}

/** Start application the old way.
 *
 * Create stack page, create a valid multiboot info structure and
 * copy the trampoline code. Then start the new task
 *
 * \param ct		config task descriptor
 * \param app		application descriptor
 * \return		0 on success */
static int
app_start_static(cfg_task_t *ct, app_t *app)
{
  int error, i;
  l4_size_t task_trampoline_size;
  l4_addr_t tramp_page_addr, esp;
  app_addr_t tramp_page;
  app_area_t *tramp_page_aa;
  l4env_infopage_t *env = app->env;
  l4util_mb_info_t *mbi;
  cfg_mem_t *ct_mem;
  char dbg_name[32];

  /* attach relocated sections (usual the application itself) */
  if ((error = app_attach_sections_to_pager(app)))
    {
      app_msg(app, "Error %d (%s) attaching sections",
		   error, l4env_errstr(error));
      return error;
    }

  /* check if some section has to be relocated -- gone with l4exec removal */
  for (i=0; i<env->section_num; i++)
    if (env->section[i].info.type & L4_DSTYPE_RELOCME)
      enter_kdebug("relink static not supported");

  app_publish_lines_symbols(app);

  /* create trampoline page */
  snprintf(dbg_name, sizeof(dbg_name), "tramp %s", app->name);
  if ((error = app_create_ds(app, APP_ADDR_STACK, env->stack_size,
			     &tramp_page_aa, dbg_name)))
    return error;

  tramp_page      = tramp_page_aa->beg;
  env->stack_low  = tramp_page.app;
  env->stack_high = tramp_page.app  + env->stack_size;
  tramp_page_addr = tramp_page.here + env->stack_size;

  /* reserve physical memory regions */
  for (ct_mem=ct->mem; ct_mem<ct->next_mem; ct_mem++)
    {
      if ((error = app_create_phys_memory(app, ct_mem->size,
					  ct_mem->flags, ct_mem->pool)))
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
	     "(have %ld bytes, need %d bytes)",
	     tramp_page_addr - tramp_page.here, 0x400);
      return -L4_ENOMEM;
    }
  esp += 0x400;

  /* copy task_trampoline PIC code on top of stack */
  task_trampoline_size = ((l4_addr_t)&_task_trampoline_end
                        - (l4_addr_t)task_trampoline);

  /* align stack pointer to dword */
  esp -= ((task_trampoline_size + 3) & ~3);

  memcpy((void*)esp, task_trampoline, task_trampoline_size);

  /* set application's stack pointer and instruction pointer */
  app->eip = HERE_TO_APP(esp, tramp_page);

  /* put function parameters for task_trampoline to stack */
  if (ct->flags & CFG_F_L4ENV_BINARY)
    l4util_stack_push_mword(&esp, ~L4UTIL_MB_VALID);
  else
    l4util_stack_push_mword(&esp, L4UTIL_MB_VALID);
  l4util_stack_push_mword(&esp, APP_ADDR_ENVPAGE); /* env dummy for task_trampoline() */
  l4util_stack_push_mword(&esp, HERE_TO_APP(mbi, tramp_page));
  l4util_stack_push_mword(&esp, env->entry_2nd);
  l4util_stack_push_mword(&esp, 0); /* fake return address */

  /* adapt application's stack pointer */
  app->esp = HERE_TO_APP(esp, tramp_page);

  if (ct->flags & CFG_F_L4ENV_BINARY)
    app_share_sections_with_client(app, app->tid); /* thread x.0 */

  return 0;
}

static void app_setup_caps(app_t *app)
{
  if (l4_is_invalid_id(app->caphandler))
    return;

  app_msg(app, "Setting up capabilities");

  cfg_cap_t *c = app->caplist;
  while (c)
    {
      l4_taskid_t dest;
      cfg_cap_t *h = c;

      if (!cfg_lookup_name(c->dest, &dest))
	app_msg(app, "no task with name '%s' for caps", c->dest);
      else
        {
	  if (c->type == CAP_TYPE_ALLOW)
	    l4ipcmon_allow(app->caphandler, app->tid, dest);
	  else
	    l4ipcmon_deny(app->caphandler, app->tid, dest);
	}

      c = c->next;
      free(h->dest); // allocated with strdup
      free(h);
    }
}

static l4_quota_desc_t app_setup_kquota(app_t *app)
{
  l4_quota_desc_t kquota = L4_INVALID_KQUOTA;
  int e;

  if (!app->kquota)
    return L4_INVALID_KQUOTA;

  kquota.q.amount = app->kquota->size;

  if (!app->kquota->users)
    {
      /* We are the first to use this quota, so we need to create a
       * new kernel quota from loader's quota. */
      kquota.q.id = l4_myself().id.task;
      kquota.q.cmd = L4_KQUOTA_CMD_NEW;
    }
  else
    {
      /* This quota is already in use, so we share it. For sharing we
       * use the first task that is in the kquota's user list. */
      kquota.q.id = app->kquota->users->tid.id.task;
      kquota.q.cmd = L4_KQUOTA_CMD_SHARE;
    }

  if ((e = kquota_add_user(app->kquota, app->tid)))
    {
      app_msg(app, "Could not add app to kquota %s.", app->kquota->name);
      return L4_INVALID_KQUOTA;
    }

  return kquota;
}

static int
app_cont_static(app_t *app)
{
  int error;
  l4_quota_desc_t kquota = app_setup_kquota(app);

  app_setup_caps(app);

  app_msg(app, "Entry at %08lx => %08lx", app->eip, app->env->entry_2nd);

  /* request task creating at task server */
  if ((error = l4ts_create_task2(&app->tid, app->eip, app->esp,
                                 app->mcp,
                                 &app_pager_id, &app->caphandler,
                                 kquota, app->prio, app->fname, 0)))
    {
      int ferror;

      app_msg(app, "Error %d (%s) creating task %x",
	      error, l4env_errstr(error), app->tid.id.task);
      if ((ferror = l4ts_free_task(&app->tid)))
	app_msg(app, "Error %d (%s) freeing task %x at task server",
	             ferror, l4env_errstr(ferror), app->tid.id.task);
      return -L4_ENOMEM;
    }

  /* set client as owner so that client is able to kill that task */
  if ((error = l4ts_owner(app->tid, app->owner)))
    {
      app_msg(app, "Error %d (%s) setting ownership",
		   error, l4env_errstr(error));
      return error;
    }

  if (app->flags & APP_SHOW_AREAS)
    {
      app_msg(app, "Areas:");
      list_app_areas(app);
    }

  app_msg(app, "Started");
  return 0;
}

static int
app_start_interp(cfg_task_t *ct, app_t *app)
{
  l4_size_t task_trampoline_size;
  l4_addr_t tramp_page_addr, esp;
  app_addr_t tramp_page;
  app_area_t *tramp_page_aa;
  l4env_infopage_t *env = app->env;
  l4util_mb_info_t *mbi;
  char dbg_name[32];
  int error;

  error = elf_map_binary(app);
  junk_ds(&app->ds_image, app->image);

  if (error)
    return error;

  env->stack_size = 4*L4_PAGESIZE;

  /* create trampoline page */
  snprintf(dbg_name, sizeof(dbg_name), "tramp %s", app->name);
  if ((error = app_create_ds(app, APP_ADDR_STACK, env->stack_size,
			     &tramp_page_aa, dbg_name)))
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
      app_msg(app,
	     "No space left for stack in trampoline page "
	     "(have %ld bytes, need %d bytes)",
	     tramp_page_addr - tramp_page.here, 0x400);
      return -L4_ENOMEM;
    }
  esp += 0x400;

  /* copy task_trampoline PIC code on top of stack */
  task_trampoline_size = ((l4_addr_t)&_task_trampoline_end
                        - (l4_addr_t)task_trampoline);

  /* align stack pointer to dword */
  esp -= ((task_trampoline_size + 3) & ~3);

  memcpy((void*)esp, task_trampoline, task_trampoline_size);

  /* set application's stack pointer and instruction pointer */
  app->eip = HERE_TO_APP(esp, tramp_page);

  /* load the libld-l4.s.so interpreter */
  if ((error = elf_map_ldso(app, APP_ADDR_LDSO)))
    return error;

  /* put function parameters for task_trampoline to stack */
  l4util_stack_push_mword(&esp, L4UTIL_MB_VALID);
  l4util_stack_push_mword(&esp, APP_ADDR_ENVPAGE);
  l4util_stack_push_mword(&esp, HERE_TO_APP(mbi, tramp_page));
  l4util_stack_push_mword(&esp, env->entry_1st);
  l4util_stack_push_mword(&esp, 0); /* fake return address */

  env->interp = APP_ADDR_LDSO;

  /* adapt application's stack pointer */
  app->esp = HERE_TO_APP(esp, tramp_page);

  /* Share all program sections not yet transfered to the application
   * (all sections but modules). This is necessary because otherwise
   * the applications region mapper is not allowed to forward pagefaults
   * to the appropriate dataspace managers. */
  app_share_sections_with_client(app, app->tid);

  return 0;
}

static int
app_cont_interp(app_t *app)
{
  int error;
  l4_quota_desc_t kquota = app_setup_kquota(app);

  app_setup_caps(app);

  app_msg(app, "Starting %s at %08lx via %08lx",
	       interp, app->env->entry_1st, app->eip);

  if ((error = l4ts_create_task2(&app->tid, app->eip, app->esp,
                                 app->mcp,
                                 &app_pager_id, &app->caphandler,
                                 kquota, app->prio, app->fname, 0)))
    {
      int ferror;

      app_msg(app, "Error %d (%s) creating task %x at task server",
	      error, l4env_errstr(error), app->tid.id.task);
      if ((ferror = l4ts_free_task(&app->tid)))
	app_msg(app, "Error %d (%s) freeing task %x at task server",
	        ferror, l4env_errstr(ferror), app->tid.id.task);

      return error;
    }

  /* set client as owner so that client is able to kill that task */
  if ((error = l4ts_owner(app->tid, app->owner)))
    {
      app_msg(app, "Error %d (%s) setting ownership",
		   error, l4env_errstr(error));
      return error;
    }

  return 0;
}

/*@}*/

/** @name Task actions */
/*@{*/
/** Cleanup application (external resources).
 *
 */
static int
app_cleanup_extern(app_t *app)
{
  int error;
  extern l4_threadid_t killing;

  if (!l4_is_invalid_id(app->tid))
    {
      killing = app->tid;
      if ((error = l4ts_kill_task(app->tid, 0)))
        app_msg(app, "Error %d (%s) killing task (ignored)",
		    error, l4env_errstr(error));
      else
      {
        #ifdef USE_TASKLIB
         if (events_send_kill(app->tid))
           app_msg(app, "Error killing task "l4util_idfmt,
	    	   l4util_idstr(app->tid));
        #endif
      } 
      killing = L4_INVALID_ID;

      return error;
    }

  return -L4_EINVAL;
}

/** Cleanup application (internal resources).
 *
 * Do only cleanup for loader and exec server.
 */
static int
app_cleanup_intern(app_t *app)
{
  app_area_t *aa;
  l4env_infopage_t *env = app->env;
  l4exec_section_t *l4exc;

  /* go through the app_area list and omit all envpage program sections
   * from killing ds which are already killed by exec layer. Nevertheless,
   * detach the section from our address space. */
  for (l4exc=env->section; l4exc<env->section+env->section_num; l4exc++)
    {
      for (aa=app->app_area; aa<app->app_area+app->app_area_next_free; aa++)
	{
	  if (l4_trunc_page(aa->beg.app) == l4_trunc_page(l4exc->addr) &&
	      (l4exc->info.type & L4_DSTYPE_EXEC_IS_OWNER))
	    {
	      /* don't kill again */
	      aa->ds = L4DM_INVALID_DATASPACE;
	      break;
	    }
	}
    }

  /* free all (pager) app_areas */
  for (aa=app->app_area; aa<app->app_area+app->app_area_next_free; aa++)
    {
      /* If this is an application area which is only attached to our
       * address space to be able to virtualize something in the targets
       * address space -- don't try to kill the dataspace */
      if (!l4dm_is_invalid_ds(aa->ds))
	junk_ds(&aa->ds, aa->beg.here);
    }

  /* tell Fiasco that tasks symbols/lines are no longer valid */
  app_unpublish_symbols(app);
  app_unpublish_lines(app);

  app->caphandler = L4_INVALID_ID;
  app->caplist    = NULL;
  if (app->kquota) // If we were user of a kquota, remove us from user list.
    kquota_del_user(app->kquota, app->tid);
  app->kquota     = NULL;

  /* mark app descriptor as free */
  app->tid = L4_INVALID_ID;
  free((void*)app->fname);  /* created with strdup */
  app->env = NULL;

  return 0;
}

/** Start the application.
 *
 * \param ct		config task descriptor
 * \param owner		the owner of the task (for transfering the ownership
 *			of the task ID)
 * \retval ret_val	created application descriptor
 * \return		0 on success */
static int
app_init(cfg_task_t *ct, l4_taskid_t owner, app_t **ret_val)
{
  int error;
  l4env_infopage_t *env;
  app_t *app;

  if ((error = create_app_desc(&app)))
    {
      *ret_val = 0;
      return error;
    }

  if (   (error = app_create_infopage(app))
      || (error = init_infopage(app->env))
      || (error = app_create_iobitmap(app, ct)))
    {
      app_cleanup_extern(app); /* kill task, free task resources */
      app_cleanup_intern(app); /* free our task resources */
      return error;
    }

  env             = app->env;
  env->magic      = L4ENV_INFOPAGE_MAGIC;
  env->fprov_id   = ct->fprov_id;
  env->memserv_id = ct->dsm_id;
  app->taskno     = ct->taskno;
  app->prio       = ct->prio;
  app->mcp        = ct->mcp;
  app->fname      = strdup(ct->task.fname);
  app->name       = strrchr(app->fname, '/');
  if (!app->name)
    app->name = app->fname;
  else
    app->name++;

  /* translate flags */
  app->flags = 0;
  if (ct->flags & CFG_F_DIRECT_MAPPED)
    app->flags |= APP_DIRECTMAP;
  app->flags |= ct->flags & CFG_F_ALLOW_VGA      ? APP_ALLOW_VGA   : 0;
  app->flags |= ct->flags & CFG_F_ALLOW_KILL     ? APP_ALLOW_KILL  : 0;
  app->flags |= ct->flags & CFG_F_ALLOW_BIOS     ? APP_ALLOW_BIOS  : 0;
  app->flags |= ct->flags & CFG_F_NO_SIGMA0      ? APP_NOSIGMA0    : 0;
  app->flags |= ct->flags & CFG_F_ALLOW_CLI      ? APP_ALLOW_CLI   : 0;
  app->flags |= ct->flags & CFG_F_SHOW_APP_AREAS ? APP_SHOW_AREAS  : 0;
  app->flags |= ct->flags & CFG_F_STOP           ? APP_STOP        : 0;
  app->flags |= ct->flags & CFG_F_NOSUPERPAGES   ? APP_NOSUPER     : 0;
  app->flags |= ct->flags & CFG_F_ALL_WRITABLE   ? APP_ALL_WRITBLE : 0;

#ifdef USE_INTEGRITY
  app->flags |= ct->flags & CFG_F_HASH_BINARY    ? APP_HASH_BINARY : 0;
  env->loader_info.hash_dyn_libs = ct->flags & CFG_F_HASH_BINARY ? 1 : 0;
#else
  env->loader_info.hash_dyn_libs = 0;
#endif

#if defined(ARCH_x86) || defined(ARCH_amd64)
  env->loader_info.has_x86_vga  = ct->flags & CFG_F_ALLOW_VGA  ? 1 : 0;
  env->loader_info.has_x86_bios = ct->flags & CFG_F_ALLOW_BIOS ? 1 : 0;
#endif

  if (cfg_fiasco_symbols)
    app->flags |= APP_SYMBOLS;
  if (cfg_fiasco_lines)
    app->flags |= APP_LINES;

  app->owner      = owner;
  app->caphandler = ct->caphandler;
  app->caplist    = ct->caplist;
  app->kquota     = ct->kquota;

  if (ct->flags & CFG_F_INTERPRETER)
    {
      /* binary must be interpreted by libld-l4.s.so */
      app->flags   |= APP_MODE_INTERP;
      app->image    = ct->image;
      app->sz_image = ct->sz_image;
      app->ds_image = ct->ds_image;
      app_msg(app, "Starting application using %s", interp);
      if (   (error = app_create_tid(app))
	  || (error = app_start_interp(ct, app)))
	;

#ifdef USE_INTEGRITY
      if (!error && ct->flags & CFG_F_HASH_BINARY)
        integrity_report_hash(ct, app);
#endif

      /* save task id for the client which sent the open() request */
      ct->task_id = app->tid;

      *ret_val = app;
      return error;
    }

  if (ct->flags & CFG_F_L4ENV_BINARY)
    app_msg(app, "Starting static l4env-style application");
  else
    app_msg(app, "Starting sigma0-style application");
  app->flags |= APP_MODE_SIGMA0;

  app->image    = ct->image;
  app->sz_image = ct->sz_image;
  app->ds_image = ct->ds_image;

  elf_map_binary(app);
  junk_ds(&app->ds_image, app->image);
  if (   (error = app_create_tid(app))
      || (error = app_start_static(ct, app)))
    ;

#ifdef USE_INTEGRITY
  if (!error && ct->flags & CFG_F_HASH_BINARY)
    integrity_report_hash(ct, app);
#endif

  /* save task id for the client which sent the open() request */
  ct->task_id = app->tid;

  *ret_val = app;
  return error;
}

int
app_cont(app_t *app)
{
  if (!(app->flags & APP_CONT))
    {
      app->flags |= APP_CONT;

      return (app->flags & APP_MODE_SIGMA0)
		? app_cont_static(app)
		: app_cont_interp(app);
    }

  return -L4_EINVAL;
}

int
app_boot(cfg_task_t *ct, l4_taskid_t owner)
{
  int error;
  app_t *app = 0;

  error = app_init(ct, owner, &app);
  if (!app)
    return error;

  if (!error && (!(app->flags & APP_STOP)))
    error = app_cont(app);

  if (error)
    {
      /* kill task, free task resources */
      app_cleanup_extern(app);
      /* free our task resources */
      if (!app_cleanup_intern(app))
        printf("==> App successfully purged\n");
      else
        printf("==> App not fully purged!\n");
    }

  return error;
}

/** Kill application.
 *
 * \param task_id	task to be killed
 * \param caller_id task which wants to kill task_id
 * \return		0 on success
 *			-L4_ENOTFOUND if no proper task was not found */
int
app_kill(l4_taskid_t task_id, l4_taskid_t caller)
{
  int i;

  if (!use_events)
    {
      LOG("Cannot kill tasks without event server (use --events switch)!");
      return -L4_EINVAL;
    }

  // distinct between invocation triggered by
  // 1. messages received from events server and forwarded by internal
  //    events thread of the loader [old handling]
  // 2. messages received from outside (e.g. 'run' client) [new handling]
  //    or by invocations during early loading/init of apps by loader [old handling]
  if (l4_task_equal(l4_myself(), caller))
    {
      for (i=0; i<MAX_APP; i++)
        {
          app_t *app = app_array + i;
          if (app->env && l4_thread_equal(task_id, app->tid))
            return app_cleanup_intern(app);
        }
    }
  else
    {
      app_t * t_kill = task_to_app(task_id);
      app_t * t_caller = task_to_app(caller);
      if ((t_kill != NULL && l4_task_equal(t_kill->owner, caller)) ||
          (t_kill != NULL && t_caller != NULL && t_caller->flags & APP_ALLOW_KILL))
        return app_cleanup_extern(t_kill);
      else
        {
          LOG("Task "l4util_idfmt" is not allowed to kill tasks", l4util_idstr(caller));
          return -L4_EPERM;
        }
    }

  return -L4_ENOTFOUND;
}

/** Dump application to standard output.
 *
 * \param task_id	L4 task id or 0 to dump all tasks
 * \return		0 on success
 *			-L4_ENOTFOUND if no proper task was not found */
int
app_dump(unsigned long task_id)
{
  int i;

  for (i=0; i<MAX_APP; i++)
    {
      if ((app_array[i].env != NULL) &&
          ((task_id == 0) || (task_id == app_array[i].tid.id.task)))
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

/** Deliver application info to flexpage.
 *
 * \param task_id	L4 task id
 * \param l4env_ds	dataspace containing l4env infopage information
 * \param client	Client to transfer the ownership to
 * \retval fname	application name
 * \return		0 on success
 *			-L4_ENOTFOUND if no proper task was not found */
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
      if ((error = create_ds(app_dsm_id, L4_PAGESIZE,
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

	      if ((error = create_ds(app_dsm_id, L4_PAGESIZE,
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

/*@}*/
