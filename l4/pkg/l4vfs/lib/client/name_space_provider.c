/**
 * \file   l4vfs/lib/client/name_space_provider.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include "name_space_provider-client.h"

#include <l4/l4vfs/types.h>
#include <l4/l4vfs/name_space_provider.h>
#include <l4/log/l4log.h>

#include <stdlib.h>

int l4vfs_register_volume(l4_threadid_t dest, l4_threadid_t server,
                          object_id_t root_id)
{
    int ret;
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    ret = l4vfs_name_space_provider_register_volume_call(&dest, &server,
                                                         &root_id,
                                                         &_dice_corba_env);

    if ( _dice_corba_env.major != CORBA_NO_EXCEPTION )
    {
        LOG("Registering failed due to an exception!");
        if (_dice_corba_env.major == CORBA_SYSTEM_EXCEPTION)
        {
            switch( _dice_corba_env.repos_id )
            {
                case CORBA_DICE_EXCEPTION_WRONG_OPCODE:
                    LOG("Server did not recognize the opcode");
                    return 1;
                case CORBA_DICE_EXCEPTION_IPC_ERROR:
                    LOG("IPC error occured.");
                    return 2;
                default:
                    LOG("some other error found: %d", 
                            _dice_corba_env.repos_id );
                    return 3;
            }
        }
    }
    
    return ret;
}

int l4vfs_unregister_volume(l4_threadid_t dest, l4_threadid_t server,
                            volume_id_t volume_id)
{
    int ret;
    CORBA_Environment _dice_corba_env = dice_default_environment;
    _dice_corba_env.malloc = (dice_malloc_func)malloc;
    _dice_corba_env.free = (dice_free_func)free;

    ret = l4vfs_name_space_provider_unregister_volume_call(&dest, &server,
                                                           volume_id,
                                                           &_dice_corba_env);
    
    if ( _dice_corba_env.major != CORBA_NO_EXCEPTION )
    {
        LOG("Registering failed due to an exception!");
        if (_dice_corba_env.major == CORBA_SYSTEM_EXCEPTION)
        {
            switch( _dice_corba_env.repos_id )
            {
                case CORBA_DICE_EXCEPTION_WRONG_OPCODE:
                    LOG("Server did not recognize the opcode");
                    return 1;
                case CORBA_DICE_EXCEPTION_IPC_ERROR:
                    LOG("IPC error occured.");
                    return 2;
                default:
                    LOG("some other error found: %d", 
                            _dice_corba_env.repos_id );
                    return 3;
            }
        }
    }
    
    return ret;
}
