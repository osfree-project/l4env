#include "basic_name_server-client.h"

#include <l4/dietlibc/io-types.h>
#include <l4/dietlibc/basic_name_server.h>

#include <stdlib.h>

#if 0
void *CORBA_alloc(unsigned long size) __attribute__((weak));
void *CORBA_alloc(unsigned long size)
{
    return malloc(size);
}

void CORBA_free(void * prt) __attribute__((weak));
void CORBA_free(void * prt)
{
    free(prt);
}
#endif

static CORBA_Environment _dice_corba_env = dice_default_environment;

object_id_t resolve(l4_threadid_t server,
                    object_id_t base,
                    const char * pathname)
{
    object_id_t ret;
    ret = io_basic_name_server_resolve_call(&server,
                                            &base,
                                            pathname,
                                            &_dice_corba_env);
    return ret;
}

char *  rev_resolve(l4_threadid_t server,
                    object_id_t dest,
                    object_id_t *parent)
{
    char * ret;
    ret = io_basic_name_server_rev_resolve_call(&server,
                                                &dest,
                                                parent,
                                                &_dice_corba_env);
    return ret;
}

l4_threadid_t thread_for_volume(l4_threadid_t server,
                                volume_id_t volume_id)
{
    l4_threadid_t ret;
    ret = io_basic_name_server_thread_for_volume_call(&server,
                                                      volume_id,
                                                      &_dice_corba_env);
    return ret;
}
