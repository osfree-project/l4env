/**
 * \file   l4vfs/fstab/server/main.c
 * \brief  
 *
 * \date   10/11/2004
 * \author Alexander Boettcher <boettcher@os.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */

#include <l4/log/l4log.h>         /* LOG* */
#include <l4/names/libnames.h>    /* names_register */
#include <l4/l4vfs/name_server.h> /* l4vfs_get_name_server_threadid */
#include <l4/l4vfs/extendable.h>  /* l4vfs_attach_namespace */
#include <l4/util/util.h>         /* l4_sleep_forever */

#include <fcntl.h>                /* O_RDONLY */
#include <stdlib.h>               /* atoi     */
#include <getopt.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#ifdef DEBUG

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

static int _DEBUG = 1;
#else
static int _DEBUG = 0;
#endif

static void fstab_info(void)
{
    LOG("usage: fstab ([-v<id> -b<mp> -m<pn>] | [-c<dir>])*\n"
        "  --volume-id,  -v <volume-id> . volume to mount\n"
        "  --base,       -b <mountpath> . mount what\n"
        "  --mountpoint, -m <pathname> .. mount to where\n"
        "  --create,     -c <path> ...... creates new directory\n"
        "\n"
        "example:\n"
        "  fstab -v 132 -b / -m /linux     \\\n"
        "        -v 14  -b / -m /simplefs  \\\n"
        "        -c /home -c /tmp");
}

int main(int argc, char *argv[])
{
    l4_threadid_t ns;
    int id     = 0;
    int fd     = 0;
    int ret    = 0;
    int option = 0;
    int step   = 0;
    char * src = 0;
    int optionid;
    int try;
    const struct option long_options[] =
        {
            { "volume-id",   1, NULL, 'v'},
            { "base",        1, NULL, 'b'},
            { "mountpoint",  1, NULL, 'm'},
            { "create",      1, NULL, 'c'},
            { 0, 0, 0, 0}
        };

    ns = l4vfs_get_name_server_threadid();

    /**
     * wait for file servers a little bit
     */
    l4_sleep(1000);

    do
    {
        option = getopt_long(argc, argv, "v:b:m:c:", long_options, &optionid);
        switch (option)
        {
        case 'v':
            if (step == 0)
            {
                id   = atoi(optarg);
                step = 1;
                LOGd(_DEBUG,"read volume-id = %d", id);
            }
            else
            {
                fstab_info();
                return 1;
            }
            break;
        case 'b':
            if (step == 1)
            {
                src  = optarg;
                step = 2;
                LOGd(_DEBUG,"read base = '%s'", src);
            }
            else
            {
                fstab_info();
                return 1;
            }
            break;
        case 'm':
            if (step == 2)
            {
                step  = 0;
                LOGd(_DEBUG,"read mountpoint = '%s'", optarg);

                fd = open(optarg, O_RDONLY);
                if (fd >= 0)
                {
                    close(fd);
                }
                else
                {
                    ret = mkdir(optarg, O_RDONLY);
                    if (ret)
                    {
                        LOG("Error: %d, Volume_id: %d, mkdir('%s')",
                            ret, id, optarg);
                        continue;
                    }
                }

                ret = 1;
                try = 0;
                while (ret && try < 10)
                {
                    if ((ret = l4vfs_attach_namespace(ns, id, src, optarg)))
                    {
                        try++;
                        LOG("Error: %d, Volume_id: %d, "
                            "attach_namespace('%s', '%s'), %d. attempt",
                            ret, id, src, optarg, try);
                        l4_sleep(2000);
                    }
                }
            }
            else
            {
                fstab_info();
                return 1;
            }

            break;
        case 'c':
            // fixme: What if optarg = "/a/b/c", and "a", "b", and "c" do not
            //        exist?
            ret   = mkdir(optarg, O_RDONLY);
            if (ret)
                LOG("Error %d, Volume_id %d, mkdir('%s')", ret, id, optarg);

            break;
        case -1:
            // prevents showing fstab_info if no more option exists
            break;
        default:
            LOG("Error: Unknown option %c", option);
            fstab_info();
            return 1;

            break;
        }
    } while (option != -1);

    /**
     * At the moment the registration is used as synchronization/information
     * for servers that the fstab server has finished the mounting.
     *
     * Better solutions are welcome ...
     */

    if (! names_register("fstab"))
        LOG("warning - name server registration failed");

#ifdef DEBUG
    /**
     * start debugging stuff
     */
    {
        DIR * D;
        struct dirent * dir;

        D = opendir("/");
        if (D == NULL)
        {
            LOG("errno = %d", errno);
            perror("opendir");
            LOG_flush();
            return 1;
        }
        while ((dir = readdir(D)))
        {
            LOG("'%s' (%ld)", dir->d_name, dir->d_ino);
            LOG_flush();
        }
        closedir(D);
    }
    /**
     * end debugging stuff
     */
#endif

    while(1)  // prevent deregistration an names via events
    {
        LOG("going to sleep ...");
        l4_sleep_forever();
    }

    return 0;
}
