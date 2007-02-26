#include "attachable-client.h"

#include <l4/dietlibc/io-types.h>
#include <l4/dietlibc/attachable.h>


static CORBA_Environment _dice_corba_env = dice_default_environment;

int attach_namespace(l4_threadid_t server,
                     volume_id_t volume_id,
                     char * mounted_dir,
                     char * mount_dir)
{
    int ret;
    ret = io_attachable_attach_namespace_call(&server,
                                              volume_id,
                                              mounted_dir,
                                              mount_dir,
                                              &_dice_corba_env);
    return ret;
}

int deattach_namespace(l4_threadid_t server,
                       char * mount_dir)
{
    int ret;
    ret = io_attachable_deattach_namespace_call(&server,
                                                mount_dir,
                                                &_dice_corba_env);
    return ret;
}
