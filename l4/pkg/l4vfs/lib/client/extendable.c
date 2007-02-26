/**
 * \file   l4vfs/lib/client/extendable.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "extendable-client.h"

#include <l4/l4vfs/types.h>
#include <l4/l4vfs/extendable.h>

#include <l4/log/l4log.h>

#include <stdlib.h>

int l4vfs_attach_namespace(l4_threadid_t server,
                           volume_id_t volume_id,
                           char * mounted_dir,
                           char * mount_dir)
{
    int ret;
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    ret = l4vfs_extendable_attach_namespace_call(&server, volume_id,
                                                 mounted_dir, mount_dir,
                                                 &_dice_corba_env);

    if ( DICE_HAS_EXCEPTION(&_dice_corba_env) )
    {
        LOG("Attach failed due to an exception!");
        if (DICE_EXCEPTION_MAJOR(&_dice_corba_env) == CORBA_SYSTEM_EXCEPTION)
        {
            switch( DICE_EXCEPTION_MINOR(&_dice_corba_env) )
            {
                case CORBA_DICE_EXCEPTION_WRONG_OPCODE:
                    LOG("Server did not recognize the opcode");
                    return 1;
                case CORBA_DICE_EXCEPTION_IPC_ERROR:
                    LOG("IPC error occured.");
                    return 2;
                default:
                    LOG("some other error found: %d", 
                            DICE_EXCEPTION_MINOR(&_dice_corba_env) );
                    return 3;
            }
        }
    }
    return ret;
}

int l4vfs_detach_namespace(l4_threadid_t server,
                           char * mount_dir)
{
    int ret;
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    ret = l4vfs_extendable_detach_namespace_call(&server, mount_dir,
                                                 &_dice_corba_env);

    if ( DICE_HAS_EXCEPTION(&_dice_corba_env) )
    {
        LOG("Detach failed due to an exception!");
        if (DICE_EXCEPTION_MAJOR(&_dice_corba_env) == CORBA_SYSTEM_EXCEPTION)
        {
            switch( DICE_EXCEPTION_MINOR(&_dice_corba_env) )
            {
                case CORBA_DICE_EXCEPTION_WRONG_OPCODE:
                    LOG("Server did not recognize the opcode");
                    return 1;
                case CORBA_DICE_EXCEPTION_IPC_ERROR:
                    LOG("IPC error occured.");
                    return 2;
                default:
                    LOG("some other error found: %d", 
                            DICE_EXCEPTION_MINOR(&_dice_corba_env) );
                    return 3;
            }
        }
    }
    return ret;
}
