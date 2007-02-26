#include <errno.h>

#include <l4/sys/types.h>
#include <l4/names/libnames.h>
#include <l4/log/l4log.h>
#include <l4/l4vfs/connection.h>
#include <l4/flips/libflips.h>

#include "flips-client.h"

static l4_threadid_t server = L4_INVALID_ID;
static l4_threadid_t flips_thread = L4_INVALID_ID;

int flips_connection(void);

int flips_connection(void)
{
    if (l4_is_invalid_id(flips_thread))
    {
        if (names_waitfor_name("FLIPS", &server, 100000) == 0)
        {
            LOG("FLIPS is not registered at names!\n");
            return -ENETDOWN;
        }

        flips_thread = l4vfs_init_connection(server);

        if (l4_is_invalid_id(flips_thread))
        {
            LOG_Error("Couldn't build up connection with FLIPS");
            return -ENETDOWN;
        }
    }
    return 0;
}

int flips_proc_read(const char *path, char *dst, int offset, int len)
{
    int ret;
    CORBA_Environment env = dice_default_environment;

    ret = flips_connection();

    if (ret != 0)
    {
        return ret;
    }

    return flips_proc_read_call(&flips_thread, path, dst, offset, &len, &env);
}

int flips_proc_write(const char *path, char *src, int len)
{
    int ret;
    CORBA_Environment env = dice_default_environment;

    ret = flips_connection();

    if (ret != 0)
    {
        return ret;
    }

    return flips_proc_write_call(&flips_thread, path, src, len, &env);
}
