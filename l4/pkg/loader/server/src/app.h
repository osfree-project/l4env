/**
 * \file	loader/server/src/app.h
 * \brief	application descriptor
 *
 * \date	06/10/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __APP_H_
#define __APP_H_

#include <l4/sys/types.h>
#include <l4/l4rm/l4rm.h>

#include "cfg.h"
#include "debug.h"

#define MAX_APP_AREA	28		/**< maximum entries for memory
					  regions of an application */
#define DEFAULT_PRIO	0x10		/**< default priority for new tasks */
#define DEFAULT_MCP	0xff		/**< default mcp for new tasks */

/** Pair of addresses. */
typedef struct
{
  l4_addr_t		here;		/**< begin of region in our address
					     space. */
  l4_addr_t		app;		/**< begin of region in application's
					     address space. */
} app_addr_t;

/** Application's area to page by our pager thread. */
typedef struct
{
  app_addr_t		beg;		/**< begin of region here/in app. */
  l4_size_t		size;		/**< size of region. */
  l4_uint16_t		type;		/**< region type. */
  l4_uint16_t		flags;		/**< region flags. */
#define APP_AREA_VALID	0x0001		/**< entry is valid */
#define APP_AREA_PAGE	0x0002		/**< forward pagefaults to ds */
#define APP_AREA_MMIO	0x0004		/**< don't kill this area */
#define APP_AREA_NOSUP	0x0008		/**< don't page superpages */
  l4dm_dataspace_t	ds;		/**< dataspace to serve. */
  const char		*dbg_name;
} app_area_t;

/** Application descriptor. */
typedef struct 
{
  l4_threadid_t		tid;		/**< L4 task id. */
  int			hi_first_msg;	/**< true if first message. */
  l4env_infopage_t	*env;		/**< ptr to environment infopage. */
  app_area_t		app_area[MAX_APP_AREA]; /**< pager regions. */
  int			app_area_next_free;	/**< number of pager regions. */
  char			*iobitmap;	/**< I/O permission bitmap. */
  const char		*fname;		/**< app name including path. */
  const char            *name;		/**< app name excluding path. */
  l4_addr_t		eip, esp;
  l4_uint32_t		flags;		/**< flags. */
#define APP_MODE_SIGMA0	0x00000001	/**< emulate sigma0 style application */
#define APP_MODE_LOADER	0x00000002	/**< loader mode */
#define APP_MODE_INTERP	0x00000004	/**< interpret using libld-l4.s.so */
#define APP_DIRECTMAP	0x00000008	/**< map program sections one-by-one */
#define APP_SYMBOLS	0x00000010	/**< load symbols */
#define APP_LINES	0x00000020	/**< load lines information */
#define APP_REBOOTABLE	0x00000040	/**< application may reboot system */
#define APP_NOVGA	0x00000080	/**< access to VGA memory denied */
#define APP_NOSIGMA0	0x00000100	/**< don't page other regions */
#define APP_ALLOW_CLI	0x00000200	/**< task may execute cli/sti */
#define APP_SHOW_AREAS	0x00000400	/**< show app areas before start */
#define APP_STOP	0x00000800	/**< stop app just before start */
#define APP_CONT	0x00001000	/**< ensure that we cont only once */
#define APP_NOSUPER	0x00002000	/**< don't page superpages */
#define APP_ALL_WRITBLE	0x00004000	/**< all sections writable */
#define APP_MSG_IO	0x00010000	/**< internal pager flag */
  l4_addr_t		image;		/**< attached image */
  l4_size_t		sz_image;	/**< size of attached image */
  l4dm_dataspace_t	ds_image;	/**< dataspace containing the image */
  l4_addr_t		symbols;	/**< symbols. */
  l4_size_t		sz_symbols;	/**< size of symbols information. */
  l4dm_dataspace_t	ds_symbols;	/**< ... dataspace. */
  l4_addr_t		lines;		/**< lines. */
  l4_size_t		sz_lines;	/**< size of lines information. */
  l4dm_dataspace_t	ds_lines;	/**< lines dataspace. */
  l4_addr_t		last_pf;	/**< last pagefault address. */
  l4_addr_t		last_pf_eip;	/**< last pagefault eip. */
  l4_uint32_t		prio;		/**< priority of thread 0. */
  l4_uint32_t		mcp;		/**< mcp of thread 0. */
  l4_taskid_t		owner;		/**< owner of that task. */
  l4_threadid_t         caphandler;     /**< capability fault handler */
  cfg_cap_t             *caplist;       /**< list of capabilities */
} app_t;

#define HERE_TO_APP(addr, base) \
  (base).app + (((unsigned int)(addr) - (base).here))

app_t* task_to_app(l4_threadid_t tid);
int  create_app_desc(app_t **new_app);

void __attribute__ ((format (printf, 2, 3)))
     app_msg(app_t *app, const char *format, ...);

void app_list_addr(app_t *app);
int  app_boot(cfg_task_t *ct, l4_taskid_t owner);
int  app_cont(app_t *app);
int  app_kill(l4_taskid_t task_id);
int  app_dump(unsigned long task_id);
int  app_info(unsigned long task_id, l4dm_dataspace_t *l4env_ds, 
	      l4_threadid_t client, char **fname);

void app_share_sections_with_client(app_t *app, l4_threadid_t client);
int  init_infopage(l4env_infopage_t *env);

int  app_attach_ds_to_pager(app_t *app, l4dm_dataspace_t *ds, l4_addr_t addr,
                            l4_size_t size, l4_uint16_t type, 
			    l4_uint32_t rights, 
			    int attach, const char *dbg_name, app_area_t **aa);

#endif
