/* $Id$ */
/*****************************************************************************/
/**
 * \file   generic_io/lib/clientlib/init.c
 * \brief  L4Env I/O Client Library Initialization
 *
 * \date   05/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <stdio.h>

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/env/errno.h>
#include <l4/sys/syscalls.h>
#include <l4/l4rm/l4rm.h>
#include <l4/names/libnames.h>

/* io includes */
#include <l4/generic_io/libio.h>

/* local */
#include "internal.h"
#include "__macros.h"

/* FIXME: hardcoded io names string */
char *IO_NAMES_STR = "io";  /**< unique name of io server */

/* Do we want logging of IO Infopage mapping? */
#define CONFIG_LOG_INFOPAGE_MAPPING 0

l4_threadid_t io_l4id = L4_INVALID_ID;  /**< io's thread id */

static int _initialized = 0;  /**< initialization flag */
static l4io_info_t *io_info_page_pointer;

/*****************************************************************************/
/**
 * \brief  Registration wrapper
 *
 * \param  type  driver information
 *
 * \return 0 on success, negative error code otherwise
 */
/*****************************************************************************/
static int __io_register(l4_uint32_t * type)
{
  int error;
  CORBA_Environment _env = dice_default_environment;

  error = l4_io_register_client_call(&io_l4id, *type, &_env);
  return DICE_ERR(error, &_env);
}

/*****************************************************************************/
/** Info page mapping wrapper
 *
 * \param  addr  mapping address
 * \retval addr  actual mapping address
 *
 * \return 0 on success, negative error code otherwise
 *
 * \todo clean up l4_fpage_unmap(io_page) workaround
 */
/*****************************************************************************/
static int __io_mapping(l4io_info_t **addr)
{
  int error;
  CORBA_Environment _env = dice_default_environment;

  l4_fpage_t rfp;  /* receive fpage desc */
  l4_snd_fpage_t info;  /* received fpage */

  if (*addr == 0)
    *addr = &io_info;

  /* request io info page */
  rfp = l4_fpage((l4_umword_t)*addr, L4_LOG2_PAGESIZE, 0, 0);

  /* XXX who'll get this page afterwards? */
  l4_fpage_unmap(rfp, L4_FP_FLUSH_PAGE | L4_FP_ALL_SPACES);

  LOGd(CONFIG_LOG_INFOPAGE_MAPPING,
       "receiving fpage {0x%08x, 0x%08x}", rfp.fp.page << 12, 1 << rfp.fp.size);
  _env.rcv_fpage = rfp;

  error = l4_io_map_info_call(&io_l4id, &info, &_env);

  LOGd(CONFIG_LOG_INFOPAGE_MAPPING,
       "received fpage {0x%08x, 0x%08x}",
       info.fpage.fp.page << 12, 1 << info.fpage.fp.size);

  if ((error=DICE_ERR(error, &_env)))
    return error;

  if ((*addr)->magic != L4IO_INFO_MAGIC)
    {
#ifdef DEBUG_ERRORS
      l4dm_dataspace_t ds;
      l4_offs_t offset;
      l4_addr_t map_addr;
      l4_size_t map_size;
      l4_threadid_t dummy;

      LOG_Error("mapping io info page");

      error = l4rm_lookup(*addr, &map_addr, &map_size, &ds, &offset, &dummy);
      if (error < 0)
        LOG(" l4rm_lookup for io info page address failed (%s)",
            l4env_errstr(error));
      else if (error == L4RM_REGION_DATASPACE)
        LOG(" io info page: offset=%d addr=%d size=%d",
            offset, map_addr, map_size);
      else
        LOG(" invalid region type for io info page (type %d)", error);
      l4rm_show_region_list();
#endif
      return -L4_ENOTFOUND;
    }

  LOGd(CONFIG_LOG_INFOPAGE_MAPPING,
       "magic: %08lx (%c%c%c%c)",
       (*addr)->magic,
       (char)(((*addr)->magic) >> 24),
       (char)(((*addr)->magic) >> 16 & 0xff),
       (char)(((*addr)->magic) >> 8 & 0xff),
       (char)(((*addr)->magic) & 0xff));

  io_info_page_pointer = *addr;

  return 0;
}

/*****************************************************************************/
/** Library initialization
 *
 * \param  io_info_addr  desired address for mapping of io info page:
 *                       - 0: libio uses dedicated section and provides \a
 *                         jiffies and \a HZ symbols
 *                       - -1: no mapping is done at all
 *                       - otherwise \a io_info_addr is used; area has to be
 *                          prereserved at RM
 * \param  type          driver class (look into libio.h)
 *
 * \retval io_info_addr  actual mapping address (or -1 if no mapping)
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
  l4_uint32_t tmp_type;

  /* check sanity of param */
  if (!io_info_addr ||
      (((l4_umword_t)*io_info_addr & ~L4_PAGEMASK) && (*io_info_addr != (void*)-1)))
    return -L4_EINVAL;

  /* query for io @ names */
  if (names_waitfor_name(IO_NAMES_STR, &io_l4id, 50000) == 0)
    {
      LOG_Error("%s not registered at names", IO_NAMES_STR);
      return -L4_ENOTFOUND;
    }

  /* register before any other request */
  memcpy(&tmp_type, &type, sizeof(tmp_type));
  if (!_initialized)
    if ((error = __io_register(&tmp_type)))
      {
        LOG_Error("while registering at io (%d)", error);
        return error;
      }

  /* map info page if requested */
  if ((*io_info_addr != (void*)-1) && (error = __io_mapping(io_info_addr)))
    {
      LOG_Error("while mapping io info page (%d)", error);
      return error;
    }

  ++_initialized;
  return 0;
}

/*****************************************************************************/
/* return pointer to info page                                               */
/*****************************************************************************/
l4io_info_t *l4io_info_page(void)
{
  return io_info_page_pointer;

}
