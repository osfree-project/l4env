/*!
 * \file	loader/server/src/fprov-if.h
 * \brief	interface to file provider
 *
 * \date	2001
 * \author	Frank Mehnert <fm3@os.inf.tu-dresden.de> */

/* (c) 2003 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details. */

#ifndef __FPROV_IF_H_
#define __FPROV_IF_H_

#include <l4/l4rm/l4rm.h>

extern l4_threadid_t tftp_id;

int load_file(const char *fname, l4_threadid_t fprov_id, l4_threadid_t dm_id,
	      int use_modpath, int contiguous, 
	      l4_addr_t *addr, l4_size_t *size, l4dm_dataspace_t *ds);

int fprov_if_init(void);

#endif

