/**
 * \file	loader/server/src/pager.h
 *
 * \date	10/06/2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de>
 *
 * \brief	Application pager. Should be moved to an own L4 server. */

#ifndef __LOADER_PAGER_H_
#define __LOADER_PAGER_H_

#include <l4/sys/types.h>

extern l4_threadid_t app_pager_id;

int start_app_pager(void);

#endif

