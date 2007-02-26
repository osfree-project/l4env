/**
 * \file	loader/server/src/pager.h
 * \brief	Application pager. Should be moved to an own L4 server.
 *
 * \date	10/06/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __PAGER_H_
#define __PAGER_H_

#include <l4/sys/types.h>
#include "app.h"

extern l4_threadid_t app_pager_id;

int is_fiasco(void);
int start_app_pager(void);
l4_addr_t addr_app_to_here(app_t *app, l4_addr_t addr)
  __attribute__((regparm(3)));

#endif

