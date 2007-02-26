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

#define MAX_APP_AREA	32		/**< maximum entries for memory
					  regions of an application */
#define DEFAULT_PRIO	0x10		/**< default priority for new tasks */

/** pair of addresses */
typedef struct
{
  l4_addr_t		here;		/**< begin of region in our address
					  space */
  l4_addr_t		app;		/**< begin of region in applications
					  address space */
} app_addr_t;

/** application's area to page by our pager thread */
typedef struct
{
  app_addr_t		beg;		/**< begin of region here/in app */
  l4_size_t		size;		/**< size of region */
  l4_uint16_t		type;		/**< region type */
  l4_uint16_t		flags;		/**< region flags */
#define APP_AREA_VALID	0x0001		/**< entry is valid */
#define APP_AREA_PAGE	0x0002		/**< forward pagefaults to ds */
  l4dm_dataspace_t	ds;		/**< dataspace to serve */
  const char		*dbg_name;
} app_area_t;

/** application descriptor */
typedef struct 
{
  l4_threadid_t		tid;		/**< L4 task id */
  int			hi_first_msg;	/**< true if first message */
  l4env_infopage_t	*env;		/**< ptr to environment infopage */
  app_area_t		app_area[MAX_APP_AREA]; /**< pager regions */
  int			app_area_next_free;	/**< number of pager regions */
  const char		*fname;		/**< name of application */
  l4_uint32_t		flags;		/**< flags */
#define APP_OLDSTYLE	0x00000001	/**< emulate old style application */
#define APP_DIRECTMAP	0x00000002	/**< map program sections 1-by-1 */
#define APP_SYMBOLS	0x00000004	/**< load symbols */
#define APP_LINES	0x00000008	/**< load lines information */
#define APP_REBOOTABLE	0x00000010	/**< application may reboot system */
#define APP_NOVGA	0x00000020	/**< access to VGA memory denied */
#define APP_NOSIGMA0	0x00000040	/**< don't page other regions */
  l4_addr_t		symbols;	/**< symbols */
  l4dm_dataspace_t	symbols_ds;	/**< ... dataspace */
  l4_addr_t		lines;		/**< lines */
  l4dm_dataspace_t	lines_ds;	/**< ... dataspace */
  l4_addr_t		last_pf;	/**< last pagefault address */
  l4_addr_t		last_pf_eip;	/**< last pagefault eip */
  l4_uint32_t		prio;		/**< priority of thread 0 */
} app_t;

#define HERE_TO_APP(addr, base) \
  (base).app + (((unsigned int)(addr) - (base).here))

app_t* task_to_app(l4_threadid_t tid);
int  create_app_desc(app_t **new_app, const char *name);

void __attribute__ ((format (printf, 2, 3)))
     app_msg(app_t *app, const char *format, ...);

void app_list_addr(l4_threadid_t tid);
int  app_start(cfg_task_t *ct, l4_threadid_t fprov_id);

int  app_kill(unsigned long task_id);
int  app_dump(unsigned long task_id);
int  app_info(unsigned long task_id, l4dm_dataspace_t *l4env_ds, 
	      l4_threadid_t client, char **fname);
int  app_init(void);

int  init_infopage(l4env_infopage_t *env);

#endif

