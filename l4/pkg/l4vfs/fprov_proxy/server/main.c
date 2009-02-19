/**
 * \file   l4vfs/fprov_proxy/server/main.c
 * \brief  Server implements generic_fprov interface and use the libc
 *         posix functions to resolve files (e.g. simplefileserver)
 *         
 *         Code is borrowed from TFTP server.
 *         
 * \date   10/11/2004
 * \author Alexander Boettcher <ab764283@inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/env/errno.h>                          /* L4_* */
#include <l4/log/l4log.h>                          /* LOG* */
#include <l4/generic_fprov/generic_fprov-server.h> /* l4fprov_file_* */ 
#include <l4/dm_phys/dm_phys.h>                    /* l4dm, l4rm     */
#include <l4/names/libnames.h>                     /* names_* */ 
#include <l4/util/util.h>                          /* for l4_sleep*/

#include <fcntl.h>    /* O_RDONLY */
#include <errno.h>    /* errno */
#include <unistd.h>   /* lseek */

#ifdef DEBUG
static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

long
l4fprov_file_open_component (CORBA_Object _dice_corba_obj,
                             const char* fname,
                             const l4_threadid_t *dm,
                             unsigned long flags,
                             l4dm_dataspace_t *ds,
                             l4_size_t *size,
                             CORBA_Server_Environment *_dice_corba_env)
{
    int err,fd;
    int errcount = 0;
    long posEnd,res;
    long toread;
    char *addr;
    char buf[L4DM_DS_NAME_MAX_LEN];
    unsigned int part = 1024*1024*4;
    
    if (!fname)
    {
        LOGd(_DEBUG, "error - no filename given");
        return -L4_ENOTFOUND;
    }

    fd = open(fname, O_RDONLY);
    if (fd == -1)
    {
        LOGd(_DEBUG, "error %d - opening file %s", errno, fname);
        return -L4_ENOTFOUND;
    }

    posEnd = lseek(fd, 0, SEEK_END);
    if (posEnd == -1)
    {
        LOGd(_DEBUG, "error %d - lseek(SEEK_END) %s", errno, fname);
        return -L4_ENOTFOUND;
    }

    res = lseek(fd, 0, SEEK_SET);
    if (res == -1)
    {
        LOGd(_DEBUG, "error %d - lseek(SEEK_SET) %s", errno, fname);
        return -L4_ENOTFOUND;
    }

    *size = posEnd;

    if ((addr = l4dm_mem_ds_allocate_named_dsm(*size, flags+L4RM_MAP,
				               buf, *dm,
                                               (l4dm_dataspace_t *)ds)) == 0)
    {
        LOGd(_DEBUG,"error - get no memory (size=%d)", *size);
        close(fd);
        return -L4_ENOMEM;
    }
    LOG("Loading %s [%dkB]", fname, *size / 1024 + 1);

    /* read file, expect not to get all at once*/ 
    toread = *size;

    while (toread != 0)
    {
        res = read(fd, addr+*size-toread, toread < part ? toread : part);
        if (res  <= 0)
        {
            errcount ++;
            LOGd(_DEBUG, "error - nothing read(%ld) = %ld! %s",
                 toread, res, fname);
            /* shall I sleep ? */
            //l4_sleep(500);

            if (errcount==3) /* how often to try ? */
            {
                LOGd(_DEBUG,
                     "error - 3 times nothing read %s -> abort",
                     fname);
                l4dm_close((l4dm_dataspace_t*)ds);
                close(fd);
                return -L4_ENOMEM;
            }
        }
        else
            errcount = 0;

        toread -= res;
    }

    close(fd);

    /* detach dataspace */
    if ((err = l4rm_detach((void *)addr)))
    {
        LOGd(_DEBUG, "error %s - l4rm_detach() %s", l4env_errstr(err), fname);
        l4dm_close((l4dm_dataspace_t*)ds);
        return -L4_ENOMEM;
    }

    /* pass dataspace to client */
    if ((err = l4dm_transfer((l4dm_dataspace_t *)ds, *_dice_corba_obj)) != 0)
    {
        LOGd(_DEBUG,"error %s - l4dm_transfer() %s", l4env_errstr(err), fname);
        l4dm_close((l4dm_dataspace_t*)ds);
        return -L4_EINVAL;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    CORBA_Server_Environment env = dice_default_server_environment;
    l4_threadid_t id;

    if (!names_waitfor_name("fstab", &id, 10000))
        LOG("warning - fstab server not found");
 
    if (!names_register("fprov_proxy_fs"))
        LOG("warning - name server registration failed");

    l4fprov_file_server_loop(&env);

    LOG("warning - server exit");

    return 0; 

}
