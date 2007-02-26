/* $Id$ */
/*****************************************************************************/
/**
 * \file	l4io/server/include/io.h
 *
 * \brief	L4Env l4io I/O Server Global Stuff
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

#ifndef _IO_IO_H
#define _IO_IO_H

/* L4 includes */
#include <l4/sys/types.h>
#include <l4/generic_io/types.h>

/* FIXME !!! see also libio implementation */
#define IO_NAMES_STR	"io"	/**< I/O names string */

/** l4io client structure type.
 * \ingroup grp_misc */
typedef struct io_client
{
  struct io_client *next;	/**< next client in list */
  l4_threadid_t c_l4id;		/**< client thread id */
  char name[16];		/**< name of client */
  l4io_drv_t drv;		/**< driver type */
} io_client_t;

/*****************************************************************************/
/**
 * \name Global l4io Server Vars
 *
 * @{ */
/*****************************************************************************/
extern l4io_info_t io_info;

/** @} */
/*****************************************************************************/
/** Test client equality.
 * \ingroup grp_misc
 *
 * \param  c0		first io client to test
 * \param  c1		second io client to test
 *
 * \return 0 on equality, non-zero on inequality
 */
/*****************************************************************************/
extern __inline__ int client_equal(io_client_t *c0, io_client_t *c1);

extern __inline__ int client_equal(io_client_t *c0, io_client_t *c1)
{
  return l4_thread_equal(c0->c_l4id, c1->c_l4id);
}

#endif /* !_IO_IO_H */
