/**
 * \file   rt_mon/server/coord/helper.h
 * \brief  Helper functions.
 *
 * \date   11/08/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#ifndef __RT_MON_SERVER_L4VFS_COORD_HELPER_H_
#define __RT_MON_SERVER_L4VFS_COORD_HELPER_H_

#include <l4/l4vfs/tree_helper.h>

/* Checks the usage count of a node and cleans up accordingly.
 */
void _check_and_clean_node(l4vfs_th_node_t * node);

/* Modifies name to a new version by appending "'", name is freed and
 * new pointer is returned.
 */
char * _version_name(char * name);

#endif
