/**
 * \file   l4vfs/lib/server/rev_resolve.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <basic_name_server-server.h>

// provide some dummy impl.
char* l4vfs_basic_name_server_rev_resolve_component(
    CORBA_Object _dice_corba_obj, const object_id_t *dest,
    object_id_t *parent, CORBA_Server_Environment *_dice_corba_env)
    __attribute__((weak));

char* l4vfs_basic_name_server_rev_resolve_component(
    CORBA_Object _dice_corba_obj, const object_id_t *dest,
    object_id_t *parent, CORBA_Server_Environment *_dice_corba_env)
{
    return NULL;
}
