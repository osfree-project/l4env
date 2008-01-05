/* $Id$ */
/*****************************************************************************/
/**
 * \file   l4io/server/include/io.h
 * \brief  L4Env l4io I/O Server Global Stuff
 *
 * \date   05/28/2003
 * \author Christian Helmuth <ch12@os.inf.tu-dresden.de>
 *
 */
/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#ifndef __L4IO_SERVER_INCLUDE_IO_H_
#define __L4IO_SERVER_INCLUDE_IO_H_

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/generic_io/types.h>

/* FIXME !!! see also libio implementation */
#define IO_NAMES_STR    "io"    /**< I/O names string */

/** l4io client structure type.
 * \ingroup grp_misc */
typedef struct io_client
{
  struct io_client *next;  /**< next client in list */
  l4_threadid_t c_l4id;    /**< client thread id */
  char name[16];           /**< name of client */
  l4io_drv_t drv;          /**< driver type */
} io_client_t;

/** \name Global l4io Server Vars
 *
 * @{ */
extern l4io_info_t io_info;

/** @} */
/** Test client equality.
 * \ingroup grp_misc
 *
 * \param  c0   first io client to test
 * \param  c1   second io client to test
 *
 * \return 0 on equality, non-zero on inequality
 */
extern __inline__ int client_equal(io_client_t *c0, io_client_t *c1);

extern __inline__ int client_equal(io_client_t *c0, io_client_t *c1)
{
  return l4_tasknum_equal(c0->c_l4id, c1->c_l4id);
}


/** Init static resource configuration */
extern int io_static_cfg_init(l4io_info_t *info, const char *requested_platform);


/** Init Fiasco-UX H/W resources */
extern int io_ux_init(void);

#endif
