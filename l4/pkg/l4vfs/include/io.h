/**
 * \file   dietlibc/include/io.h
 * \brief  Some data from the io backend
 *
 * \date   10/26/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __DIETLIBC_INCLUDE_IO_H_
#define __DIETLIBC_INCLUDE_IO_H_

#include <l4/l4vfs/types.h>

// this will be initialized by the constructor init_io()
extern l4_threadid_t l4vfs_name_server;

#endif
