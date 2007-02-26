#include "name_space_provider-client.h"

#include <l4/dietlibc/io-types.h>
#include <l4/dietlibc/name_space_provider.h>

static CORBA_Environment _dice_corba_env = dice_default_environment;

int register_volume(l4_threadid_t server, volume_id_t volume_id)
{
    int ret;
    ret = io_name_space_provider_register_volume_call(&server,
                                                      volume_id,
                                                      &_dice_corba_env);
    return ret;
}

int unregister_volume(l4_threadid_t server, volume_id_t volume_id)
{
    int ret;
    ret = io_name_space_provider_unregister_volume_call(&server,
                                                        volume_id,
                                                        &_dice_corba_env);
    return ret;
}
