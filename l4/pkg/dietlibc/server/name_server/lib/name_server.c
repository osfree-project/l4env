#include <l4/dietlibc/name_server.h>

#include <l4/sys/types.h>
#include <l4/rmgr/librmgr.h>

static int initialized = 0;
static l4_threadid_t name_server_thread_id;

/* shamelessly stolen from names' lib
 */
l4_threadid_t get_name_server_threadid()
{
    int error;

    if (! initialized)
    {
        if (!rmgr_init())
            return L4_NIL_ID;

        error = rmgr_get_task_id("name_server", &name_server_thread_id);
        if (error)
            return L4_NIL_ID;

        if (! l4_is_nil_id(name_server_thread_id))
            initialized = 1;
    }
    // fast fix for l4env: running thread is 2
    name_server_thread_id.id.lthread = 2;

    return name_server_thread_id;
}
