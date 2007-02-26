/* $Id$ */
/*****************************************************************************/
/**
 * \file	generic_io/lib/src/init.c
 *
 * \brief	L4Env I/O Client Library Initialization
 *
 * \author	Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 * Copyright (C) 2001-2002
 * Dresden University of Technology, Operating Systems Research Group
 *
 * This file contains free software, you can redistribute it and/or modify 
 * it under the terms of the GNU General Public License, Version 2 as 
 * published by the Free Software Foundation (see the file COPYING). 
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * For different licensing schemes please contact 
 * <contact@os.inf.tu-dresden.de>.
 */
/*****************************************************************************/

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/syscalls.h>
#include <l4/l4rm/l4rm.h>
#include <l4/names/libnames.h>

/* OSKit includes */
#include <stdio.h>

/* io includes */
#include <l4/generic_io/libio.h>

/* local */
#include "internal.h"
#include "__macros.h"

/* FIXME: hardcoded io names string */
char *IO_NAMES_STR = "io";	/**< unique name of io server */

static l4io_info_t io_info __attribute__ ((section (".io_info_page")));
unsigned long jiffies(void) __attribute__((weak, alias("io_info+4")));
unsigned long HZ(void) __attribute__((weak, alias("io_info+8")));
unsigned long xtime(void) __attribute__((weak, alias("io_info+12")));

l4_threadid_t io_l4id = L4_INVALID_ID;	/**< io's thread id */

static int _initialized = 0;	/**< initialization flag */

/*****************************************************************************/
/**
 * \brief  Registration wrapper
 *
 * \param  type		driver information
 *
 * \return 0 on success, negative error code otherwise
 */
/*****************************************************************************/
static int __io_register(l4_uint32_t * type)
{
  int error;
  sm_exc_t _exc;

  error = l4_io_register_client(io_l4id, *type, &_exc);
  return FLICK_ERR(error, &_exc);
}

/*****************************************************************************/
/** Info page mapping wrapper
 *
 * \param  addr		mapping address
 * \retval addr		actual mapping address
 *
 * \return 0 on success, negative error code otherwise
 *
 * \todo clean up l4_fpage_unmap(io_page) workaround
 */
/*****************************************************************************/
static int __io_mapping(l4io_info_t **addr)
{
  int error;
  sm_exc_t _exc;

  l4_fpage_t rfp;		/* receive fpage desc */
  l4_snd_fpage_t info;		/* received fpage */

  if (*addr == 0)
    *addr = &io_info;

  /* request io info page */
  rfp = l4_fpage((l4_uint32_t)*addr, L4_LOG2_PAGESIZE, 0, 0);

  /* XXX who'll get this page afterwards? */
  l4_fpage_unmap(rfp, L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);

  DMSG("receiving fpage {0x%08x, 0x%08x}\n", rfp.fp.page << 12, 1 << rfp.fp.size);

  error = l4_io_map_info(io_l4id, rfp, &info, &_exc);

  DMSG("received fpage {0x%08x, 0x%08x}\n",
       info.fpage.fp.page << 12, 1 << info.fpage.fp.size);

  if ((error=FLICK_ERR(error, &_exc)))
    return error;

  if ((*addr)->magic != L4IO_INFO_MAGIC)
    {
#ifdef DEBUG_ERRORS
      l4dm_dataspace_t ds;
      l4_offs_t offset; 
      l4_addr_t map_addr;
      l4_size_t map_size;

      ERROR("mapping io info page");

      error = l4rm_lookup(*addr,
			  &ds, &offset, &map_addr, &map_size);
      if (error)
	DMSG("  l4rm_lookup for io info page address failed (%s)\n",
	     l4env_errstr(error));
      else
	DMSG("  io info page: offset=%d addr=%d size=%d\n",
	     offset, map_addr, map_size);
      l4rm_show_region_list();
#endif
      return -L4_ENOTFOUND;
    }

  DMSG("magic: %08x (%c%c%c%c)\n",
       (*addr)->magic,
       ((*addr)->magic) >> 24,
       ((*addr)->magic) >> 16 & 0xff,
       ((*addr)->magic) >> 8 & 0xff,
       ((*addr)->magic) & 0xff);

  return 0;
}

/*****************************************************************************/
/** Library initialization
 *
 * \param  io_info_addr desired address for mapping of io info page:
 *			- 0: libio uses dedicated section and provides \a
 *			  jiffies and \a HZ symbols
 *			- -1: no mapping is done at all
 *			- otherwise \a io_info_addr is used; area has to be
 *			  prereserved at RM
 * \param  type		driver class (look into libio.h)
 *
 * \retval io_info_addr actual mapping address (or -1 if no mapping)
 *
 * \return 0 on success, negative error code otherwise
 *
 * This initializes libio. Before io info page is mapped into client's address
 * space any potentially mapping at the given address is FLUSHED! \a
 * io_info_addr has to be pagesize aligned.
 *
 * \todo driver classes (OS, device, ...)
 */
/*****************************************************************************/
int l4io_init(l4io_info_t **io_info_addr, l4io_drv_t type)
{
  int error;

  if (_initialized)
    return 0;

  /* check sanity of param */
  if (!io_info_addr ||
      (((l4_uint32_t)*io_info_addr & ~L4_PAGEMASK) && (*io_info_addr != (void*)-1)))
    return -L4_EINVAL;

  /* query for io @ names */
  if (names_waitfor_name(IO_NAMES_STR, &io_l4id, 50000) == 0)
    {
      ERROR("%s not registered at names", IO_NAMES_STR);
      return -L4_ENOTFOUND;
    }

  /* register before any other request */
  if ((error = __io_register((l4_uint32_t *)&type)))
    {
      ERROR("while registering at io (%d)", error);
      return error;
    }

  /* map info page if requested */
  if ((*io_info_addr != (void*)-1) && (error = __io_mapping(io_info_addr)))
    {
      ERROR("while mapping io info page (%d)", error);
      return error;
    }

  ++_initialized;
  return 0;
}
