/**
 * \file   libc_backends_l4env/include/mmap_util.h
 * \brief  Manages mappings between locally mapped dataspaces and the
 *         mapping servers
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __LIBC_BACKENDS_L4ENV_INCLUDE_MMAP_UTIL_H_
#define __LIBC_BACKENDS_L4ENV_INCLUDE_MMAP_UTIL_H_

/* Stores the mapping between a locally mapped dataspace and the
 * server where we got this mmaped from.
 */
typedef struct ds2server_s
{
    l4dm_dataspace_t ds;
    l4_threadid_t    id;
    l4_uint32_t      area_id;
} ds2server_t;

int add_ds2server(l4dm_dataspace_t *, l4_threadid_t, l4_uint32_t);
int del_ds2server(l4dm_dataspace_t *);
ds2server_t * get_ds2server(l4dm_dataspace_t *);
void init_ds2server(void);

#endif
