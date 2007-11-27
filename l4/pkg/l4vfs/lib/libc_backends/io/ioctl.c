/**
 * \file   dietlibc/lib/backends/io/ioctl.c
 * \brief  
 *
 * \date   08/10/2004
 * \author Martin Pohlack  <mp26@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>
#include <libgen.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <termios.h>
#include <net/if.h>
#include <net/route.h> // for struct rtentry

#include <l4/l4vfs/types.h>
#include <l4/l4vfs/name_server.h>
#include <l4/l4vfs/basic_name_server.h>
#include <l4/l4vfs/basic_io.h>
#include <l4/l4vfs/container_io.h>
#include <l4/l4vfs/common_io.h>
#include <l4/l4vfs/file-table.h>
#include <l4/l4vfs/volumes.h>

#include <l4/crtx/ctor.h>

#include <l4/log/l4log.h>
#include <l4/util/l4_macros.h>
#include <l4/util/mbi_argv.h>   // for argc and argv[]
#include <l4/names/libnames.h>

#ifdef USE_UCLIBC
int ioctl( int fd, unsigned long cmd, ... )
#else
int ioctl( int fd, int cmd, ... )
#endif
{
    file_desc_t fdesc;
    int ret;
    size_t count;
    char *arg;
    va_list list;

    if (! ft_is_open(fd))
    {
		LOG_Error("fd %d not open", fd);
        errno = EBADF;
        return -1;
    }

    fdesc = ft_get_entry(fd);

    if (l4_is_invalid_id(fdesc.server_id))
    { // should not happen
		LOG_Error("invalid server for fd");
        errno = EBADF;
        return -1;
    }

    // there is always _one_ 3rd argument to ioctl. we extract
    // it as a pointer as it always points to a specific data type
    va_start( list, cmd );
    arg = va_arg( list, l4_int8_t * );
    va_end( list );

    /* PLEASE NOTE:     
     * Linux some day introduced the _IOC_-macros to handle ioctl
     * commands. Thus by default one should be able to determine
     * the size of an ioctl argument using the _IOC_SIZE macro.
     *
     * Unfortunately at the time _IOC came up, there already were
     * a lot of devices using ioctls not specified this way. These
     * commands do not include information about the argument's size
     * within the command number itself. However, we need to know
     * this exactly when sending this data with an IPC. 
     *
     * The only solution to the problem is: If your ioctl commands
     * do not work correctly because the optional argument is not sent
     * within the IPC, add a case concerning this ioctl in the switch
     * statement below.
     *
     * (Bjoern Doebel, 28.04.2005)
     */
    switch( cmd )
    {
        case TIOCGWINSZ:
            count = sizeof(struct winsize);
            break;
        case TCGETS:
        case TCSETS:
        case TCSETSW:
        case TCSETSF:
            count = sizeof( struct termios );
            break;
        case SIOCSIFFLAGS:
        case SIOCGIFFLAGS:
        case SIOCSIFADDR:
        case SIOCSIFDSTADDR:
        case SIOCGIFDSTADDR:
        case SIOCGIFADDR:
        case SIOCSIFNETMASK:
        case SIOCGIFNETMASK:
        case SIOCSIFBRDADDR:
        case SIOCGIFBRDADDR:
            count = sizeof (struct ifreq);
            break;
        case SIOCGIFCONF:
            count = sizeof(struct ifconf);
            break;
        case SIOCADDRT:
            count = sizeof(struct rtentry);
            break;
        case FIONREAD:
            count = sizeof(int);
            break;
        default:
           // by default the data size can be extracted from the command 
           // (thanks to the ioctl macros stolen from linux...:)
            count = _IOC_SIZE( cmd );
            if (count < 0)
                LOG("WARNING: seem to use ioctl with negative argument size.");
        break;
    }

    // now we can call the server
    ret = l4vfs_ioctl( fdesc.server_id,
                       fdesc.object_handle,
                       cmd,
                       &arg,
                       &count );

    return ret;
}
