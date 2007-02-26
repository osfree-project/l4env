/**
 * \file   l4vfs/term_server/examples/vc_test/vc_plain.c
 * \brief  Play an ASCII movie provided by the simple file server.
 *
 * \date   2004 - 2006
 * \author Björn Döbel  <doebel@os.inf.tu-dresden.de>
 */
/* (c) 2004 Technische Universitaet Dresden
 * This file is part of DROPS, which is distributed under the terms of the
 * GNU General Public License 2. Please see the COPYING file for details.
 */
#include <l4/log/l4log.h>
#include <l4/l4vfs/basic_name_server.h>
#include <l4/l4vfs/extendable.h>
#include <l4/l4vfs/name_server.h>
#include <l4/l4vfs/volume_ids.h>
#include <l4/sys/types.h>
#include <l4/sys/syscalls.h>
#include <l4/util/util.h>
#include <l4/util/parse_cmd.h>
#include <dice/dice.h> // for CORBA_*

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>

char LOG_tag[9]="vc_plain";
char *the_movie;
int delay;

#ifdef DEBUG
    int _DEBUG = 1;
#else
    int _DEBUG = 0;
#endif

#define BUF_SIZE 1024

void *CORBA_alloc(unsigned long size)
{
    return malloc(size);
}

void CORBA_free(void * prt)
{
    free(prt);
}

int main(int argc, const char* argv[])
{
    int ret, ret3;
    l4_uint32_t count;
    char buf[100];

    parse_cmdline(
        &argc, &argv,
        'm', "movie", "movie to play (loaded from simple_file_server)",
        PARSE_CMD_STRING, "NO MOVIE", &the_movie, 
        'd', "delay", "delay (in ms) between characters (for smooth movie animation)",
        PARSE_CMD_INT, 10, &delay,
        0);

    LOG("movie to load: %s", the_movie);
    LOG("print delay: %d", delay);

    ret3 = open(the_movie, O_RDONLY);
    LOG("opened movie file: %d", ret3);
    if (ret3 < 0)
    {
        LOG_Error("Could not open movie file %s", the_movie);
        return 1;
    }

    // cursor OFF
    printf("\033[25l");
    while( (count = read(ret3, buf, 10)) )
    {
        write(1, buf, count);
        l4_sleep(delay);
    }
    printf("press return to close ...\n\n");
    ret = read(STDIN_FILENO, buf, 1);

    close(ret3);
    close(0);
    close(1);

    return 0;
}
